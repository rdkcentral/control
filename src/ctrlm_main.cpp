/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/sysinfo.h>
#include <poll.h>
#include <glib.h>
#include <string.h>
#include <semaphore.h>
#include <memory>
#include <algorithm>
#include <fstream>
#include <secure_wrapper.h>
#include <rdkversion.h>
#include "jansson.h"
#include "libIBus.h"
#include "irMgr.h"
#include "plat_ir.h"
#include "sysMgr.h"
#ifdef BREAKPAD_SUPPORT
#include "client/linux/handler/exception_handler.h"
#endif
#include "comcastIrKeyCodes.h"
#include "ctrlm_version_build.h"
#include "ctrlm_hal_rf4ce.h"
#include "ctrlm_hal_ble.h"
#include "ctrlm_hal_ip.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "ctrlm_database.h"
#include "ctrlm_rcu.h"
#include "ctrlm_validation.h"
#include "ctrlm_recovery.h"
#include "ctrlm_ir_controller.h"
#ifdef CTRLM_THUNDER
#include "ctrlm_thunder_plugin_device_info.h"
#endif
#ifdef AUTH_ENABLED
#include "ctrlm_auth.h"
#endif
#include "ctrlm_rfc.h"
#include "ctrlm_telemetry.h"
#include "ctrlm_config.h"
#include "ctrlm_config_default.h"
#include "ctrlm_tr181.h"
#include "rf4ce/ctrlm_rf4ce_network.h"
#include "ctrlm_device_update.h"
#include "ctrlm_vendor_network_factory.h"
#include "dsMgr.h"
#include "dsRpc.h"
#include "dsDisplay.h"
#ifdef SYSTEMD_NOTIFY
#include <systemd/sd-daemon.h>
#endif
#include <systemd/sd-bus.h>
#include "xr_voice_sdk.h"
#include "ctrlm_voice_obj.h"
#include "ctrlm_voice_obj_generic.h"
#include "ctrlm_voice_endpoint.h"
#include "ctrlm_irdb_factory.h"
#ifdef FDC_ENABLED
#include "xr_fdc.h"
#endif
#include<features.h>
#ifdef MEMORY_LOCK
#include "clnl.h"
#endif

using namespace std;

#ifndef CTRLM_VERSION
#define CTRLM_VERSION "1.0"
#endif

#define CTRLM_THREAD_NAME_MAIN          "Ctrlm Main"
#define CTRLM_THREAD_NAME_DATABASE      "Ctrlm Database"
#define CTRLM_THREAD_NAME_DEVICE_UPDATE "Ctrlm Device Update"
#define CTRLM_THREAD_NAME_VOICE_SDK     "Voice SDK"
#define CTRLM_THREAD_NAME_IR_CONTROLLER "IR Controller"
#define CTRLM_THREAD_NAME_RF4CE         "RF4CE"
#define CTRLM_THREAD_NAME_BLE           "BLE"

#define CTRLM_DEFAULT_DEVICE_MAC_INTERFACE "eth0"

#define CTRLM_RESTART_DELAY_SHORT    "0"
#define CTRLM_RESTART_UPDATE_TIMEOUT (5000)

#define CTRLM_RF4CE_LEN_IR_REMOTE_USAGE 14
#define CTRLM_RF4CE_LEN_LAST_KEY_INFO   sizeof(ctrlm_last_key_info)
#define CTRLM_RF4CE_LEN_SHUTDOWN_TIME   4
#define CTRLM_RF4CE_LEN_PAIRING_METRICS sizeof(ctrlm_pairing_metrics_t)

#define CTRLM_MAIN_QUEUE_REPEAT_DELAY   (5000)

#define NETWORK_ID_BASE_RF4CE   1
#define NETWORK_ID_BASE_IP      11
#define NETWORK_ID_BASE_BLE     21
#define NETWORK_ID_BASE_CUSTOM  41

#define CTRLM_MAIN_FIRST_BOOT_TIME_MAX (180) // maximum amount of uptime allowed (in seconds) for declaring "first boot"

typedef void (*ctrlm_queue_push_t)(gpointer);
typedef void (*ctrlm_monitor_poll)(void *data);

typedef struct {
   const char *                    name;
   ctrlm_queue_push_t              queue_push;
   ctrlm_obj_network_t *           obj_network;
   ctrlm_monitor_poll              function;
   ctrlm_thread_monitor_response_t response;
} ctrlm_thread_monitor_t;

typedef struct
{
   bool has_ir_xr2;
   bool has_ir_xr5;
   bool has_ir_xr11;
   bool has_ir_xr15;
   bool has_ir_remote;
} ctrlm_ir_remote_usage;

typedef struct {
   int                         controller_id;
   guchar                      source_type;
   guint32                     source_key_code;
   long long                   timestamp;
   ctrlm_ir_remote_type        last_ir_remote_type;
   bool                        is_screen_bind_mode;
   ctrlm_remote_keypad_config  remote_keypad_config;
   char                        source_name[CTRLM_RCU_RIB_ATTR_LEN_PRODUCT_NAME];
} ctrlm_last_key_info;

typedef struct {
   gboolean                            active;
   ctrlm_pairing_modes_t               pairing_mode;
   ctrlm_pairing_restrict_by_remote_t  restrict_by_remote;
   ctrlm_bind_status_t                 bind_status;
} ctrlm_pairing_window;

typedef struct {
   unsigned long            num_screenbind_failures;
   unsigned long            last_screenbind_error_timestamp;
   ctrlm_bind_status_t      last_screenbind_error_code;
   char                     last_screenbind_remote_type[CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH];
   unsigned long            num_non_screenbind_failures;
   unsigned long            last_non_screenbind_error_timestamp;
   ctrlm_bind_status_t      last_non_screenbind_error_code;
   unsigned char            last_non_screenbind_error_binding_type;
   char                     last_non_screenbind_remote_type[CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH];
} ctrlm_pairing_metrics_t;

static const char *key_source_names[sizeof(IARM_Bus_IRMgr_KeySrc_t)] = 
{
   "FP",
   "IR",
   "RF"
};

typedef struct {
   GThread *                          main_thread;
   GMainLoop *                        main_loop;
   sem_t                              semaphore;
   sem_t                              ctrlm_utils_sem;
   GAsyncQueue *                      queue;
   string                             stb_name;
   string                             receiver_id;
   string                             device_id;
   ctrlm_device_type_t                device_type;
   string                             service_account_id;
   string                             partner_id;
   string                             device_mac;
   string                             experience;
   string                             service_access_token;
   time_t                             service_access_token_expiration;
   guint                              service_access_token_expiration_tag;
   sem_t                              service_access_token_semaphore;
   string                             image_name;
   string                             image_branch;
   string                             image_version;
   string                             image_build_time;
   string                             db_path;
   string                             minidump_path;
   gboolean                           has_receiver_id;
   gboolean                           has_device_id;
   gboolean                           has_device_type;
   gboolean                           has_service_account_id;
   gboolean                           has_partner_id;
   gboolean                           has_experience;
   gboolean                           has_service_access_token;
   gboolean                           sat_enabled;
   gboolean                           production_build;
   guint                              thread_monitor_timeout_val;
   guint                              thread_monitor_timeout_tag;
   guint                              thread_monitor_index;
   bool                               thread_monitor_active;
   bool                               thread_monitor_minidump;
   string                             server_url_authservice;
   guint                              authservice_poll_val;
   guint                              authservice_poll_tag;
   guint                              authservice_fast_poll_val;
   guint                              authservice_fast_retries;
   guint                              authservice_fast_retries_max;
   guint                              bound_controller_qty;
   gboolean                           recently_booted;
   guint                              recently_booted_timeout_val;
   guint                              recently_booted_timeout_tag;
   gboolean                           line_of_sight;
   guint                              line_of_sight_timeout_val;
   guint                              line_of_sight_timeout_tag;
   gboolean                           autobind;
   guint                              autobind_timeout_val;
   guint                              autobind_timeout_tag;
   gboolean                           binding_button;
   guint                              binding_button_timeout_val;
   guint                              binding_button_timeout_tag;
   gboolean                           binding_screen_active;
   guint                              screen_bind_timeout_val;
   guint                              screen_bind_timeout_tag;
   gboolean                           one_touch_autobind_active;
   guint                              one_touch_autobind_timeout_val;
   guint                              one_touch_autobind_timeout_tag;
   ctrlm_pairing_window               pairing_window;
   bool                               mask_pii;
   guint                              crash_recovery_threshold;
   gboolean                           successful_init;
   ctrlm_ir_remote_usage              ir_remote_usage_today;
   ctrlm_ir_remote_usage              ir_remote_usage_yesterday;
   guint32                            today;
   ctrlm_pairing_metrics_t            pairing_metrics;
   char                               discovery_remote_type[CTRLM_HAL_RF4CE_USER_STRING_SIZE];
   time_t                             shutdown_time;
   ctrlm_last_key_info                last_key_info;
   gboolean                           loading_db;
   map<ctrlm_controller_id_t, bool>   precomission_table;
   map<ctrlm_network_id_t, ctrlm_obj_network_t *> networks;     // Map to hold the Networks that will be used by Control Manager
   map<ctrlm_network_id_t, ctrlm_network_type_t>  network_type; // Map to hold the Network types that will be used by Control Manager
   vector<ctrlm_thread_monitor_t>     monitor_threads;
   int                                return_code;
   ctrlm_voice_t                     *voice_session;
#ifdef IRDB_ENABLED
   ctrlm_irdb_t                      *irdb;
#endif
   ctrlm_telemetry_t                 *telemetry;
   ctrlm_cs_values_t                  cs_values;
#ifdef AUTH_ENABLED
   ctrlm_auth_t                      *authservice;
#endif
#ifdef CTRLM_THUNDER
   Thunder::DeviceInfo::ctrlm_thunder_plugin_device_info_t *thunder_device_info;
#endif
   ctrlm_power_state_t                power_state;
   gboolean                           auto_ack;
   gboolean                           local_conf;
   guint                              telemetry_report_interval;
   ctrlm_ir_controller_t             *ir_controller;
#ifdef DEEP_SLEEP_ENABLED
   gboolean                           wake_with_voice_allowed;
#endif
} ctrlm_global_t;

static ctrlm_global_t g_ctrlm;

// Prototypes
#ifdef AUTH_ENABLED
static gboolean ctrlm_has_authservice_data(void);
static gboolean ctrlm_load_authservice_data(void);
#ifdef AUTH_RECEIVER_ID
static gboolean ctrlm_load_receiver_id(void);
static void     ctrlm_main_has_receiver_id_set(gboolean has_id);
#endif
#ifdef AUTH_DEVICE_ID
static gboolean ctrlm_load_device_id(void);
static void     ctrlm_main_has_device_id_set(gboolean has_id);
#endif
#ifdef AUTH_ACCOUNT_ID
static gboolean ctrlm_load_service_account_id(const char *account_id);
static void     ctrlm_main_has_service_account_id_set(gboolean has_id);
#endif
#ifdef AUTH_PARTNER_ID
static gboolean ctrlm_load_partner_id(void);
static void     ctrlm_main_has_partner_id_set(gboolean has_id);
#endif
#ifdef AUTH_EXPERIENCE
static gboolean ctrlm_load_experience(void);
static void     ctrlm_main_has_experience_set(gboolean has_experience);
#endif
#ifdef AUTH_SAT_TOKEN
static gboolean ctrlm_load_service_access_token(void);
static void     ctrlm_main_has_service_access_token_set(gboolean has_token);
#endif
#endif
static gboolean ctrlm_load_version(void);
static gboolean ctrlm_load_device_mac(void);
static gboolean ctrlm_load_device_type(void);
static void     ctrlm_device_type_loaded(ctrlm_device_type_t device_type);
static void     ctrlm_main_has_device_type_set(gboolean has_type);
#ifdef CTRLM_THUNDER
static void ctrlm_device_info_activated(void *user_data);
#endif

static gboolean ctrlm_load_config(json_t **json_obj_root, json_t **json_obj_net_rf4ce, json_t **json_obj_voice, json_t **json_obj_device_update, json_t **json_obj_validation, json_t **json_obj_vsdk);
static gboolean ctrlm_iarm_init(void);
static void     ctrlm_iarm_terminate(void);
static gboolean ctrlm_networks_pre_init(json_t *json_obj_net_rf4ce, json_t *json_obj_root);
static gboolean ctrlm_networks_init(void);
static void     ctrlm_networks_terminate(void);
static void     ctrlm_thread_monitor_init(void);
static void     ctrlm_terminate(void);
static gboolean ctrlm_message_queue_delay(gpointer data);
static void     ctrlm_trigger_startup_actions(void);

static gpointer ctrlm_main_thread(gpointer param);
static void     ctrlm_queue_msg_destroy(gpointer msg);
static gboolean ctrlm_timeout_recently_booted(gpointer user_data);
static gboolean ctrlm_timeout_systemd_restart_delay(gpointer user_data);
static gboolean ctrlm_thread_monitor(gpointer user_data);
static gboolean ctrlm_was_cpu_halted(void);
static gboolean ctrlm_start_iarm(gpointer user_data);
#ifdef AUTH_ENABLED
static gboolean ctrlm_authservice_poll(gpointer user_data);
#ifdef AUTH_SAT_TOKEN
static gboolean ctrlm_authservice_expired(gpointer user_data);
#endif
#endif
static gboolean ctrlm_ntp_check(gpointer user_data);
static void     ctrlm_signals_register(void);
static void     ctrlm_signal_handler(int signal);

static void     ctrlm_main_iarm_call_status_get_(ctrlm_main_iarm_call_status_t *status);
static void     ctrlm_main_iarm_call_property_get_(ctrlm_main_iarm_call_property_t *property);
static void     ctrlm_main_iarm_call_discovery_config_set_(ctrlm_main_iarm_call_discovery_config_t *config);
static void     ctrlm_main_iarm_call_autobind_config_set_(ctrlm_main_iarm_call_autobind_config_t *config);
static void     ctrlm_main_iarm_call_precommission_config_set_(ctrlm_main_iarm_call_precommision_config_t *config);
static void     ctrlm_main_iarm_call_factory_reset_(ctrlm_main_iarm_call_factory_reset_t *reset);
static void     ctrlm_main_iarm_call_controller_unbind_(ctrlm_main_iarm_call_controller_unbind_t *unbind);
static void     ctrlm_main_update_export_controller_list(void);
static void     ctrlm_main_iarm_call_ir_remote_usage_get_(ctrlm_main_iarm_call_ir_remote_usage_t *ir_remote_usage);
static void     ctrlm_main_iarm_call_pairing_metrics_get_(ctrlm_main_iarm_call_pairing_metrics_t *pairing_metrics);
static void     ctrlm_main_iarm_call_last_key_info_get_(ctrlm_main_iarm_call_last_key_info_t *last_key_info);
static void     ctrlm_main_iarm_call_control_service_end_pairing_mode_(ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing);
static void     ctrlm_stop_one_touch_autobind_(ctrlm_network_id_t network_id);
static void     ctrlm_close_pairing_window_(ctrlm_network_id_t network_id, ctrlm_close_pairing_window_reason reason);
static void     ctrlm_pairing_window_bind_status_set_(ctrlm_bind_status_t bind_status);
static void     ctrlm_discovery_remote_type_set_(const char* remote_type_str);
static void     ctrlm_controller_product_name_get(ctrlm_controller_id_t controller_id, char *source_name);
static void     ctrlm_global_rfc_values_retrieved(const ctrlm_rfc_attr_t &attr);

static gboolean ctrlm_timeout_line_of_sight(gpointer user_data);
static gboolean ctrlm_timeout_autobind(gpointer user_data);
static gboolean ctrlm_timeout_binding_button(gpointer user_data);
static gboolean ctrlm_timeout_screen_bind(gpointer user_data);
static gboolean ctrlm_timeout_one_touch_autobind(gpointer user_data);

static gboolean ctrlm_main_handle_day_change_ir_remote_usage();
static void     ctrlm_property_write_ir_remote_usage(void);
static guchar   ctrlm_property_write_ir_remote_usage(guchar *data, guchar length);
static void     ctrlm_property_read_ir_remote_usage(void);
static void     ctrlm_property_write_pairing_metrics(void);
static guchar   ctrlm_property_write_pairing_metrics(guchar *data, guchar length);
static void     ctrlm_property_read_pairing_metrics(void);
static void     ctrlm_property_write_last_key_info(void);
static guchar   ctrlm_property_write_last_key_info(guchar *data, guchar length);
static void     ctrlm_property_read_last_key_info(void);
static void     ctrlm_property_write_shutdown_time(void);
static guchar   ctrlm_property_write_shutdown_time(guchar *data, guchar length);
static void     ctrlm_property_read_shutdown_time(void);
static void     control_service_values_read_from_db();
static void     ctrlm_check_for_key_tag(int key_tag);

#ifdef MEMORY_LOCK
const char *memory_lock_progs[] = {
"/usr/bin/controlMgr",
"/usr/lib/libctrlm_hal_rf4ce.so.0.0.0",
"/usr/lib/libxraudio.so.0.0.0",
"/usr/lib/libxraudio-hal.so.0.0.0",
"/usr/lib/libxr_mq.so.0.0.0",
"/usr/lib/libxr-timer.so.0.0.0",
"/usr/lib/libxr-timestamp.so.0.0.0"
};
#endif

#if CTRLM_HAL_RF4CE_API_VERSION >= 9
static void ctrlm_crash_recovery_check();
static void ctrlm_backup_data();
#endif

static void ctrlm_ir_controller_thread_poll(void *data);
static void ctrlm_vsdk_thread_poll(void *data);
static void ctrlm_vsdk_thread_response(void *data);

#ifdef MEM_DEBUG
static gboolean ctrlm_memory_profile(gpointer user_data) {
   g_mem_profile();
   return(TRUE);
}
#endif

#ifdef BREAKPAD_SUPPORT
static bool ctrlm_minidump_callback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) {
  XLOGD_FATAL("Minidump location: %s Status: %s", descriptor.path(), succeeded ? "SUCCEEDED" : "FAILED");
  return succeeded;
}
#endif


gboolean ctrlm_is_production_build(void) {
   return(g_ctrlm.production_build);
}

int main(int argc, char *argv[]) {
   // Set stdout to be line buffered
   setvbuf(stdout, NULL, _IOLBF, 0);

   XLOGD_INFO("name <%-24s> version <%-7s> branch <%-20s> commit <%s>", "ctrlm-main", CTRLM_MAIN_VERSION, CTRLM_MAIN_BRANCH, CTRLM_MAIN_COMMIT_ID);

#ifdef MEMORY_LOCK
   clnl_init();
   for(unsigned int i = 0; i < (sizeof(memory_lock_progs) / sizeof(memory_lock_progs[0])); i++) {
      if(ctrlm_file_exists(memory_lock_progs[i])) {
         if(clnl_lock(memory_lock_progs[i], SECTION_TEXT)) {
            XLOGD_ERROR("failed to lock instructions to memory <%s>", memory_lock_progs[i]);
         } else {
            XLOGD_INFO("successfully locked to memory <%s>", memory_lock_progs[i]);
         }
      } else {
         XLOGD_DEBUG("file doesn't exist, cannot lock to memory <%s>", memory_lock_progs[i]);
      }
   }
#endif

   ctrlm_signals_register();

#ifdef BREAKPAD_SUPPORT
   std::string minidump_path = "/opt/minidumps";
   #ifdef BREAKPAD_MINIDUMP_PATH_OVERRIDE
   minidump_path = BREAKPAD_MINIDUMP_PATH_OVERRIDE;
   #else
   FILE *fp= NULL;;
   if(( fp = fopen("/tmp/.SecureDumpEnable", "r")) != NULL) {
        minidump_path = "/opt/secure/minidumps";
        fclose(fp);
   }
   #endif

   google_breakpad::MinidumpDescriptor descriptor(minidump_path.c_str());
   google_breakpad::ExceptionHandler eh(descriptor, NULL, ctrlm_minidump_callback, NULL, true, -1);

   //ctrlm_crash();
#endif

   XLOGD_INFO("glib     run-time version... %d.%d.%d", glib_major_version, glib_minor_version, glib_micro_version);
   XLOGD_INFO("glib compile-time version... %d.%d.%d", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
   const char *error = glib_check_version(GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);

   if(NULL != error) {
      XLOGD_FATAL("Glib not compatible: %s", error);
      return(-1);
   } else {
      XLOGD_INFO("Glib run-time library is compatible");
   }

#ifdef MEM_DEBUG
   XLOGD_WARN("Memory debug is ENABLED.");
   g_mem_set_vtable(glib_mem_profiler_table);
#endif

   sem_init(&g_ctrlm.ctrlm_utils_sem, 0, 1);

   if(!ctrlm_iarm_init()) {
      XLOGD_FATAL("Unable to initialize IARM bus.");
      // TODO handle this failure such that it retries
      return(-1);
   }

   vsdk_version_info_t version_info[VSDK_VERSION_QTY_MAX] = {0};

   uint32_t qty_vsdk = VSDK_VERSION_QTY_MAX;
   vsdk_version(version_info, &qty_vsdk);

   for(uint32_t index = 0; index < qty_vsdk; index++) {
      vsdk_version_info_t *entry = &version_info[index];
      if(entry->name != NULL) {
         XLOGD_INFO("name <%-24s> version <%-7s> branch <%-20s> commit <%s>", entry->name ? entry->name : "NULL", entry->version ? entry->version : "NULL", entry->branch ? entry->branch : "NULL", entry->commit_id ? entry->commit_id : "NULL");
      }
   }
   vsdk_init();

   //struct sched_param param;
   //param.sched_priority = 10;
   //if(0 != sched_setscheduler(0, SCHED_FIFO, &param)) {
   //   XLOGD_ERROR("Unable to set scheduler priority!");
   //}

   // Initialize control manager global structure
   g_ctrlm.main_loop                      = g_main_loop_new(NULL, true);
   g_ctrlm.main_thread                    = NULL;
   g_ctrlm.queue                          = NULL;
   g_ctrlm.production_build               = true;
   g_ctrlm.has_service_access_token       = false;
   g_ctrlm.sat_enabled                    = true;
   g_ctrlm.service_access_token_expiration_tag = 0;
   g_ctrlm.thread_monitor_timeout_val     = JSON_INT_VALUE_CTRLM_GLOBAL_THREAD_MONITOR_PERIOD * 1000;
   g_ctrlm.thread_monitor_index           = 0;
   g_ctrlm.thread_monitor_active          = true;
   g_ctrlm.thread_monitor_minidump        = JSON_BOOL_VALUE_CTRLM_GLOBAL_THREAD_MONITOR_MINIDUMP;
   g_ctrlm.server_url_authservice         = JSON_STR_VALUE_CTRLM_GLOBAL_URL_AUTH_SERVICE;
   g_ctrlm.authservice_poll_val           = JSON_INT_VALUE_CTRLM_GLOBAL_AUTHSERVICE_POLL_PERIOD * 1000;
   g_ctrlm.authservice_fast_poll_val      = JSON_INT_VALUE_CTRLM_GLOBAL_AUTHSERVICE_FAST_POLL_PERIOD;
   g_ctrlm.authservice_fast_retries       = 0;
   g_ctrlm.authservice_fast_retries_max   = JSON_INT_VALUE_CTRLM_GLOBAL_AUTHSERVICE_FAST_MAX_RETRIES;
   g_ctrlm.bound_controller_qty           = 0;
   g_ctrlm.recently_booted                = FALSE;
   g_ctrlm.recently_booted_timeout_val    = JSON_INT_VALUE_CTRLM_GLOBAL_TIMEOUT_RECENTLY_BOOTED;
   g_ctrlm.recently_booted_timeout_tag    = 0;
   g_ctrlm.line_of_sight                  = FALSE;
   g_ctrlm.line_of_sight_timeout_val      = JSON_INT_VALUE_CTRLM_GLOBAL_TIMEOUT_LINE_OF_SIGHT;
   g_ctrlm.line_of_sight_timeout_tag      = 0;
   g_ctrlm.autobind                       = FALSE;
   g_ctrlm.autobind_timeout_val           = JSON_INT_VALUE_CTRLM_GLOBAL_TIMEOUT_AUTOBIND;
   g_ctrlm.autobind_timeout_tag           = 0;
   g_ctrlm.binding_button                 = FALSE;
   g_ctrlm.binding_button_timeout_val     = JSON_INT_VALUE_CTRLM_GLOBAL_TIMEOUT_BUTTON_BINDING;
   g_ctrlm.binding_button_timeout_tag     = 0;
   g_ctrlm.binding_screen_active          = FALSE;
   g_ctrlm.screen_bind_timeout_val        = JSON_INT_VALUE_CTRLM_GLOBAL_TIMEOUT_SCREEN_BIND;
   g_ctrlm.screen_bind_timeout_tag        = 0;
   g_ctrlm.one_touch_autobind_timeout_val = JSON_INT_VALUE_CTRLM_GLOBAL_TIMEOUT_ONE_TOUCH_AUTOBIND;
   g_ctrlm.one_touch_autobind_timeout_tag = 0;
   g_ctrlm.one_touch_autobind_active      = FALSE;
   g_ctrlm.pairing_window.active          = FALSE;
   g_ctrlm.pairing_window.bind_status     = CTRLM_BIND_STATUS_NO_DISCOVERY_REQUEST;
   g_ctrlm.pairing_window.restrict_by_remote = CTRLM_PAIRING_RESTRICT_NONE;
   g_ctrlm.mask_pii                       = true;
   g_ctrlm.db_path                        = JSON_STR_VALUE_CTRLM_GLOBAL_DB_PATH;
   g_ctrlm.minidump_path                  = JSON_STR_VALUE_CTRLM_GLOBAL_MINIDUMP_PATH;
   g_ctrlm.crash_recovery_threshold       = JSON_INT_VALUE_CTRLM_GLOBAL_CRASH_RECOVERY_THRESHOLD;
   g_ctrlm.successful_init                = FALSE;
   //g_ctrlm.precomission_table             = g_hash_table_new(g_str_hash, g_str_equal);
   g_ctrlm.loading_db                     = false;
   g_ctrlm.return_code                    = 0;
   g_ctrlm.power_state                    = ctrlm_main_iarm_call_get_power_state();
   g_ctrlm.auto_ack                       = true;
   g_ctrlm.local_conf                     = false;
   g_ctrlm.telemetry                      = NULL;
   g_ctrlm.telemetry_report_interval      = JSON_INT_VALUE_CTRLM_GLOBAL_TELEMETRY_REPORT_INTERVAL;
   g_ctrlm.service_access_token.clear();
   g_ctrlm.has_receiver_id                = false;
   g_ctrlm.has_device_id                  = false;
   g_ctrlm.has_device_type                = false;
   g_ctrlm.has_service_account_id         = false;
   g_ctrlm.has_partner_id                 = false;
   g_ctrlm.has_experience                 = false;
   g_ctrlm.has_service_access_token       = false;
   sem_init(&g_ctrlm.service_access_token_semaphore, 0, 1);

   g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_UNKNOWN;
   g_ctrlm.last_key_info.is_screen_bind_mode  = false;
   g_ctrlm.last_key_info.remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_INVALID;
#ifdef DEEP_SLEEP_ENABLED
   g_ctrlm.wake_with_voice_allowed            = false;
#endif
   errno_t safec_rc = strcpy_s(g_ctrlm.last_key_info.source_name, sizeof(g_ctrlm.last_key_info.source_name), ctrlm_rcu_ir_remote_types_str(g_ctrlm.last_key_info.last_ir_remote_type));
   ERR_CHK(safec_rc);

   g_ctrlm.discovery_remote_type[0] = '\0';

#ifdef MEM_DEBUG
   g_mem_profile();
   g_timeout_add_seconds(10, ctrlm_memory_profile, NULL);
#endif

   XLOGD_INFO("load version");
   if(!ctrlm_load_version()) {
      XLOGD_FATAL("failed to load version");
      return(-1);
   }

   // Launch a thread to handle DB writes asynchronously
   // Create an asynchronous queue to receive incoming messages from the networks
   g_ctrlm.queue = g_async_queue_new_full(ctrlm_queue_msg_destroy);

   g_ctrlm.mask_pii = ctrlm_is_production_build() ? JSON_ARRAY_VAL_BOOL_CTRLM_GLOBAL_MASK_PII_0 : JSON_ARRAY_VAL_BOOL_CTRLM_GLOBAL_MASK_PII_1;

#ifdef TELEMETRY_SUPPORT
   XLOGD_INFO("create Telemetry object");
   g_ctrlm.telemetry = ctrlm_telemetry_t::get_instance();
#endif

   XLOGD_INFO("create Voice object");
   g_ctrlm.voice_session = new ctrlm_voice_generic_t();

   XLOGD_INFO("ctrlm_rfc init");
   // This tells the RFC component to go fetch all the enabled attributes once
   // we are fully initialized. This brings us to parity with the current RFC/TR181
   // implementation.
   ctrlm_rfc_t *rfc = ctrlm_rfc_t::get_instance();
   if(rfc) {
      rfc->add_changed_listener(ctrlm_rfc_t::attrs::GLOBAL, &ctrlm_global_rfc_values_retrieved);
   }
   // TODO: We could possibly schedule this to run once every few hours or whatever
   //       the team decides is best.

   // Check if recently booted
   struct sysinfo s_info;
   if(sysinfo(&s_info) != 0) {
      XLOGD_ERROR("Unable to get system uptime");
   } else {
      XLOGD_INFO("System up for %lu seconds", s_info.uptime);
      if(s_info.uptime < CTRLM_MAIN_FIRST_BOOT_TIME_MAX) { // System first boot
         XLOGD_TELEMETRY("ctrlm_main: System first boot");
      }
      if(s_info.uptime < (long)(g_ctrlm.recently_booted_timeout_val / 1000)) { // System just booted
         XLOGD_INFO("Setting recently booted to true");
         g_ctrlm.recently_booted = TRUE;
      }
   }

   XLOGD_INFO("load config");
   // Load configuration from the filesystem
   json_t *json_obj_root, *json_obj_net_rf4ce, *json_obj_voice, *json_obj_device_update, *json_obj_validation, *json_obj_vsdk;
   json_obj_root          = NULL;
   json_obj_net_rf4ce     = NULL;
   json_obj_voice         = NULL;
   json_obj_device_update = NULL;
   json_obj_validation    = NULL;
   json_obj_vsdk          = NULL;
   ctrlm_load_config(&json_obj_root, &json_obj_net_rf4ce, &json_obj_voice, &json_obj_device_update, &json_obj_validation, &json_obj_vsdk);

#ifdef TELEMETRY_SUPPORT
   // set telemetry duration after config parsing
   g_ctrlm.telemetry->set_duration(g_ctrlm.telemetry_report_interval);
#endif

   XLOGD_INFO("load device mac");
   if(!ctrlm_load_device_mac()) {
      XLOGD_ERROR("failed to load device mac");
      // Do not crash the program here
   }

   XLOGD_INFO("load device type");
   if(!ctrlm_load_device_type()) {
      XLOGD_ERROR("failed to load device type");
   }

#ifdef AUTH_ENABLED
   XLOGD_INFO("ctrlm_auth init");
   g_ctrlm.authservice = ctrlm_auth_service_create(g_ctrlm.server_url_authservice);

   ctrlm_voice_cert_t device_cert;
   bool ocsp_verify_stapling = false;
   bool ocsp_verify_ca       = false;

   if(!g_ctrlm.authservice->device_cert_get(device_cert, ocsp_verify_stapling, ocsp_verify_ca)) {
      XLOGD_ERROR("unable to get device certificate");
   } else {
      if(!g_ctrlm.voice_session->voice_stb_data_device_certificate_set(device_cert, ocsp_verify_stapling, ocsp_verify_ca)) {
         XLOGD_ERROR("unable to set device certificate");
      }
   }

   if (!ctrlm_has_authservice_data()) {
      if (!g_ctrlm.authservice->is_ready()) {
         XLOGD_WARN("Authservice not ready, no reason to poll");
      } else {
         if (!ctrlm_load_authservice_data()) {
            XLOGD_INFO("Starting polling authservice for device data");
            g_ctrlm.authservice_poll_tag = ctrlm_timeout_create(g_ctrlm.recently_booted ? g_ctrlm.authservice_fast_poll_val : g_ctrlm.authservice_poll_val,
                                                                ctrlm_authservice_poll,
                                                                NULL);
            if (!g_ctrlm.recently_booted) {
               g_ctrlm.authservice_fast_retries = g_ctrlm.authservice_fast_retries_max;
            }
         }
      }
   }
#endif // AUTH_ENABLED

   g_ctrlm.voice_session->voice_stb_data_stb_name_set(g_ctrlm.stb_name);
   g_ctrlm.voice_session->voice_stb_data_pii_mask_set(g_ctrlm.mask_pii);

#if defined(BREAKPAD_SUPPORT) && !defined(BREAKPAD_MINIDUMP_PATH_OVERRIDE)
   if(g_ctrlm.minidump_path != JSON_STR_VALUE_CTRLM_GLOBAL_MINIDUMP_PATH) {
      google_breakpad::MinidumpDescriptor descriptor_json(g_ctrlm.minidump_path.c_str());
      eh.set_minidump_descriptor(descriptor_json);
   }
#endif

   // Database init must occur after network qty and type are known
   XLOGD_INFO("pre-init networks");
   ctrlm_networks_pre_init(json_obj_net_rf4ce, json_obj_root);

   XLOGD_INFO("init recovery");
   ctrlm_recovery_init();

#if CTRLM_HAL_RF4CE_API_VERSION >= 9
   // Check to see if we are recovering from a crash before init
   ctrlm_crash_recovery_check();
#endif

   XLOGD_INFO("init database");
   if(!ctrlm_db_init(g_ctrlm.db_path.c_str())) {
#if CTRLM_HAL_RF4CE_API_VERSION >= 9
      uint32_t invalid_db = 1;
      ctrlm_recovery_property_set(CTRLM_RECOVERY_INVALID_CTRLM_DB, &invalid_db);
      return(-1);
#endif
   }

   g_ctrlm.loading_db = true;
   //Read shutdown time data from the db
   ctrlm_property_read_shutdown_time();
   //Read IR remote usage data from the db
   ctrlm_property_read_ir_remote_usage();
   //Read pairing metrics data from the db
   ctrlm_property_read_pairing_metrics();
   //Read last key info from the db
   ctrlm_property_read_last_key_info();
   g_ctrlm.loading_db = false;

   // This needs to happen after the DB is init, but before voice
   if(TRUE == g_ctrlm.recently_booted) {
      g_ctrlm.recently_booted_timeout_tag   = ctrlm_timeout_create(g_ctrlm.recently_booted_timeout_val - (s_info.uptime * 1000), ctrlm_timeout_recently_booted, NULL);
   }

   ctrlm_timeout_create(CTRLM_RESTART_UPDATE_TIMEOUT, ctrlm_timeout_systemd_restart_delay, NULL);

   XLOGD_INFO("init validation");
   ctrlm_validation_init(json_obj_validation);

   XLOGD_INFO("init rcu");
   ctrlm_rcu_init();

   // Device update components needs to be initialized after controllers are loaded
   XLOGD_INFO("init device update");
   ctrlm_device_update_init(json_obj_device_update);

   // Initialize semaphore
   sem_init(&g_ctrlm.semaphore, 0, 0);

   g_ctrlm.main_thread = g_thread_new("ctrlm_main", ctrlm_main_thread, NULL);

   // Block until initialization is complete or a timeout occurs
   XLOGD_INFO("Waiting for ctrlm main thread initialization...");
   sem_wait(&g_ctrlm.semaphore);

   XLOGD_INFO("init IR controller object");
   g_ctrlm.ir_controller = ctrlm_ir_controller_t::get_instance();
   g_ctrlm.ir_controller->mask_key_codes_set(g_ctrlm.mask_pii);
   string ir_db_table;
   ctrlm_db_ir_controller_create(ir_db_table);
   g_ctrlm.ir_controller->db_table_set(ir_db_table);
   g_ctrlm.ir_controller->db_load();
   g_ctrlm.ir_controller->print_status();

   XLOGD_INFO("init networks");
   if(!ctrlm_networks_init()) {
      XLOGD_FATAL("Unable to initialize networks");
      return(-1);
   }

   XLOGD_INFO("init voice");
   g_ctrlm.voice_session->voice_configure_config_file_json(json_obj_voice, json_obj_vsdk, g_ctrlm.local_conf );
   XLOGD_INFO("networks init complete");

#if CTRLM_HAL_RF4CE_API_VERSION >= 9
   // Init was successful, create backups of all NVM files
   ctrlm_backup_data();
   // Terminate recovery component and reset all values
   ctrlm_recovery_terminate(true);
#endif

   if(json_obj_root) {
      json_decref(json_obj_root);
   }

   //export device list for all devices on all networks for xconf update checks
   XLOGD_INFO("init xconf device list");
   ctrlm_main_update_export_controller_list();

   // Thread monitor
   ctrlm_thread_monitor_init();

   g_ctrlm.successful_init = TRUE;

   ctrlm_trigger_startup_actions();

   XLOGD_INFO("Enter main loop");
   g_main_loop_run(g_ctrlm.main_loop);

   //Save the shutdown time if it is valid
   time_t current_time = time(NULL);
   if(g_ctrlm.shutdown_time < current_time) {
      g_ctrlm.shutdown_time = current_time;
      ctrlm_property_write_shutdown_time();
   } else {
      XLOGD_WARN("Current Time <%ld> is less than the last shutdown time <%ld>.  Ignoring.", current_time, g_ctrlm.shutdown_time);
   }

   XLOGD_INFO("main loop exited");
   ctrlm_terminate();

   ctrlm_dsmgr_deinit();

   if(g_ctrlm.voice_session != NULL) {
      delete g_ctrlm.voice_session;
      g_ctrlm.voice_session = NULL;
   }

   vsdk_term();

   #ifdef IRDB_ENABLED
   if(g_ctrlm.irdb != NULL) {
      delete g_ctrlm.irdb;
      g_ctrlm.irdb = NULL;
   }
   #endif

   sem_destroy(&g_ctrlm.service_access_token_semaphore);

#if AUTH_ENABLED
   if(g_ctrlm.authservice != NULL) {
      delete g_ctrlm.authservice;
      g_ctrlm.authservice = NULL;
   }
#endif

   sem_destroy(&g_ctrlm.ctrlm_utils_sem);

#ifdef MEMORY_LOCK
   for(unsigned int i = 0; i < (sizeof(memory_lock_progs) / sizeof(memory_lock_progs[0])); i++) {
      if(ctrlm_file_exists(memory_lock_progs[i])) {
         clnl_unlock(memory_lock_progs[i], SECTION_TEXT);
      }
   }
   clnl_destroy();
#endif

   ctrlm_config_t::destroy_instance();
   ctrlm_rfc_t::destroy_instance();
   #ifdef TELEMETRY_SUPPORT
   ctrlm_telemetry_t::destroy_instance();
   #endif
   ctrlm_ir_controller_t::destroy_instance();

   XLOGD_INFO("exit program");
   return (g_ctrlm.return_code);
}

std::string ctrlm_main_ir_controller_name_get() {
   return g_ctrlm.ir_controller->name_get();
}

void ctrlm_main_ir_last_keypress_get(ctrlm_ir_last_keypress_t *last_key_info) {
   if (g_ctrlm.main_thread != g_thread_self ()) {
      XLOGD_ERROR("not called from ctrlm_main_thread!!!!!");
      if(!ctrlm_is_production_build()) {
         g_assert(0);
      }
   } else {
      last_key_info->last_key_time = g_ctrlm.ir_controller->last_key_time_get();
      last_key_info->last_key_code = g_ctrlm.ir_controller->last_key_code_get();
   }
}

void ctrlm_utils_sem_wait(){
   sem_wait(&g_ctrlm.ctrlm_utils_sem);
}

void ctrlm_utils_sem_post(){
   sem_post(&g_ctrlm.ctrlm_utils_sem);
}

void ctrlm_thread_monitor_init(void) {
   ctrlm_thread_monitor_t thread_monitor;

   g_ctrlm.thread_monitor_timeout_tag = ctrlm_timeout_create(g_ctrlm.thread_monitor_timeout_val, ctrlm_thread_monitor, NULL);

   thread_monitor.name        = CTRLM_THREAD_NAME_MAIN;
   thread_monitor.queue_push  = ctrlm_main_queue_msg_push;
   thread_monitor.obj_network = NULL;
   thread_monitor.function    = NULL;
   thread_monitor.response    = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
   g_ctrlm.monitor_threads.push_back(thread_monitor);

   thread_monitor.name        = CTRLM_THREAD_NAME_DATABASE;
   thread_monitor.queue_push  = ctrlm_db_queue_msg_push_front;
   thread_monitor.obj_network = NULL;
   thread_monitor.function    = NULL;
   thread_monitor.response    = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
   g_ctrlm.monitor_threads.push_back(thread_monitor);

   thread_monitor.name        = CTRLM_THREAD_NAME_DEVICE_UPDATE;
   thread_monitor.queue_push  = ctrlm_device_update_queue_msg_push;
   thread_monitor.obj_network = NULL;
   thread_monitor.function    = NULL;
   thread_monitor.response    = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
   g_ctrlm.monitor_threads.push_back(thread_monitor);

   thread_monitor.name        = CTRLM_THREAD_NAME_VOICE_SDK;
   thread_monitor.queue_push  = NULL;
   thread_monitor.obj_network = NULL;
   thread_monitor.function    = ctrlm_vsdk_thread_poll;
   thread_monitor.response    = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
   g_ctrlm.monitor_threads.push_back(thread_monitor);

   thread_monitor.name        = CTRLM_THREAD_NAME_IR_CONTROLLER;
   thread_monitor.queue_push  = NULL;
   thread_monitor.obj_network = NULL;
   thread_monitor.function    = ctrlm_ir_controller_thread_poll;
   thread_monitor.response    = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
   g_ctrlm.monitor_threads.push_back(thread_monitor);

   for(auto const &itr : g_ctrlm.networks) {
      if(CTRLM_HAL_RESULT_SUCCESS == itr.second->init_result_) {
         thread_monitor.name        = itr.second->name_get();
         thread_monitor.queue_push  = NULL;
         thread_monitor.obj_network = itr.second;
         thread_monitor.function    = NULL;
         thread_monitor.response    = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
         g_ctrlm.monitor_threads.push_back(thread_monitor);
      }
   }

   g_ctrlm.monitor_threads.shrink_to_fit();

   if(CTRLM_TR181_RESULT_SUCCESS != ctrlm_tr181_bool_get(CTRLM_RF4CE_TR181_THREAD_MONITOR_MINIDUMP_ENABLE, &g_ctrlm.thread_monitor_minidump)) {
      XLOGD_INFO("Thread Monitor Minidump is <%s> (TR181 not present)", (g_ctrlm.thread_monitor_minidump ? "ENABLED" : "DISABLED"));
   } else {
      XLOGD_INFO("Thread Monitor Minidump is <%s>", (g_ctrlm.thread_monitor_minidump ? "ENABLED" : "DISABLED"));
   }

   // Run once to kick off the first poll
   ctrlm_thread_monitor(NULL);
}

gboolean ctrlm_thread_monitor(gpointer user_data) {
   if(g_ctrlm.thread_monitor_index < (60 * 60 * 1000)) { // One hour in milliseconds
      XLOG_RAW("."); XLOG_FLUSH();
      g_ctrlm.thread_monitor_index += g_ctrlm.thread_monitor_timeout_val;
   } else {
      XLOG_RAW("\n");
      XLOGD_NO_LF(XLOG_LEVEL_INFO, "."); XLOG_FLUSH();
      g_ctrlm.thread_monitor_index = 0;
   }

   #ifdef FDC_ENABLED
   uint32_t limit_soft = 40;
   uint32_t limit_hard = FD_SETSIZE - 20;
   int32_t rc = xr_fdc_check(limit_soft, limit_hard, true);
   if(rc < 0) {
      XLOGD_ERROR("xr_fdc_check");
   } else if(rc > 0) {
      XLOGD_FATAL("xr_fdc_check hard limit");
      ctrlm_quit_main_loop();
      return(FALSE);
   }
   #endif

   if(ctrlm_was_cpu_halted()) {
      XLOGD_INFO("skipping response check due to power state <%s>",ctrlm_power_state_str(g_ctrlm.power_state));
      g_ctrlm.thread_monitor_active = false; // Deactivate thread monitoring
   } else if(!g_ctrlm.thread_monitor_active) {
      XLOGD_INFO("activate due to power state <%s>",ctrlm_power_state_str(g_ctrlm.power_state));
      g_ctrlm.thread_monitor_active = true;  // Activate thread monitoring again
   } else {
      // Check the response from each thread on the previous attempt
      for(vector<ctrlm_thread_monitor_t>::iterator it = g_ctrlm.monitor_threads.begin(); it != g_ctrlm.monitor_threads.end(); it++) {
         XLOGD_DEBUG("Checking %s", it->name);

         if(it->response != CTRLM_THREAD_MONITOR_RESPONSE_ALIVE) {
            XLOGD_TELEMETRY("Thread %s is unresponsive", it->name);
            #ifdef BREAKPAD_SUPPORT
            if(g_ctrlm.thread_monitor_minidump) {
               XLOGD_FATAL("Thread Monitor Minidump is enabled");

               if(       0 == strncmp(it->name, CTRLM_THREAD_NAME_MAIN,          sizeof(CTRLM_THREAD_NAME_MAIN))) {
                  ctrlm_crash_ctrlm_main();
               } else if(0 == strncmp(it->name, CTRLM_THREAD_NAME_VOICE_SDK,     sizeof(CTRLM_THREAD_NAME_VOICE_SDK))) {
                  ctrlm_crash_vsdk();
               } else if(0 == strncmp(it->name, CTRLM_THREAD_NAME_RF4CE,         sizeof(CTRLM_THREAD_NAME_RF4CE))) {
                  #ifdef CTRLM_RF4CE_HAL_QORVO
                  ctrlm_crash_rf4ce_qorvo();
                  #else
                  ctrlm_crash_rf4ce_ti();
                  #endif
               } else if(0 == strncmp(it->name, CTRLM_THREAD_NAME_BLE,           sizeof(CTRLM_THREAD_NAME_BLE))) {
                  ctrlm_crash_ble();
               } else if(0 == strncmp(it->name, CTRLM_THREAD_NAME_DATABASE,      sizeof(CTRLM_THREAD_NAME_DATABASE))) {
                  ctrlm_crash_ctrlm_database();
               } else if(0 == strncmp(it->name, CTRLM_THREAD_NAME_DEVICE_UPDATE, sizeof(CTRLM_THREAD_NAME_DEVICE_UPDATE))) {
                  ctrlm_crash_ctrlm_device_update();
               } else {
                  ctrlm_crash();
               }
            }
            #endif
            ctrlm_quit_main_loop();
            return (FALSE);
         }
      }
   }

   if(g_ctrlm.thread_monitor_active) { // Thread monitoring is active
      // Send a message to each thread to respond
      for(vector<ctrlm_thread_monitor_t>::iterator it = g_ctrlm.monitor_threads.begin(); it != g_ctrlm.monitor_threads.end(); it++) {
         XLOGD_DEBUG("Sending %s", it->name);
         it->response = CTRLM_THREAD_MONITOR_RESPONSE_DEAD;

         if(it->queue_push != NULL) { // Allocate a message and send it to the thread's queue
            ctrlm_thread_monitor_msg_t *msg = (ctrlm_thread_monitor_msg_t *)g_malloc(sizeof(ctrlm_thread_monitor_msg_t));

            if(NULL == msg) {
               XLOGD_FATAL("Out of memory");
               g_assert(0);
               ctrlm_quit_main_loop();
               return(FALSE);
            }
            msg->type     = CTRLM_MAIN_QUEUE_MSG_TYPE_TICKLE;
            msg->response = &it->response;

            (*(it->queue_push))((gpointer)msg);
         } else if(it->obj_network != NULL){
            ctrlm_thread_monitor_msg_t *msg = (ctrlm_thread_monitor_msg_t *)g_malloc(sizeof(ctrlm_thread_monitor_msg_t));

            if(NULL == msg) {
               XLOGD_FATAL("Out of memory");
               g_assert(0);
               ctrlm_quit_main_loop();
               return(FALSE);
            }
            msg->type        = CTRLM_MAIN_QUEUE_MSG_TYPE_THREAD_MONITOR_POLL;
            msg->obj_network = it->obj_network;
            msg->response    = &it->response;
            ctrlm_main_queue_msg_push(msg);
         } else if(it->function != NULL) {
            (*it->function)(&it->response);
         }
      }
   }

   return(TRUE);
}

// Returns true if the CPU was halted based on the current power state
gboolean ctrlm_was_cpu_halted(void) {
   if(g_ctrlm.power_state == CTRLM_POWER_STATE_DEEP_SLEEP) {
      return(TRUE);
   }
   return(FALSE);
}

void ctrlm_vsdk_thread_response(void *data) {
   ctrlm_thread_monitor_response_t *response = (ctrlm_thread_monitor_response_t *)data;
   if(response != NULL) {
      *response = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
   }
}

void ctrlm_vsdk_thread_poll(void *data) {
   vsdk_thread_poll(ctrlm_vsdk_thread_response, data);
}

void ctrlm_ir_controller_thread_poll(void *data) {
   g_ctrlm.ir_controller->thread_poll(data);
}

#ifdef AUTH_ENABLED
#ifdef AUTH_SAT_TOKEN
static gboolean ctrlm_authservice_expired(gpointer user_data) {
   XLOGD_TELEMETRY("SAT Token is expired...");
   ctrlm_main_invalidate_service_access_token();
   return(FALSE);
}
#endif

gboolean ctrlm_authservice_poll(gpointer user_data) {
   ctrlm_main_queue_msg_authservice_poll_t *msg = (ctrlm_main_queue_msg_authservice_poll_t *)g_malloc(sizeof(ctrlm_main_queue_msg_authservice_poll_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      ctrlm_quit_main_loop();
      ctrlm_timeout_destroy(&g_ctrlm.authservice_poll_tag);
      return(false);
   }

   msg->type = CTRLM_MAIN_QUEUE_MSG_TYPE_AUTHSERVICE_POLL;
   ctrlm_main_queue_msg_push(msg);

   return(FALSE);
}
#endif

void ctrlm_signals_register(void) {
   // Handle these signals
   XLOGD_INFO("Registering SIGINT...");
   if(signal(SIGINT, ctrlm_signal_handler) == SIG_ERR) {
      XLOGD_ERROR("Unable to register for SIGINT.");
   }
   XLOGD_INFO("Registering SIGTERM...");
   if(signal(SIGTERM, ctrlm_signal_handler) == SIG_ERR) {
      XLOGD_ERROR("Unable to register for SIGTERM.");
   }
   XLOGD_INFO("Registering SIGQUIT...");
   if(signal(SIGQUIT, ctrlm_signal_handler) == SIG_ERR) {
      XLOGD_ERROR("Unable to register for SIGQUIT.");
   }
   XLOGD_INFO("Registering SIGPIPE...");
   if(signal(SIGPIPE, ctrlm_signal_handler) == SIG_ERR) {
      XLOGD_ERROR("Unable to register for SIGPIPE.");
   }
}

void ctrlm_signal_handler(int signal) {
   switch(signal) {
      case SIGTERM:
      case SIGINT: {
         XLOGD_INFO("Received %s", signal == SIGINT ? "SIGINT" : "SIGTERM");
         ctrlm_quit_main_loop();
         break;
      }
      case SIGQUIT: {
         XLOGD_INFO("Received SIGQUIT");
#ifdef BREAKPAD_SUPPORT
         ctrlm_crash();
#endif
         break;
      }
      case SIGPIPE: {
         XLOGD_ERROR("Received SIGPIPE. Pipe is broken");
         break;
      }
      default:
         XLOGD_ERROR("Received unhandled signal %d", signal);
         break;
   }
}

void ctrlm_quit_main_loop() {
   if (g_main_loop_is_running(g_ctrlm.main_loop)) {
      g_main_loop_quit(g_ctrlm.main_loop);
   }
}


void ctrlm_terminate(void) {
   XLOGD_INFO("IARM");
   ctrlm_main_iarm_terminate();

   XLOGD_INFO("Main");
   // Now clean up control manager main
   if(g_ctrlm.main_thread != NULL) {
      ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));
      if(msg == NULL) {
         XLOGD_ERROR("Out of memory");
      } else {
         struct timespec end_time;
         clock_gettime(CLOCK_REALTIME, &end_time);
         end_time.tv_sec += 5;
         msg->type = CTRLM_MAIN_QUEUE_MSG_TYPE_TERMINATE;

         ctrlm_main_queue_msg_push((gpointer)msg);

         // Block until termination is acknowledged or a timeout occurs
         XLOGD_INFO("Waiting for main thread termination...");
         int acknowledged = sem_timedwait(&g_ctrlm.semaphore, &end_time); 

         if(acknowledged==-1) { // no response received
            XLOGD_INFO("Do NOT wait for thread to exit");
         } else {
            // Wait for thread to exit
            XLOGD_INFO("Waiting for thread to exit");
            g_thread_join(g_ctrlm.main_thread);
            XLOGD_INFO("thread exited.");
         }
      }
   }

   XLOGD_INFO("Recovery");
   ctrlm_recovery_terminate(false);

   XLOGD_INFO("Validation");
   ctrlm_validation_terminate();

   XLOGD_INFO("Rcu");
   ctrlm_rcu_terminate();

   XLOGD_INFO("Device Update");
   ctrlm_device_update_terminate();

   XLOGD_INFO("Networks");
   ctrlm_networks_terminate();

   XLOGD_INFO("Database");
   ctrlm_db_terminate();

   XLOGD_INFO("IARM");
   ctrlm_iarm_terminate();
}

#if CTRLM_HAL_RF4CE_API_VERSION >= 15
void ctrlm_on_network_assert(ctrlm_network_id_t network_id, const char* assert_info) {
   XLOGD_INFO("Assert \'%s\' on %s(%u) network.", assert_info, ctrlm_network_type_str(ctrlm_network_type_get(network_id)).c_str(), network_id);
   if (CTRLM_NETWORK_TYPE_RF4CE == ctrlm_network_type_get(network_id)) {
      ctrlm_obj_network_t* net = NULL;
      if(g_ctrlm.networks.count(network_id) > 0) {
         net = g_ctrlm.networks[network_id];
         net->analyze_assert_reason(assert_info);
      }

      // RDK HW test requests ctrlm via IARM to check rf4ce state
      // keep ctrlm on if rf4ce chip is in failed state.
      if (net == NULL || !net->is_failed_state()) {
         ctrlm_on_network_assert(network_id);
      }
   }
}
#endif

void ctrlm_on_network_assert(ctrlm_network_id_t network_id) {
   XLOGD_TELEMETRY("Assert on network %u. Terminating...", network_id);
   if(g_ctrlm.networks.count(network_id) > 0) {
      g_ctrlm.networks[network_id]->disable_hal_calls();
   }
   if (g_ctrlm.main_thread == g_thread_self ()) {
       // Invalidate main thread so terminate does not attempt to terminate it
       g_ctrlm.main_thread = NULL;
   }
   // g_main_loop_quit() will be called in ctrlm_signal_handler(SIGTERM)
   g_ctrlm.return_code = -1;
   ctrlm_signal_handler(SIGTERM);
   // give main() time to clean up
   sleep(5);
   // Exit here in case main fails to exit
   exit(-1);
}

gboolean ctrlm_load_version(void) {
   rdk_version_info_t info;
   int ret_val = rdk_version_parse_version(&info);

   if(ret_val != 0) {
      XLOGD_ERROR("parse error <%s>", info.parse_error == NULL ? "" : info.parse_error);
   } else {
      g_ctrlm.image_name       = info.image_name;
      g_ctrlm.stb_name         = info.stb_name;
      g_ctrlm.image_branch     = info.branch_name;
      g_ctrlm.image_version    = info.version_name;
      g_ctrlm.image_build_time = info.image_build_time;
      g_ctrlm.production_build = info.production_build;

      XLOGD_INFO("STB Name <%s> Image Type <%s> Version <%s> Branch <%s> Build Time <%s>", g_ctrlm.stb_name.c_str(), g_ctrlm.production_build ? "PROD" : "DEV", g_ctrlm.image_version.c_str(), g_ctrlm.image_branch.c_str(), g_ctrlm.image_build_time.c_str());
   }

   rdk_version_object_free(&info);

   return(ret_val == 0);
}

gboolean ctrlm_load_device_mac(void) {
   gboolean ret = false;
   std::string mac;
   std::string file;
   std::ifstream ifs;
   const char *interface = NULL;

   // First check environment variable to find ESTB interface, or fall back to default
   interface = getenv("ESTB_INTERFACE");
   file = "/sys/class/net/" + std::string((interface != NULL ? interface : CTRLM_DEFAULT_DEVICE_MAC_INTERFACE)) + "/address";

   // Open file and read mac address
   ifs.open(file.c_str(), std::ifstream::in);
   if(ifs.is_open()) {
      ifs >> mac;
      std::transform(mac.begin(), mac.end(), mac.begin(), ::toupper);
      g_ctrlm.device_mac = mac;
      ret = true;
      XLOGD_INFO("Device Mac set to <%s>", ctrlm_is_pii_mask_enabled() ? "***" : mac.c_str());
   } else {
      XLOGD_ERROR("Failed to get MAC address for device mac");
      g_ctrlm.device_mac = "UNKNOWN";
   }

   return(ret);
}

#ifdef CTRLM_THUNDER
void ctrlm_device_info_activated(void *user_data) {
   ctrlm_device_type_t device_type = CTRLM_DEVICE_TYPE_INVALID;
   
   g_ctrlm.thunder_device_info->get_device_type(device_type);

   ctrlm_device_type_loaded(device_type);
}
#endif

void ctrlm_device_type_loaded(ctrlm_device_type_t device_type) {
   if(device_type >= CTRLM_DEVICE_TYPE_INVALID) {
      XLOGD_ERROR("invalid device type <%s>", ctrlm_device_type_str(device_type));
      ctrlm_main_has_device_type_set(false);
   } else {
      g_ctrlm.device_type = device_type;
      XLOGD_INFO("device type <%s>", ctrlm_device_type_str(g_ctrlm.device_type));

      g_ctrlm.voice_session->voice_stb_data_device_type_set(g_ctrlm.device_type);

      #ifdef IRDB_ENABLED
      XLOGD_INFO("create IRDB object");
      g_ctrlm.irdb = ctrlm_irdb_create((g_ctrlm.device_type == CTRLM_DEVICE_TYPE_TV) ? true : false);
      #endif

      ctrlm_main_has_device_type_set(true);
   }
}

gboolean ctrlm_load_device_type(void) {
   gboolean ret = true;

   #ifdef CTRLM_THUNDER
   g_ctrlm.thunder_device_info = Thunder::DeviceInfo::ctrlm_thunder_plugin_device_info_t::getInstance();
   g_ctrlm.thunder_device_info->add_on_activation_handler((Thunder::DeviceInfo::on_activation_handler_t)ctrlm_device_info_activated);
   #else
   ctrlm_device_type_loaded(CTRLM_DEVICE_TYPE_STB_IP);
   #endif

   return(ret);
}

void ctrlm_main_has_device_type_set(gboolean has_type) {
   g_ctrlm.has_device_type = has_type;
}

gboolean ctrlm_main_has_device_type_get(void) {
   return(g_ctrlm.has_device_type);
}

#ifdef AUTH_ENABLED
void ctrlm_main_auth_start_poll() {
   ctrlm_timeout_destroy(&g_ctrlm.authservice_poll_tag);
   g_ctrlm.authservice_poll_tag = ctrlm_timeout_create(g_ctrlm.recently_booted? g_ctrlm.authservice_fast_poll_val : g_ctrlm.authservice_poll_val,
                                                         ctrlm_authservice_poll,
                                                         NULL);
}

#ifdef AUTH_RECEIVER_ID
gboolean ctrlm_main_has_receiver_id_get(void) {
   return(g_ctrlm.has_receiver_id);
}

void ctrlm_main_has_receiver_id_set(gboolean has_id) {
   g_ctrlm.has_receiver_id = has_id;
}

gboolean ctrlm_load_receiver_id(void) {
   if(!g_ctrlm.authservice->get_receiver_id(g_ctrlm.receiver_id)) {
      ctrlm_main_has_receiver_id_set(false);
      return(false);
   }

   g_ctrlm.voice_session->voice_stb_data_receiver_id_set(g_ctrlm.receiver_id);

   for(auto const &itr : g_ctrlm.networks) {
      itr.second->receiver_id_set(g_ctrlm.receiver_id);
   }
   ctrlm_main_has_receiver_id_set(true);
   return(true);
}
#endif

#ifdef AUTH_DEVICE_ID
void ctrlm_main_has_device_id_set(gboolean has_id) {
   g_ctrlm.has_device_id   = has_id;
}

gboolean ctrlm_main_has_device_id_get(void) {
   return(g_ctrlm.has_device_id);
}

gboolean ctrlm_load_device_id(void) {
   if(!g_ctrlm.authservice->get_device_id(g_ctrlm.device_id)) {
      ctrlm_main_has_device_id_set(false);
      return(false);
   }
   g_ctrlm.voice_session->voice_stb_data_device_id_set(g_ctrlm.device_id);

   for(auto const &itr : g_ctrlm.networks) {
      itr.second->device_id_set(g_ctrlm.device_id);
   }
   ctrlm_main_has_device_id_set(true);
   return(true);
}
#endif

#ifdef AUTH_ACCOUNT_ID
gboolean ctrlm_main_has_service_account_id_get(void) {
   return(g_ctrlm.has_service_account_id);
}

void ctrlm_main_has_service_account_id_set(gboolean has_id) {
   g_ctrlm.has_service_account_id = has_id;
}

gboolean ctrlm_load_service_account_id(const char *account_id) {
   if(account_id == NULL) {
      if(!g_ctrlm.authservice->get_account_id(g_ctrlm.service_account_id)) {
         ctrlm_main_has_service_account_id_set(false);
         return(false);
      }
   } else {
      g_ctrlm.service_account_id = account_id;
   }
   g_ctrlm.voice_session->voice_stb_data_account_number_set(g_ctrlm.service_account_id);

   for(auto const &itr : g_ctrlm.networks) {
      itr.second->service_account_id_set(g_ctrlm.service_account_id);
   }
   ctrlm_main_has_service_account_id_set(true);
   return(true);
}
#endif

#ifdef AUTH_PARTNER_ID
gboolean ctrlm_main_has_partner_id_get(void) {
   return(g_ctrlm.has_partner_id);
}

void ctrlm_main_has_partner_id_set(gboolean has_id) {
   g_ctrlm.has_partner_id = has_id;
}

gboolean ctrlm_load_partner_id(void) {
   if(!g_ctrlm.authservice->get_partner_id(g_ctrlm.partner_id)) {
      ctrlm_main_has_partner_id_set(false);
      return(false);
   }
   g_ctrlm.voice_session->voice_stb_data_partner_id_set(g_ctrlm.partner_id);

   for(auto const &itr : g_ctrlm.networks) {
      itr.second->partner_id_set(g_ctrlm.partner_id);
   }
   ctrlm_main_has_partner_id_set(true);
   return(true);
}
#endif

#ifdef AUTH_EXPERIENCE
gboolean ctrlm_main_has_experience_get(void) {
   return(g_ctrlm.has_experience);
}

void ctrlm_main_has_experience_set(gboolean has_experience) {
   g_ctrlm.has_experience = has_experience;
}

gboolean ctrlm_load_experience(void) {
   if(!g_ctrlm.authservice->get_experience(g_ctrlm.experience)) {
      ctrlm_main_has_experience_set(false);
      return(false);
   }
   g_ctrlm.voice_session->voice_stb_data_experience_set(g_ctrlm.experience);

   for(auto const &itr : g_ctrlm.networks) {
      itr.second->experience_set(g_ctrlm.experience);
   }
   ctrlm_main_has_experience_set(true);
   return(true);
}
#endif

#ifdef AUTH_SAT_TOKEN
gboolean ctrlm_main_needs_service_access_token_get(void) {
   gboolean ret = false;
   sem_wait(&g_ctrlm.service_access_token_semaphore);
   if(g_ctrlm.sat_enabled) {
      ret = !g_ctrlm.has_service_access_token;
   }
   sem_post(&g_ctrlm.service_access_token_semaphore);
   return ret;
}

void ctrlm_main_has_service_access_token_set(gboolean has_token) {
   sem_wait(&g_ctrlm.service_access_token_semaphore);
   g_ctrlm.has_service_access_token = has_token;
   sem_post(&g_ctrlm.service_access_token_semaphore);
}

gboolean ctrlm_load_service_access_token(void) {
   if(!g_ctrlm.authservice->get_sat(g_ctrlm.service_access_token, g_ctrlm.service_access_token_expiration)) {
      ctrlm_main_has_service_access_token_set(false);
      return(false);
   }
   time_t current = time(NULL);
   if(g_ctrlm.service_access_token_expiration - current <= 0) {
      XLOGD_WARN("SAT Token retrieved is already expired...");
      ctrlm_main_has_service_access_token_set(false);
      return(false);
   }

   ctrlm_main_has_service_access_token_set(true);
   XLOGD_INFO("SAT Token retrieved and expires in %ld seconds", g_ctrlm.service_access_token_expiration - current);
   XLOGD_DEBUG("<%s>", g_ctrlm.service_access_token.c_str());
   g_ctrlm.voice_session->voice_stb_data_sat_set(g_ctrlm.service_access_token);

   if(!g_ctrlm.authservice->supports_sat_expiration()) {
      time_t timeout = g_ctrlm.service_access_token_expiration - current;
      XLOGD_INFO("Setting SAT Timer for %ld seconds", timeout);
      g_ctrlm.service_access_token_expiration_tag = ctrlm_timeout_create(timeout * 1000, ctrlm_authservice_expired, NULL);
   }
   return(true);
}
#endif
#endif

gboolean ctrlm_has_authservice_data(void) {
   gboolean ret = TRUE;
#ifdef AUTH_ENABLED
#ifdef AUTH_RECEIVER_ID
   if(!ctrlm_main_has_receiver_id_get()) {
      ret = FALSE;
   }
#endif

#ifdef AUTH_DEVICE_ID
   if(!ctrlm_main_has_device_id_get()) {
      ret = FALSE;
   }
#endif

#ifdef AUTH_ACCOUNT_ID
   if(!ctrlm_main_has_service_account_id_get()) {
      ret = FALSE;
   }
#endif

#ifdef AUTH_PARTNER_ID
   if(!ctrlm_main_has_partner_id_get()) {
      ret = FALSE;
   }
#endif

#ifdef AUTH_EXPERIENCE
   if(!ctrlm_main_has_experience_get()) {
      ret = FALSE;
   }
#endif

#ifdef AUTH_SAT_TOKEN
   if(ctrlm_main_needs_service_access_token_get()) {
      ret = FALSE;
   }
#endif
#endif

   return(ret);
}

gboolean ctrlm_load_authservice_data(void) {
   gboolean ret = TRUE;
#ifdef AUTH_ENABLED
   if(g_ctrlm.authservice->is_ready()) {
#ifdef AUTH_RECEIVER_ID
   if(!ctrlm_main_has_receiver_id_get()) {
      XLOGD_INFO("load receiver id");
      if(!ctrlm_load_receiver_id()) {
         XLOGD_TELEMETRY("failed to load receiver id");
         ret = FALSE;
      } else {
         XLOGD_INFO("load receiver id successfully <%s>", ctrlm_is_pii_mask_enabled() ? "***" : g_ctrlm.receiver_id.c_str());
      }
   }
#endif

#ifdef AUTH_DEVICE_ID
   if(!ctrlm_main_has_device_id_get()) {
      XLOGD_INFO("load device id");
      if(!ctrlm_load_device_id()) {
         XLOGD_TELEMETRY("failed to load device id");
         ret = FALSE;
      } else {
         XLOGD_INFO("load device id successfully <%s>", ctrlm_is_pii_mask_enabled() ? "***" : g_ctrlm.device_id.c_str());
      }
   }
#endif

#ifdef AUTH_ACCOUNT_ID
   if(!ctrlm_main_has_service_account_id_get()) {
      XLOGD_INFO("load account id");
      if(!ctrlm_load_service_account_id(NULL)) {
         XLOGD_TELEMETRY("failed to load account id");
         ret = FALSE;
      } else {
         XLOGD_INFO("load account id successfully <%s>", ctrlm_is_pii_mask_enabled() ? "***" : g_ctrlm.service_account_id.c_str());
      }
   }
#endif

#ifdef AUTH_PARTNER_ID
   if(!ctrlm_main_has_partner_id_get()) {
      XLOGD_INFO("load partner id");
      if(!ctrlm_load_partner_id()) {
         XLOGD_TELEMETRY("failed to load partner id");
         ret = FALSE;
      } else {
         XLOGD_INFO("load partner id successfully <%s>", ctrlm_is_pii_mask_enabled() ? "***" : g_ctrlm.partner_id.c_str());
      }
   }
#endif

#ifdef AUTH_EXPERIENCE
   if(!ctrlm_main_has_experience_get()) {
      XLOGD_INFO("load experience");
      if(!ctrlm_load_experience()) {
         XLOGD_TELEMETRY("failed to load experience");
         ret = FALSE;
      } else {
         XLOGD_INFO("load experience successfully <%s>", ctrlm_is_pii_mask_enabled() ? "***" : g_ctrlm.experience.c_str());
      }
   }
#endif

#ifdef AUTH_SAT_TOKEN
   if(ctrlm_main_needs_service_access_token_get()) {
      XLOGD_INFO("load sat token");
      if(!ctrlm_load_service_access_token()) {
         XLOGD_TELEMETRY("failed to load sat token");
         if(!g_ctrlm.authservice->supports_sat_expiration()) { // Do not continue to poll just for SAT if authservice supports onChange event
            ret = FALSE;
         }
      } else {
         XLOGD_INFO("load sat token successfully");
      }
   }
#endif
   } else {
      XLOGD_WARN("Authservice is not ready...");
   }
#endif

   return(ret);
}

gboolean ctrlm_load_config(json_t **json_obj_root, json_t **json_obj_net_rf4ce, json_t **json_obj_voice, json_t **json_obj_device_update, json_t **json_obj_validation, json_t **json_obj_vsdk) {
   std::string config_fn_opt = "/opt/ctrlm_config.json";
   std::string config_fn_etc = "/etc/ctrlm_config.json";
   json_t *json_obj_ctrlm;
   ctrlm_config_t *ctrlm_config = ctrlm_config_t::get_instance();
   gboolean local_conf = false;
   
   XLOGD_INFO("");

   if(ctrlm_config == NULL) {
      XLOGD_ERROR("Failed to get config manager instance");
      return(false);
   } else if(!ctrlm_is_production_build() && g_file_test(config_fn_opt.c_str(), G_FILE_TEST_EXISTS) && ctrlm_config->load_config(config_fn_opt)) {
      XLOGD_INFO("Read configuration from <%s>", config_fn_opt.c_str());
      local_conf = true;
   } else if(g_file_test(config_fn_etc.c_str(), G_FILE_TEST_EXISTS) && ctrlm_config->load_config(config_fn_etc)) {
      XLOGD_INFO("Read configuration from <%s>", config_fn_etc.c_str());
   } else {
      XLOGD_WARN("Configuration error. Configuration file(s) missing, using defaults");
      return(false);
   }

   // Parse the JSON data
   *json_obj_root = ctrlm_config->json_from_path("", true); // Get root AND add ref to it, since this code derefs it
   if(*json_obj_root == NULL) {
      XLOGD_ERROR("JSON object from config manager is NULL");
      return(false);
   } else if(!json_is_object(*json_obj_root)) {
      // invalid response data
      XLOGD_INFO("received invalid json response data - not a json object");
      json_decref(*json_obj_root);
      *json_obj_root = NULL;
      return(false);
   }

   // Extract the RF4CE network configuration object
   #ifdef CTRLM_NETWORK_RF4CE
   *json_obj_net_rf4ce = json_object_get(*json_obj_root, JSON_OBJ_NAME_NETWORK_RF4CE);
   if(*json_obj_net_rf4ce == NULL || !json_is_object(*json_obj_net_rf4ce)) {
      XLOGD_WARN("RF4CE network object not found");
   }
   #endif

   // Extract the voice configuration object
   *json_obj_voice = json_object_get(*json_obj_root, JSON_OBJ_NAME_VOICE);
   if(*json_obj_voice == NULL || !json_is_object(*json_obj_voice)) {
      XLOGD_WARN("voice object not found");
   }

   // Extract the device update configuration object
   *json_obj_device_update = json_object_get(*json_obj_root, JSON_OBJ_NAME_DEVICE_UPDATE);
   if(*json_obj_device_update == NULL || !json_is_object(*json_obj_device_update)) {
      XLOGD_WARN("device update object not found");
   }

  //Extract the vsdk configuration object
   *json_obj_vsdk = json_object_get( *json_obj_root, JSON_OBJ_NAME_VSDK);
   if(*json_obj_vsdk == NULL || !json_is_object(*json_obj_vsdk)) {
      XLOGD_WARN("vsdk object not found");
      json_obj_vsdk = NULL;
   }

   // Extract the ctrlm global configuration object
   json_obj_ctrlm = json_object_get(*json_obj_root, JSON_OBJ_NAME_CTRLM_GLOBAL);
   if(json_obj_ctrlm == NULL || !json_is_object(json_obj_ctrlm)) {
      XLOGD_WARN("control manger object not found");
   } else {
      json_config conf_global;
      if(!conf_global.config_object_set(json_obj_ctrlm)) {
         XLOGD_ERROR("unable to set config object");
      }

      // Extract the validation configuration object
      *json_obj_validation = json_object_get(json_obj_ctrlm, JSON_OBJ_NAME_CTRLM_GLOBAL_VALIDATION_CONFIG);
      if(*json_obj_validation == NULL || !json_is_object(*json_obj_validation)) {
         XLOGD_WARN("validation object not found");
      }

      // Now parse the control manager object
      json_t *json_obj = json_object_get(json_obj_ctrlm, JSON_STR_NAME_CTRLM_GLOBAL_DB_PATH);
      const char *text = "Database Path";
      if(json_obj != NULL && json_is_string(json_obj)) {
         XLOGD_INFO("%-24s - PRESENT <%s>", text, json_string_value(json_obj));
         g_ctrlm.db_path = json_string_value(json_obj);
      } else {
         XLOGD_INFO("%-24s - ABSENT", text);
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_STR_NAME_CTRLM_GLOBAL_MINIDUMP_PATH);
      text = "Minidump Path";
      if(json_obj != NULL && json_is_string(json_obj)) {
         XLOGD_INFO("%-24s - PRESENT <%s>", text, json_string_value(json_obj));
         g_ctrlm.minidump_path = json_string_value(json_obj);
      } else {
         XLOGD_INFO("%-24s - ABSENT", text);
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_THREAD_MONITOR_PERIOD);
      text = "Thread Monitor Period";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 1) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.thread_monitor_timeout_val = value * 1000;
         }
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_BOOL_NAME_CTRLM_GLOBAL_THREAD_MONITOR_MINIDUMP);
      text     = "Thread Monitor Minidump";
      if(json_obj != NULL && json_is_boolean(json_obj)) {
         XLOGD_INFO("%-28s - PRESENT <%s>", text, json_is_true(json_obj) ? "true" : "false");
         if(json_is_true(json_obj)) {
            g_ctrlm.thread_monitor_minidump = true;
         } else {
            g_ctrlm.thread_monitor_minidump = false;
         }
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_STR_NAME_CTRLM_GLOBAL_URL_AUTH_SERVICE);
      text     = "Auth Service URL";
      if(json_obj == NULL || !json_is_string(json_obj)) {
         XLOGD_INFO("%-25s - ABSENT", text);
      } else {
         XLOGD_INFO("%-25s - PRESENT <%s>", text, json_string_value(json_obj));
         g_ctrlm.server_url_authservice = json_string_value(json_obj);
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_AUTHSERVICE_POLL_PERIOD);
      text = "Authservice Poll Period";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 1) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.authservice_poll_val = value * 1000;
         }
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_AUTHSERVICE_FAST_POLL_PERIOD);
      text = "Authservice Fast Poll Period";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 1) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.authservice_fast_poll_val = value;
         }
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_AUTHSERVICE_FAST_MAX_RETRIES);
      text = "Authservice Fast Max Retries";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 1) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.authservice_fast_retries_max = value;
         }
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_RECENTLY_BOOTED);
      text = "Timeout Recently Booted";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 0) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.recently_booted_timeout_val = value;
         }
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_LINE_OF_SIGHT);
      text     = "Timeout Line of Sight";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 0) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.line_of_sight_timeout_val = value;
         }
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_AUTOBIND);
      text     = "Timeout Autobind";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 0) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.autobind_timeout_val = value;
         }
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_BUTTON_BINDING);
      text     = "Timeout Button Binding";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 0) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.binding_button_timeout_val = value;
         }
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_SCREEN_BIND);
      text     = "Timeout Screen Bind";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 0) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.screen_bind_timeout_val = value;
         }
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_ONE_TOUCH_AUTOBIND);
      text     = "Timeout One Touch Autobind";
      if(json_obj == NULL || !json_is_integer(json_obj)) {
         XLOGD_INFO("%-24s - ABSENT", text);
      } else {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 0) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.one_touch_autobind_timeout_val = value;
         }
      }

      text     = "Mask PII";
      if(conf_global.config_value_get(JSON_ARRAY_NAME_CTRLM_GLOBAL_MASK_PII, g_ctrlm.mask_pii, ctrlm_is_production_build() ? CTRLM_JSON_ARRAY_INDEX_PRD : CTRLM_JSON_ARRAY_INDEX_DEV)) {
         XLOGD_INFO("%-28s - PRESENT <%s>", text, g_ctrlm.mask_pii ? "true" : "false");
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }
      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_CRASH_RECOVERY_THRESHOLD);
      text     = "Crash Recovery Threshold";
      if(json_obj != NULL && json_is_integer(json_obj)) {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value < 0) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.crash_recovery_threshold = value;
         }
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }

      json_obj = json_object_get(json_obj_ctrlm, JSON_INT_NAME_CTRLM_GLOBAL_TELEMETRY_REPORT_INTERVAL);
      text     = "Telemetry Report Interval";
      if(json_obj != NULL && json_is_integer(json_obj)) {
         json_int_t value = json_integer_value(json_obj);
         XLOGD_INFO("%-24s - PRESENT <%lld>", text, value);
         if(value <= 0) {
            XLOGD_INFO("%-24s - OUT OF RANGE %lld", text, value);
         } else {
            g_ctrlm.telemetry_report_interval = value;
         }
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }

#if defined(AUTH_ENABLED) && defined(AUTH_DEVICE_ID)
      json_obj = json_object_get(json_obj_ctrlm, JSON_STR_NAME_CTRLM_GLOBAL_DEVICE_ID);
      text = "Device ID";
      if(json_obj != NULL && json_is_string(json_obj) && (strlen(json_string_value(json_obj)) != 0)) {
         XLOGD_INFO("%-24s - PRESENT <%s>", text, json_string_value(json_obj));
         g_ctrlm.device_id = json_string_value(json_obj);
         ctrlm_main_has_device_id_set(true);
      } else {
         XLOGD_INFO("%-24s - ABSENT", text);
      }
#endif
   }

   XLOGD_INFO("Database Path                <%s>", g_ctrlm.db_path.c_str());
   XLOGD_INFO("Minidump Path                <%s>", g_ctrlm.minidump_path.c_str());
   XLOGD_INFO("Thread Monitor Period        %u ms", g_ctrlm.thread_monitor_timeout_val);
   XLOGD_INFO("Authservice Poll Period      %u ms", g_ctrlm.authservice_poll_val);
   XLOGD_INFO("Timeout Recently Booted      %u ms", g_ctrlm.recently_booted_timeout_val);
   XLOGD_INFO("Timeout Line of Sight        %u ms", g_ctrlm.line_of_sight_timeout_val);
   XLOGD_INFO("Timeout Autobind             %u ms", g_ctrlm.autobind_timeout_val);
   XLOGD_INFO("Timeout Screen Bind          %u ms", g_ctrlm.screen_bind_timeout_val);
   XLOGD_INFO("Timeout One Touch Autobind   %u ms", g_ctrlm.one_touch_autobind_timeout_val);
   XLOGD_INFO("Mask PII                     <%s>", g_ctrlm.mask_pii ? "YES" : "NO");
   XLOGD_INFO("Crash Recovery Threshold     <%u>", g_ctrlm.crash_recovery_threshold);
   XLOGD_INFO("Auth Service URL             <%s>", g_ctrlm.server_url_authservice.c_str());
   XLOGD_INFO("Telemetry Report Interval    %u ms", g_ctrlm.telemetry_report_interval);

   g_ctrlm.local_conf = local_conf;

   return true;
}

void ctrlm_global_rfc_values_retrieved(const ctrlm_rfc_attr_t &attr) {
   // DB Path / Authservice URL / Device ID updates not supported via RFC
   // attr.get_rfc_value(JSON_STR_NAME_CTRLM_GLOBAL_DB_PATH, g_ctrlm.db_path);
   // attr.get_rfc_value(JSON_STR_NAME_CTRLM_GLOBAL_URL_AUTH_SERVICE, g_ctrlm.server_url_authservice);
   // attr.get_rfc_value(JSON_STR_NAME_CTRLM_GLOBAL_DEVICE_ID, g_ctrlm.device_id);
   #ifdef BREAKPAD_SUPPORT
   if(attr.get_rfc_value(JSON_STR_NAME_CTRLM_GLOBAL_MINIDUMP_PATH, g_ctrlm.minidump_path)) {
      google_breakpad::MinidumpDescriptor descriptor(g_ctrlm.minidump_path.c_str());
      google_breakpad::ExceptionHandler eh(descriptor, NULL, ctrlm_minidump_callback, NULL, true, -1);
   }
   #endif
   if(attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_THREAD_MONITOR_PERIOD, g_ctrlm.thread_monitor_timeout_val, 1)) {
      g_ctrlm.thread_monitor_timeout_val *= 1000;
   }

   if(attr.get_rfc_value(JSON_BOOL_NAME_CTRLM_GLOBAL_THREAD_MONITOR_MINIDUMP, g_ctrlm.thread_monitor_minidump)) {
      XLOGD_INFO("Thread Monitor Minidump is <%s>", (g_ctrlm.thread_monitor_minidump ? "ENABLED" : "DISABLED"));
   }

   if(attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_AUTHSERVICE_POLL_PERIOD, g_ctrlm.authservice_poll_val, 1)) {
      g_ctrlm.authservice_poll_val *= 1000;
   }
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_RECENTLY_BOOTED, g_ctrlm.recently_booted_timeout_val, 1);
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_LINE_OF_SIGHT, g_ctrlm.line_of_sight_timeout_val, 0);
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_AUTOBIND, g_ctrlm.autobind_timeout_val, 0);
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_BUTTON_BINDING, g_ctrlm.binding_button_timeout_val, 0);
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_SCREEN_BIND, g_ctrlm.screen_bind_timeout_val, 0);
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_TIMEOUT_ONE_TOUCH_AUTOBIND, g_ctrlm.one_touch_autobind_timeout_val, 0);
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_CRASH_RECOVERY_THRESHOLD, g_ctrlm.crash_recovery_threshold, 0);
   if(attr.get_rfc_value(JSON_ARRAY_NAME_CTRLM_GLOBAL_MASK_PII, g_ctrlm.mask_pii, ctrlm_is_production_build() ? CTRLM_JSON_ARRAY_INDEX_PRD : CTRLM_JSON_ARRAY_INDEX_DEV)) {
      g_ctrlm.voice_session->voice_stb_data_pii_mask_set(g_ctrlm.mask_pii);
   }
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_AUTHSERVICE_POLL_PERIOD, g_ctrlm.authservice_poll_val);   
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_AUTHSERVICE_FAST_POLL_PERIOD, g_ctrlm.authservice_fast_poll_val);
   attr.get_rfc_value(JSON_INT_NAME_CTRLM_GLOBAL_AUTHSERVICE_FAST_MAX_RETRIES, g_ctrlm.authservice_fast_retries_max);   
}

gboolean ctrlm_iarm_init(void) {
   IARM_Result_t result;

   // Initialize the IARM Bus
   result = IARM_Bus_Init(CTRLM_MAIN_IARM_BUS_NAME);
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_FATAL("Unable to initialize IARM bus!");
      return(false);
   }

   // Connect to IARM Bus
   result = IARM_Bus_Connect();
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_FATAL("Unable to connect IARM bus!");
      return(false);
   }

   // Register all events that can be generated by Control Manager
   result = IARM_Bus_RegisterEvent(CTRLM_MAIN_IARM_EVENT_MAX);
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_FATAL("Unable to register events!");
      return(false);
   }

   return(true);
}


void ctrlm_iarm_terminate(void) {
   IARM_Result_t result;

   // Disconnect from IARM Bus
   result = IARM_Bus_Disconnect();
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_FATAL("Unable to disconnect IARM bus!");
   }

   // Terminate the IARM Bus
   #if 0 // TODO may need to enable this when running in the RDK
   result = IARM_Bus_Term();
   if(IARM_RESULT_SUCCESS != result) {
      XLOGD_FATAL("Unable to terminate IARM bus!");
   }
   #endif
}

ctrlm_network_id_t network_id_get_next(ctrlm_network_type_t network_type) {
   ctrlm_network_id_t network_id = CTRLM_MAIN_NETWORK_ID_ALL;

   static ctrlm_network_id_t network_id_rf4ce  = NETWORK_ID_BASE_RF4CE;
   static ctrlm_network_id_t network_id_ip     = NETWORK_ID_BASE_IP;
   static ctrlm_network_id_t network_id_ble    = NETWORK_ID_BASE_BLE;
   static ctrlm_network_id_t network_id_custom = NETWORK_ID_BASE_CUSTOM;
   switch(network_type) {
      case CTRLM_NETWORK_TYPE_RF4CE:
         network_id = network_id_rf4ce++;
         if(NETWORK_ID_BASE_IP == network_id)
         {
             XLOGD_WARN("RF4CE network ID wraparound" );
             network_id = NETWORK_ID_BASE_RF4CE;
         }
         break;
      case CTRLM_NETWORK_TYPE_IP:
         network_id = network_id_ip++;
         if(NETWORK_ID_BASE_BLE == network_id)
         {
             XLOGD_WARN("IP network ID wraparound" );
             network_id = NETWORK_ID_BASE_IP;
         }
         break;
      case CTRLM_NETWORK_TYPE_BLUETOOTH_LE:
         network_id = network_id_ble++;
         if(NETWORK_ID_BASE_CUSTOM == network_id)
         {
             XLOGD_WARN("BLE network ID wraparound" );
             network_id = NETWORK_ID_BASE_BLE;
         }
         break;
      case CTRLM_NETWORK_TYPE_DSP:
          network_id = CTRLM_MAIN_NETWORK_ID_DSP;
          break;
      default:
         network_id = network_id_custom++;
         if(CTRLM_MAIN_NETWORK_ID_DSP == network_id_custom)
         {
             XLOGD_WARN("Custom network ID wraparound" );
             network_id_custom = NETWORK_ID_BASE_CUSTOM;
         }
         break;
   }
   return(network_id);
}

extern ctrlm_obj_network_t* create_ctrlm_obj_network_t(ctrlm_network_type_t type, ctrlm_network_id_t id, const char *name, gboolean mask_key_codes, json_t *json_obj_net_ip, GThread *original_thread);

gboolean ctrlm_networks_pre_init(json_t *json_obj_net_rf4ce, json_t *json_config_root) {
   #ifdef CTRLM_NETWORK_RF4CE
   ctrlm_network_id_t network_id;
   network_id = network_id_get_next(CTRLM_NETWORK_TYPE_RF4CE);

   ctrlm_obj_network_rf4ce_t *obj_net_rf4ce = new ctrlm_obj_network_rf4ce_t(CTRLM_NETWORK_TYPE_RF4CE, network_id, "RF4CE", g_ctrlm.mask_pii, json_obj_net_rf4ce, g_thread_self());
   // Set main function for the RF4CE Network object
   obj_net_rf4ce->hal_api_main_set(ctrlm_hal_rf4ce_main);
   g_ctrlm.networks[network_id]      = obj_net_rf4ce;
   //g_ctrlm.networks[network_id].net.rf4ce = obj_net_rf4ce;
   g_ctrlm.network_type[network_id]       = g_ctrlm.networks[network_id]->type_get();
   #endif

   vendor_network_opts_t vendor_network_opts;
   vendor_network_opts.ignore_mask    = 0;
   vendor_network_opts.mask_key_codes = g_ctrlm.mask_pii;

   networks_map_t networks_map;
   ctrlm_vendor_network_factory(&vendor_network_opts, json_config_root, networks_map);


   auto networks_map_end = networks_map.end();
   for (auto net = networks_map.begin(); net != networks_map_end; ++net) {
      g_ctrlm.networks[net->first]   = net->second;
      g_ctrlm.network_type[net->first]    = g_ctrlm.networks[net->first]->type_get();
   }

   for(auto const &itr : g_ctrlm.networks) {
      itr.second->stb_name_set(g_ctrlm.stb_name);
      itr.second->mask_key_codes_set(g_ctrlm.mask_pii);
   }

   return(true);
}

gboolean ctrlm_networks_init(void) {
   //ctrlm_crash();

   // Initialize all the network interfaces
   for(auto const &itr : g_ctrlm.networks) {
      // Initialize the network
      ctrlm_hal_result_t result = itr.second->network_init(g_ctrlm.main_thread);

      if(CTRLM_HAL_RESULT_SUCCESS != result) { // Error occurred
         // For now, we only care about rf4ce
         if((itr.second->type_get() == CTRLM_NETWORK_TYPE_RF4CE || !itr.second->is_failed_state()) || itr.second->type_get() == CTRLM_NETWORK_TYPE_BLUETOOTH_LE) {
             return(false);
         }
      }
   }

   // Read Control Service values from DB
   control_service_values_read_from_db();

   return(true);
}

void ctrlm_networks_terminate(void) {
   g_ctrlm.successful_init = false;
   for(auto const &itr : g_ctrlm.networks) {
      XLOGD_INFO("Terminating %s network", itr.second->name_get());

      itr.second->network_destroy();

      // Call destructor
      delete itr.second;
   }
}

void ctrlm_network_list_get(vector<ctrlm_network_id_t> *list) {
   if(list == NULL || list->size() != 0) {
      XLOGD_WARN("Invalid list.");
      return;
   }
   vector<ctrlm_network_id_t>::iterator it_vector;
   it_vector = list->begin();

   for(auto const &itr : g_ctrlm.networks) {
      XLOGD_INFO("Network id %u", itr.first);
      it_vector = list->insert(it_vector, itr.first);
   }
}

// Determine if the network id is valid
gboolean ctrlm_network_id_is_valid(ctrlm_network_id_t network_id) {
   if(g_ctrlm.networks.count(network_id) == 0) {
      if(network_id != CTRLM_MAIN_NETWORK_ID_DSP) {
         XLOGD_TELEMETRY("Invalid Network Id %u", network_id);
      }
      return(false);
   }
   return(true);
}

gboolean ctrlm_controller_id_is_valid(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   gboolean ret = false;

   if(ctrlm_network_id_is_valid(network_id)) {
      if(g_ctrlm.main_thread == g_thread_self()) {
         XLOGD_DEBUG("main thread");
         ret = g_ctrlm.networks[network_id]->controller_exists(controller_id);
      } else {
         ctrlm_main_queue_msg_controller_exists_t dqm = {0};
         sem_t semaphore;
         sem_init(&semaphore, 0, 0);

         dqm.controller_id     = controller_id;
         dqm.semaphore         = &semaphore;
         dqm.controller_exists = &ret;

         ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::controller_exists, &dqm, sizeof(dqm), NULL, network_id);

         sem_wait(&semaphore);
         sem_destroy(&semaphore);
      }
   }
   return ret;
}

// Return the network type
ctrlm_network_type_t ctrlm_network_type_get(ctrlm_network_id_t network_id) {
   if(!ctrlm_network_id_is_valid(network_id)) {
      if(network_id != CTRLM_MAIN_NETWORK_ID_DSP) {
         XLOGD_ERROR("Invalid Network Id");
      }
      return(CTRLM_NETWORK_TYPE_INVALID);
   }
   return(g_ctrlm.network_type[network_id]);
}

std::vector<ctrlm_network_type_t> ctrlm_network_types_get(void) {
    std::vector<ctrlm_network_type_t> network_types;

    for (auto const &it : g_ctrlm.network_type) {
        network_types.push_back(it.second);
    }
    return network_types;
}

ctrlm_network_id_t ctrlm_network_id_get(ctrlm_network_type_t network_type) {
   auto pred = [network_type](const std::pair<ctrlm_network_id_t, ctrlm_network_type_t>& entry) {return network_type == entry.second;};
   auto networks_type_end = g_ctrlm.network_type.cend();
   auto network_type_entry = std::find_if(g_ctrlm.network_type.cbegin(), networks_type_end, pred);
   if (network_type_entry != networks_type_end) {
      return network_type_entry->first;
   }
   return(CTRLM_NETWORK_TYPE_INVALID);
}

gboolean ctrlm_precomission_lookup(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   return(g_ctrlm.precomission_table.count(controller_id));
}
gboolean ctrlm_is_recently_booted(void) {
   return(g_ctrlm.recently_booted);
}

gboolean ctrlm_is_line_of_sight(void) {
   return(g_ctrlm.line_of_sight);
}

gboolean ctrlm_is_autobind_active(void) {
   return(g_ctrlm.autobind);
}

gboolean ctrlm_is_binding_button_pressed(void) {
   return(g_ctrlm.binding_button);
}

gboolean ctrlm_is_binding_screen_active(void) {
   return(g_ctrlm.binding_screen_active);
}

gboolean ctrlm_is_one_touch_autobind_active(void) {
   return(g_ctrlm.one_touch_autobind_active);
}

gboolean ctrlm_is_binding_table_empty(void) {
   return((g_ctrlm.bound_controller_qty > 0) ? FALSE : TRUE);
}

// Add a message to the control manager's processing queue
void ctrlm_main_queue_msg_push(gpointer msg) {
   g_async_queue_push(g_ctrlm.queue, msg);
}

void ctrlm_queue_msg_destroy(gpointer msg) {
   if(msg) {
      g_free(msg);
   }
}

void ctrlm_main_update_export_controller_list() {

   XLOGD_DEBUG("entering");
   device_update_check_locations_t update_locations_valid = ctrlm_device_update_check_locations_get();
   string xconf_path = ctrlm_device_update_get_xconf_path();

   // we create the list if the check location is set to xconf or both
   if (update_locations_valid == DEVICE_UPDATE_CHECK_FILESYSTEM) {
      XLOGD_WARN("set for file system updates.  Do not process controller list");
      return;
   }
   XLOGD_DEBUG("doing xconf json create");

   xconf_path += "/rc-proxy-params.json";

   json_t *controller_list = json_array();
   for(auto const &itr : g_ctrlm.networks) {
      json_t *temp = itr.second->xconf_export_controllers();
      if(temp && json_is_array(temp)) {
         json_array_extend(controller_list, temp);
      } else {
         XLOGD_WARN("Network %d did not supply a valid controller list", itr.first);
      }
   }
   // CUSTOM FORMATTING CODE
   if(json_array_size(controller_list) == 0) {
      XLOGD_INFO("No controller information for XCONF, writing empty file");
   }
   std::string output = "";

   // Work with string now
   output += "[\n";
   for(unsigned int i = 0; i < json_array_size(controller_list); i++) {
      char *buf = json_dumps(json_array_get(controller_list, i), JSON_PRESERVE_ORDER | JSON_INDENT(0) | JSON_COMPACT);
      if(buf) {
         output += buf;
         if(i != json_array_size(controller_list)-1) {
            output += ",";
         }
         output += "\n";
      }
   }
   output += "]";
   if(false == g_file_set_contents(xconf_path.c_str(), output.c_str(), output.length(), NULL)) {
      XLOGD_ERROR("Failed to dump xconf controller list");
   }

   // END CUSTOM FORMATTING CODE
   json_decref(controller_list);

   XLOGD_DEBUG("exiting");

}

void ctrlm_main_update_check_update_complete_all(ctrlm_main_queue_msg_update_file_check_t *msg) {

   XLOGD_DEBUG("entering");
#ifdef CTRLM_NETWORK_RF4CE
   try {
      for(auto const &itr : g_ctrlm.networks) {
         ctrlm_obj_network_t *obj_net = itr.second;
         ctrlm_network_type_t network_type = obj_net->type_get();
         XLOGD_INFO("network %s", obj_net->name_get());
         if(network_type == CTRLM_NETWORK_TYPE_RF4CE) {
            ((ctrlm_obj_network_rf4ce_t *)obj_net)->check_if_update_file_still_needed(msg);
         }
      }

   } catch (exception& e) {
      XLOGD_ERROR("exception %s", e.what());
   }
#endif
   XLOGD_DEBUG("exiting");

}

gpointer ctrlm_main_thread(gpointer param) {
   bool running = true;
   XLOGD_INFO("Started");
   // Unblock the caller that launched this thread
   sem_post(&g_ctrlm.semaphore);

   XLOGD_INFO("Enter main loop");
   do {
      gpointer msg = g_async_queue_pop(g_ctrlm.queue);

      if(msg == NULL) {
         XLOGD_ERROR("NULL message received");
         continue;
      }

      ctrlm_main_queue_msg_header_t *hdr = (ctrlm_main_queue_msg_header_t *)msg;
      ctrlm_obj_network_t           *obj_net       = NULL;

      XLOGD_DEBUG("Type <%s> Network Id %u", ctrlm_main_queue_msg_type_str(hdr->type), hdr->network_id);
      if(0 == (hdr->type & CTRLM_MAIN_QUEUE_MSG_TYPE_GLOBAL)) { // Network specific message
         // This check has to occur before assigning the network objects,
         // since the object will be created if it doesn't exist already.
         if(!ctrlm_network_id_is_valid(hdr->network_id)) {
            XLOGD_ERROR("INVALID Network Id! %u", hdr->network_id);
            ctrlm_queue_msg_destroy(msg);
            continue;
         }

         obj_net       = g_ctrlm.networks[hdr->network_id];

         if (obj_net == NULL) {
            XLOGD_ERROR("Network object is NULL");
            ctrlm_queue_msg_destroy(msg);
            continue;
         } else if (!obj_net->is_ready()) {
            XLOGD_ERROR("Network %s not ready", obj_net->name_get());
            ctrlm_queue_msg_destroy(msg);
            continue;
         }
      }

      switch(hdr->type) {
         case CTRLM_MAIN_QUEUE_MSG_TYPE_TICKLE: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_TICKLE");
            ctrlm_thread_monitor_msg_t *thread_monitor_msg = (ctrlm_thread_monitor_msg_t *) msg;
            *thread_monitor_msg->response = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_TERMINATE: {
            XLOGD_INFO("message type CTRLM_MAIN_QUEUE_MSG_TYPE_TERMINATE");
            sem_post(&g_ctrlm.semaphore);
            running = false;
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_EXPORT_CONTROLLER_LIST: {
            XLOGD_DEBUG("message type %s", ctrlm_main_queue_msg_type_str(hdr->type));
            ctrlm_main_update_export_controller_list();
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_BEGIN: {
            ctrlm_main_queue_msg_bind_validation_begin_t *dqm = (ctrlm_main_queue_msg_bind_validation_begin_t *)msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_BEGIN");

            ctrlm_validation_begin(hdr->network_id, dqm->controller_id, obj_net->ctrlm_controller_type_get(dqm->controller_id));
            obj_net->bind_validation_begin(dqm);
            obj_net->iarm_event_rcu_status();
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_END: {
            ctrlm_main_queue_msg_bind_validation_end_t *dqm = (ctrlm_main_queue_msg_bind_validation_end_t *)msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_END");

            ctrlm_validation_end(hdr->network_id, dqm->controller_id, obj_net->ctrlm_controller_type_get(dqm->controller_id), dqm->binding_type, dqm->validation_type, dqm->result, dqm->semaphore, dqm->cmd_result);
            obj_net->bind_validation_end(dqm);
            obj_net->iarm_event_rcu_status();
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_FAILED_TIMEOUT: {
              ctrlm_main_queue_msg_bind_validation_failed_timeout_t *dqm = (ctrlm_main_queue_msg_bind_validation_failed_timeout_t *)msg;
              XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_VALIDATION_FAILED_TIMEOUT");
              obj_net->bind_validation_timeout(dqm->controller_id);
              if(g_ctrlm.pairing_window.active) {
                 ctrlm_pairing_window_bind_status_set_(CTRLM_BIND_STATUS_VALILDATION_FAILURE);
              }
              break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_CONFIGURATION_COMPLETE: {
            ctrlm_main_queue_msg_bind_configuration_complete_t *dqm = (ctrlm_main_queue_msg_bind_configuration_complete_t *)msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_BIND_CONFIGURATION_COMPLETE");
            ctrlm_controller_status_t status;
            if(dqm->result == CTRLM_RCU_CONFIGURATION_RESULT_SUCCESS) {
               obj_net->ctrlm_controller_status_get(dqm->controller_id, &status);
            } else {
               errno_t safec_rc = memset_s(&status, sizeof(ctrlm_controller_status_t), 0, sizeof(ctrlm_controller_status_t));
               ERR_CHK(safec_rc);
            }

            ctrlm_configuration_complete(hdr->network_id, dqm->controller_id, obj_net->ctrlm_controller_type_get(dqm->controller_id), obj_net->ctrlm_binding_type_get(dqm->controller_id), &status, dqm->result);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_NETWORK_PROPERTY_SET: {
            ctrlm_main_queue_msg_network_property_set_t *dqm = (ctrlm_main_queue_msg_network_property_set_t *)msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_NETWORK_PROPERTY_SET");
            XLOGD_INFO("Setting network property <%s>", ctrlm_hal_network_property_str(dqm->property));
            ctrlm_hal_result_t result = obj_net->property_set(dqm->property, dqm->value);
            if(result != CTRLM_HAL_RESULT_SUCCESS) {
                  XLOGD_ERROR("Unable to set network property <%s>", ctrlm_hal_network_property_str(dqm->property));
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STATUS: {
            ctrlm_main_queue_msg_main_status_t *dqm = (ctrlm_main_queue_msg_main_status_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STATUS");
            ctrlm_main_iarm_call_status_get_(dqm->status);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_SET: {
            ctrlm_main_queue_msg_main_property_t *dqm = (ctrlm_main_queue_msg_main_property_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_SET");
            ctrlm_main_iarm_call_property_set_(dqm->property);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_GET: {
            ctrlm_main_queue_msg_main_property_t *dqm = (ctrlm_main_queue_msg_main_property_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_GET");
            ctrlm_main_iarm_call_property_get_(dqm->property);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_DISCOVERY_CONFIG_SET: {
            ctrlm_main_queue_msg_main_discovery_config_t *dqm = (ctrlm_main_queue_msg_main_discovery_config_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_DISCOVERY_CONFIG_SET");
            ctrlm_main_iarm_call_discovery_config_set_(dqm->config);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_AUTOBIND_CONFIG_SET: {
            ctrlm_main_queue_msg_main_autobind_config_t *dqm = (ctrlm_main_queue_msg_main_autobind_config_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_AUTOBIND_CONFIG_SET");
            ctrlm_main_iarm_call_autobind_config_set_(dqm->config);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PRECOMMISSION_CONFIG_SET: {
            ctrlm_main_queue_msg_main_precommision_config_t *dqm = (ctrlm_main_queue_msg_main_precommision_config_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PRECOMMISSION_CONFIG_SET");
            ctrlm_main_iarm_call_precommission_config_set_(dqm->config);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_FACTORY_RESET: {
            ctrlm_main_queue_msg_main_factory_reset_t *dqm = (ctrlm_main_queue_msg_main_factory_reset_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_FACTORY_RESET");
            ctrlm_main_iarm_call_factory_reset_(dqm->reset);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROLLER_UNBIND: {
            ctrlm_main_queue_msg_main_controller_unbind_t *dqm = (ctrlm_main_queue_msg_main_controller_unbind_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROLLER_UNBIND");
            ctrlm_main_iarm_call_controller_unbind_(dqm->unbind);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_LINE_OF_SIGHT: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_LINE_OF_SIGHT");
            g_ctrlm.line_of_sight             = FALSE;
            g_ctrlm.line_of_sight_timeout_tag = 0;
            ctrlm_main_iarm_event_binding_line_of_sight(FALSE);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_AUTOBIND: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_AUTOBIND");
            g_ctrlm.autobind             = FALSE;
            g_ctrlm.autobind_timeout_tag = 0;
            ctrlm_main_iarm_event_autobind_line_of_sight(FALSE);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_BUTTON: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_BUTTON");
            if (g_ctrlm.binding_button) {
               ctrlm_timeout_destroy(&g_ctrlm.binding_button_timeout_tag);
               g_ctrlm.binding_button = FALSE;
               ctrlm_main_iarm_event_binding_button(FALSE);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_SCREEN: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_SCREEN");
            if (g_ctrlm.binding_screen_active) {
               ctrlm_timeout_destroy(&g_ctrlm.screen_bind_timeout_tag);
               g_ctrlm.binding_screen_active = FALSE;
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_ONE_TOUCH_AUTOBIND: {
            ctrlm_stop_one_touch_autobind_(hdr->network_id);
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_ONE_TOUCH_AUTOBIND");
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CLOSE_PAIRING_WINDOW: {
            ctrlm_main_queue_msg_close_pairing_window_t *dqm = (ctrlm_main_queue_msg_close_pairing_window_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CLOSE_PAIRING_WINDOW");
            ctrlm_close_pairing_window_(dqm->network_id, dqm->reason);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_BIND_STATUS_SET: {
            ctrlm_main_queue_msg_pairing_window_bind_status_t *dqm = (ctrlm_main_queue_msg_pairing_window_bind_status_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_BIND_STATUS_SET");
            ctrlm_pairing_window_bind_status_set_(dqm->bind_status);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_DISCOVERY_REMOTE_TYPE_SET: {
            ctrlm_main_queue_msg_discovery_remote_type_t *dqm = (ctrlm_main_queue_msg_discovery_remote_type_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_DISCOVERY_REMOTE_TYPE_SET");
            ctrlm_discovery_remote_type_set_(dqm->remote_type_str);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_CHECK_UPDATE_FILE_NEEDED:
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_CHECK_UPDATE_FILE_NEEDED");
            ctrlm_main_update_check_update_complete_all((ctrlm_main_queue_msg_update_file_check_t *)msg);
            break;
         case CTRLM_MAIN_QUEUE_MSG_TYPE_THREAD_MONITOR_POLL: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_THREAD_MONITOR_POLL");
            ctrlm_thread_monitor_msg_t *thread_monitor_msg = (ctrlm_thread_monitor_msg_t *) msg;
            ctrlm_obj_network_t        *obj_net            = thread_monitor_msg->obj_network;
            obj_net->thread_monitor_poll(thread_monitor_msg->response);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_POWER_STATE_CHANGE: {
            ctrlm_power_state_t old_state = g_ctrlm.power_state;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_POWER_STATE_CHANGE");
            ctrlm_main_queue_power_state_change_t *dqm = (ctrlm_main_queue_power_state_change_t *)msg;
            if(NULL == dqm) {
               XLOGD_ERROR("Power State Change msg NULL");
               break;
            }
            
            XLOGD_DEBUG("Old power state <%s>, new power state <%s>", ctrlm_power_state_str(old_state), ctrlm_power_state_str(dqm->new_state));

            if(old_state == dqm->new_state) {
               XLOGD_INFO("Already in power state <%s>", ctrlm_power_state_str(old_state));
               break;
            }

            if( (old_state == CTRLM_POWER_STATE_DEEP_SLEEP) && (dqm->new_state != CTRLM_POWER_STATE_DEEP_SLEEP) ) {
               XLOGD_INFO("power_state_change: wake DB and networks");
               #ifdef DEEP_SLEEP_ENABLED
               g_ctrlm.wake_with_voice_allowed = true;
               #endif
               ctrlm_db_power_state_change(true);
               for(auto const &itr : g_ctrlm.networks) {
                  itr.second->power_state_change(true);
               }
            }else if( (old_state != CTRLM_POWER_STATE_DEEP_SLEEP) && (dqm->new_state == CTRLM_POWER_STATE_DEEP_SLEEP) ) {
               XLOGD_INFO("power_state_change: halt DB and networks");
               ctrlm_db_power_state_change(false);
               for(auto const &itr : g_ctrlm.networks) {
                  itr.second->power_state_change(false);
               }
            }

            #ifdef DEEP_SLEEP_ENABLED
            //Wake with voice? Handle NSM voice, do not change power state
            if(dqm->new_state == CTRLM_POWER_STATE_ON) {
               bool wake_with_voice_allowed = g_ctrlm.wake_with_voice_allowed;
               g_ctrlm.wake_with_voice_allowed = false;
               if( (wake_with_voice_allowed == true) && (ctrlm_main_iarm_wakeup_reason_voice() == true) ) {
                  if( g_ctrlm.voice_session->nsm_voice_session == true ) {
                     XLOGD_INFO("Handling NSM voice session, ignore ON");
                  } else {
                     g_ctrlm.voice_session->voice_nsm_session_request();
                  }
                  break;
               }
            }
            #endif

            //If execution reaches here, then change power state and inform VSDK of on or deep sleep states
            g_ctrlm.power_state = dqm->new_state;

            XLOGD_INFO("Enter power state <%s>", ctrlm_power_state_str(g_ctrlm.power_state));
            #ifdef DEEP_SLEEP_ENABLED
            if(g_ctrlm.power_state == CTRLM_POWER_STATE_DEEP_SLEEP) {
               XLOGD_INFO("NSM is <%s>",  ctrlm_main_iarm_networked_standby()?"ENABLED":"DISABLED");
            }
            #endif

            if(dqm->new_state != CTRLM_POWER_STATE_STANDBY) {
               // Set VSDK power state
               g_ctrlm.voice_session->voice_power_state_change(g_ctrlm.power_state);
            }
            break;
         }
#ifdef AUTH_ENABLED
         case CTRLM_MAIN_QUEUE_MSG_TYPE_AUTHSERVICE_POLL: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_AUTHSERVICE_POLL");

            if(!ctrlm_load_authservice_data()) {
               bool fast_poll = true;
               if (g_ctrlm.authservice_fast_retries < g_ctrlm.authservice_fast_retries_max) {
                  g_ctrlm.authservice_fast_retries++;
               }
               if(g_ctrlm.authservice_fast_retries == g_ctrlm.authservice_fast_retries_max) {
                  XLOGD_INFO("Switching to normal authservice poll interval");
                  fast_poll = false;
               }
               g_ctrlm.authservice_poll_tag = ctrlm_timeout_create(fast_poll ? g_ctrlm.authservice_fast_poll_val : g_ctrlm.authservice_poll_val, ctrlm_authservice_poll, NULL);
            } else {
               g_ctrlm.authservice_fast_retries = 0;
               g_ctrlm.authservice_poll_tag = 0;
            }
            break;
         }
#endif
#ifdef CTRLM_NETWORK_RF4CE
         case CTRLM_MAIN_QUEUE_MSG_TYPE_NOTIFY_FIRMWARE: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_NOTIFY_FIRMWARE");
            if(ctrlm_main_successful_init_get()) {
               ctrlm_main_queue_msg_notify_firmware_t *dqm = (ctrlm_main_queue_msg_notify_firmware_t *)msg;
               for(auto const &itr : g_ctrlm.networks) {
                  if(ctrlm_network_type_get(itr.first) == CTRLM_NETWORK_TYPE_RF4CE) {
                     ctrlm_obj_network_rf4ce_t *net_rf4ce = (ctrlm_obj_network_rf4ce_t *)itr.second;
                     net_rf4ce->notify_firmware(dqm->controller_type, dqm->image_type, dqm->force_update, dqm->version_software, dqm->version_hardware_min, dqm->version_bootloader_min);
                  }
               }
            } else {
               // Networks are not ready, push back to the queue, then continue so it's not freed
               ctrlm_timeout_create(CTRLM_MAIN_QUEUE_REPEAT_DELAY, ctrlm_message_queue_delay, msg);
               continue;
            }
            break;
         }
#endif
         case CTRLM_MAIN_QUEUE_MSG_TYPE_IR_REMOTE_USAGE: {
            gboolean day_changed = false;
            ctrlm_main_queue_msg_ir_remote_usage_t *dqm = (ctrlm_main_queue_msg_ir_remote_usage_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_IR_REMOTE_USAGE");
            //Check if the day changed
            day_changed = ctrlm_main_handle_day_change_ir_remote_usage();
            if(day_changed) {
               ctrlm_property_write_ir_remote_usage();
            }
            ctrlm_main_iarm_call_ir_remote_usage_get_(dqm->ir_remote_usage);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_PAIRING_METRICS: {
            ctrlm_main_queue_msg_pairing_metrics_t *dqm = (ctrlm_main_queue_msg_pairing_metrics_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_PAIRING_METRICS");
            ctrlm_main_iarm_call_pairing_metrics_get_(dqm->pairing_metrics);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_LAST_KEY_INFO: {
            ctrlm_main_queue_msg_last_key_info_t *dqm = (ctrlm_main_queue_msg_last_key_info_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_LAST_KEY_INFO");
            ctrlm_main_iarm_call_last_key_info_get_(dqm->last_key_info);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_TYPE_GET: {
            bool result = false;
            ctrlm_main_queue_msg_controller_type_get_t *dqm = (ctrlm_main_queue_msg_controller_type_get_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_TYPE_GET");
            if(dqm->controller_type) {
               *dqm->controller_type = obj_net->ctrlm_controller_type_get(dqm->controller_id);
               result = (*dqm->controller_type != CTRLM_RCU_CONTROLLER_TYPE_INVALID ? true : false);
            }
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = (result ? CTRLM_CONTROLLER_STATUS_REQUEST_SUCCESS : CTRLM_CONTROLLER_STATUS_REQUEST_ERROR);
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_SET_VALUES: {
            ctrlm_main_queue_msg_main_control_service_settings_t *dqm = (ctrlm_main_queue_msg_main_control_service_settings_t *) msg;
            ctrlm_main_iarm_call_control_service_settings_t *settings = dqm->settings;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_SET_VALUES");

            if(settings->available & CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_ASB_ENABLED) {
#ifdef ASB
               // Write new asb_enabled flag to NVM
               ctrlm_db_asb_enabled_write(&settings->asb_enabled, CTRLM_ASB_ENABLED_LEN); 
               g_ctrlm.cs_values.asb_enable = settings->asb_enabled;
               XLOGD_INFO("ASB Enabled Set Values <%s>", g_ctrlm.cs_values.asb_enable ? "true" : "false");
#else
               XLOGD_INFO("ASB Enabled Set Values <false>, ASB Not Supported");
#endif
            }
            if(settings->available & CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_OPEN_CHIME_ENABLED) {
               // Write new open_chime_enabled flag to NVM
               ctrlm_db_open_chime_enabled_write(&settings->open_chime_enabled, CTRLM_OPEN_CHIME_ENABLED_LEN);
               g_ctrlm.cs_values.chime_open_enable = settings->open_chime_enabled;
               XLOGD_INFO("Open Chime Enabled <%s>", settings->open_chime_enabled ? "true" : "false");
            }
            if(settings->available & CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_CLOSE_CHIME_ENABLED) {
               // Write new close_chime_enabled flag to NVM
               ctrlm_db_close_chime_enabled_write(&settings->close_chime_enabled, CTRLM_CLOSE_CHIME_ENABLED_LEN); 
               g_ctrlm.cs_values.chime_close_enable = settings->close_chime_enabled;
               XLOGD_INFO("Close Chime Enabled <%s>", settings->close_chime_enabled ? "true" : "false");
            }
            if(settings->available & CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_PRIVACY_CHIME_ENABLED) {
               // Write new privacy_chime_enabled flag to NVM
               ctrlm_db_privacy_chime_enabled_write(&settings->privacy_chime_enabled, CTRLM_PRIVACY_CHIME_ENABLED_LEN); 
               g_ctrlm.cs_values.chime_privacy_enable = settings->privacy_chime_enabled;
               XLOGD_INFO("Privacy Chime Enabled <%s>", settings->privacy_chime_enabled ? "true" : "false");
            }
            if(settings->available & CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_CONVERSATIONAL_MODE) {
               if(settings->conversational_mode > CTRLM_MAX_CONVERSATIONAL_MODE) {
                  XLOGD_WARN("Conversational Mode Invalid <%d>.  Ignoring.", settings->conversational_mode);
               } else {
                  // Write new conversational mode to NVM
                  ctrlm_db_conversational_mode_write((guchar *)&settings->conversational_mode, CTRLM_CONVERSATIONAL_MODE_LEN); 
                  g_ctrlm.cs_values.conversational_mode = settings->conversational_mode;
                  XLOGD_INFO("Conversational Mode Set <%d>", settings->conversational_mode);
               }
            }
            if(settings->available & CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_SET_CHIME_VOLUME) {
               if(settings->chime_volume >= CTRLM_CHIME_VOLUME_INVALID) {
                  XLOGD_WARN("Chime Volume Invalid <%d>.  Ignoring.", settings->chime_volume);
               } else {
                  // Write new chime_volume to NVM
                  ctrlm_db_chime_volume_write((guchar *)&settings->chime_volume, CTRLM_CHIME_VOLUME_LEN); 
                  g_ctrlm.cs_values.chime_volume = settings->chime_volume;
                  XLOGD_INFO("Chime Volume Set <%d>", settings->chime_volume);
               }
            }
            if(settings->available & CTRLM_MAIN_CONTROL_SERVICE_SETTINGS_SET_IR_COMMAND_REPEATS) {
               if((settings->ir_command_repeats < CTRLM_MIN_IR_COMMAND_REPEATS) || (settings->ir_command_repeats > CTRLM_MAX_IR_COMMAND_REPEATS)) {
                  XLOGD_WARN("IR command repeats Invalid <%d>.  Ignoring.", settings->ir_command_repeats);
               } else {
                  // Write new ir_command_repeats to NVM
                  ctrlm_db_ir_command_repeats_write(&settings->ir_command_repeats, CTRLM_IR_COMMAND_REPEATS_LEN); 
                  g_ctrlm.cs_values.ir_repeats = settings->ir_command_repeats;
                  XLOGD_INFO("IR Command Repeats Set <%d>", settings->ir_command_repeats);
               }
            }

            // Set these values in the networks
            for(auto const &itr : g_ctrlm.networks) {
               itr.second->cs_values_set(&g_ctrlm.cs_values, false);
            }

            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_GET_VALUES: {
            ctrlm_main_queue_msg_main_control_service_settings_t *dqm = (ctrlm_main_queue_msg_main_control_service_settings_t *) msg;
            ctrlm_main_iarm_call_control_service_settings_t *settings = dqm->settings;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_GET_VALUES");
#ifdef ASB
            settings->asb_supported = true;
#else
            settings->asb_supported = false;
#endif
            settings->asb_enabled   = g_ctrlm.cs_values.asb_enable;
            settings->open_chime_enabled          = g_ctrlm.cs_values.chime_open_enable;
            settings->close_chime_enabled         = g_ctrlm.cs_values.chime_close_enable;
            settings->privacy_chime_enabled       = g_ctrlm.cs_values.chime_privacy_enable;
            settings->conversational_mode         = g_ctrlm.cs_values.conversational_mode;
            settings->chime_volume                = g_ctrlm.cs_values.chime_volume;
            settings->ir_command_repeats          = g_ctrlm.cs_values.ir_repeats;
            XLOGD_INFO("ASB Get Values: Supported <%s>  ASB Enabled <%s>  Open Chime Enabled <%s>  Close Chime Enabled <%s>  Privacy Chime Enabled <%s>  Conversational Mode <%u>  Chime Volume <%d>  IR Command Repeats <%d>",
               settings->asb_supported ? "true" : "false", settings->asb_enabled ? "true" : "false", settings->open_chime_enabled ? "true" : "false",
               settings->close_chime_enabled ? "true" : "false", settings->privacy_chime_enabled ? "true" : "false", settings->conversational_mode, settings->chime_volume, settings->ir_command_repeats);
            
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_CAN_FIND_MY_REMOTE: {
            ctrlm_main_queue_msg_main_control_service_can_find_my_remote_t *dqm = (ctrlm_main_queue_msg_main_control_service_can_find_my_remote_t *) msg;
            ctrlm_main_iarm_call_control_service_can_find_my_remote_t *can_find_my_remote = dqm->can_find_my_remote;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_CAN_FIND_MY_REMOTE");
            obj_net = g_ctrlm.networks[hdr->network_id];
            can_find_my_remote->is_supported = obj_net->is_fmr_supported();
            XLOGD_INFO("Can find My Remote: Supported <%s>", can_find_my_remote->is_supported ? "true" : "false");
            
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_START_PAIRING_MODE: {
            ctrlm_main_queue_msg_main_control_service_pairing_mode_t *dqm = (ctrlm_main_queue_msg_main_control_service_pairing_mode_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_START_PAIRING_MODE");
            ctrlm_main_iarm_call_control_service_start_pairing_mode_(dqm->pairing);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_END_PAIRING_MODE: {
            ctrlm_main_queue_msg_main_control_service_pairing_mode_t *dqm = (ctrlm_main_queue_msg_main_control_service_pairing_mode_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_END_PAIRING_MODE");
            ctrlm_main_iarm_call_control_service_end_pairing_mode_(dqm->pairing);
            if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
               // Signal the semaphore to indicate that the result is present
               *dqm->cmd_result = CTRLM_MAIN_STATUS_REQUEST_SUCCESS;
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_BATTERY_MILESTONE_EVENT: {
            ctrlm_main_queue_msg_rf4ce_battery_milestone_t *dqm = (ctrlm_main_queue_msg_rf4ce_battery_milestone_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_BATTERY_MILESTONE_EVENT");
            ctrlm_rcu_iarm_event_battery_milestone(dqm->network_id, dqm->controller_id, dqm->battery_event, dqm->percent);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_REMOTE_REBOOT_EVENT: {
            ctrlm_main_queue_msg_remote_reboot_t *dqm = (ctrlm_main_queue_msg_remote_reboot_t *) msg;
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_REMOTE_REBOOT_EVENT");
            ctrlm_rcu_iarm_event_remote_reboot(dqm->network_id, dqm->controller_id, dqm->voltage, dqm->reason, dqm->assert_number);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_NTP_CHECK: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_NTP_CHECK");
            if(time(NULL) < ctrlm_shutdown_time_get()) {
               ctrlm_timeout_create(5000, ctrlm_ntp_check, NULL);
            } else {
               XLOGD_INFO("Time is correct, calling functions that depend on time at boot");
               for(auto const &itr : g_ctrlm.networks) {
                  itr.second->set_timers();
               }
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_REVERSE_CMD: {
            if(ctrlm_main_successful_init_get()) {
               ctrlm_main_queue_msg_rcu_reverse_cmd_t *dqm = (ctrlm_main_queue_msg_rcu_reverse_cmd_t *) msg;
               ctrlm_controller_status_cmd_result_t      cmd_result = CTRLM_CONTROLLER_STATUS_REQUEST_ERROR;
               XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_CONTROLLER_REVERSE_CMD");
               cmd_result = obj_net->req_process_reverse_cmd(dqm);
               if(dqm->semaphore != NULL && dqm->cmd_result != NULL) {
                  // Signal the semaphore to indicate that the result is present
                  *dqm->cmd_result = cmd_result;
                  sem_post(dqm->semaphore);
               }
            } else {
               // Networks are not ready, push back to the queue, then continue so it's not freed
               ctrlm_timeout_create(CTRLM_MAIN_QUEUE_REPEAT_DELAY, ctrlm_message_queue_delay, msg);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_AUDIO_CAPTURE_START: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_AUDIO_CAPTURE_START");
            ctrlm_main_queue_msg_audio_capture_start_t *start_msg = (ctrlm_main_queue_msg_audio_capture_start_t *)msg;
            g_ctrlm.voice_session->ctrlm_voice_xrsr_session_capture_start(start_msg);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_AUDIO_CAPTURE_STOP: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_AUDIO_CAPTURE_STOP");
            g_ctrlm.voice_session->ctrlm_voice_xrsr_session_capture_stop();
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_ACCOUNT_ID_UPDATE: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_ACCOUNT_ID_UPDATE");
            #ifdef AUTH_ENABLED
            #ifdef AUTH_ACCOUNT_ID
            ctrlm_main_queue_msg_account_id_update_t *dqm = (ctrlm_main_queue_msg_account_id_update_t *)msg;
            if(dqm) {
               ctrlm_load_service_account_id(dqm->account_id);
            }
            #endif
            #endif
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_STARTUP: {
            XLOGD_DEBUG("message type CTRLM_MAIN_QUEUE_MSG_TYPE_STARTUP");
            // All main thread startup activities can be placed here.
            // RFC Retrieval, NTP check, register for IARM calls
            
            ctrlm_start_iarm(NULL);

            // init device manager
            ctrlm_dsmgr_init();
            
            ctrlm_rfc_t *rfc = ctrlm_rfc_t::get_instance();
            if(rfc) {
               rfc->fetch_attributes();
            }

            // Perform any post initialization network startup actions
            for(auto const &itr : g_ctrlm.networks) {
               itr.second->start(g_ctrlm.main_loop);
            }

            ctrlm_timeout_create(1000, ctrlm_ntp_check, NULL);
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_HANDLER: {
            ctrlm_main_queue_msg_handler_t *dqm = (ctrlm_main_queue_msg_handler_t *) msg;

            switch(dqm->type) {
               case CTRLM_HANDLER_NETWORK: {
                  ctrlm_obj_network_t *net = (ctrlm_obj_network_t *)dqm->obj;
                  if(net != NULL) { // This occurs when the network object is passed in
                     (net->*dqm->msg_handler.n)(dqm->data, dqm->data_len);
                  } else {               // This occurs when the network_id is "supposed" to be passed in, let's check
                     vector<ctrlm_obj_network_t *> nets;
                     if(dqm->header.network_id == CTRLM_MAIN_NETWORK_ID_ALL) { // This is for ALL networks, this is still valid
                        for(const auto &itr : g_ctrlm.networks) {
                           nets.push_back(itr.second);
                        }
                     } else if(ctrlm_network_id_is_valid(dqm->header.network_id)) { // Valid network id, this is only for one network
                        nets.push_back(g_ctrlm.networks[dqm->header.network_id]);
                     } else {
                        if(dqm->header.network_id != CTRLM_MAIN_NETWORK_ID_DSP) {
                           XLOGD_ERROR("Invalid Network ID! %u", dqm->header.network_id);
                        }
                        break;
                     }
                     for(const auto &itr : nets) {
                        (itr->*dqm->msg_handler.n)(dqm->data, dqm->data_len);
                     }
                  }
                  break;
               }
               case CTRLM_HANDLER_VOICE: {
                  ctrlm_voice_endpoint_t *voice = (ctrlm_voice_endpoint_t *)dqm->obj;
                  (voice->*dqm->msg_handler.v)(dqm->data, dqm->data_len);
                  break;
               }
               case CTRLM_HANDLER_CONTROLLER: {
                 ctrlm_obj_controller_t *controller = (ctrlm_obj_controller_t *)dqm->obj;
                 if(controller) {
                    (controller->*dqm->msg_handler.c)(dqm->data, dqm->data_len);
                 } else {
                    XLOGD_ERROR("Controller object NULL!");
                 }
                 break;
               }
               default: {
                  XLOGD_ERROR("unnkown handler type");
                  break;
               }
            }
            if(dqm->semaphore != NULL) {
               sem_post(dqm->semaphore);
            }
            break;
         }
         case CTRLM_MAIN_QUEUE_MSG_TYPE_HANDLER_NEW: {
            ctrlm_main_queue_msg_handler_new_t *dqm = (ctrlm_main_queue_msg_handler_new_t *) msg;

            switch(dqm->type) {
               case CTRLM_HANDLER_NETWORK: {
                  ctrlm_obj_network_t *net = (ctrlm_obj_network_t *)dqm->obj;
                  if(net != NULL) { // This occurs when the network object is passed in
                     (net->*dqm->msg_handler.n)(dqm->data.get(), dqm->data_len);
                  } else {               // This occurs when the network_id is "supposed" to be passed in, let's check
                     vector<ctrlm_obj_network_t *> nets;
                     if(dqm->header.network_id == CTRLM_MAIN_NETWORK_ID_ALL) { // This is for ALL networks, this is still valid
                        for(const auto &itr : g_ctrlm.networks) {
                           nets.push_back(itr.second);
                        }
                     } else if(ctrlm_network_id_is_valid(dqm->header.network_id)) { // Valid network id, this is only for one network
                        nets.push_back(g_ctrlm.networks[dqm->header.network_id]);
                     } else {
                        if(dqm->header.network_id != CTRLM_MAIN_NETWORK_ID_DSP) {
                           XLOGD_ERROR("Invalid Network ID! %u", dqm->header.network_id);
                        }
                        break;
                     }
                     for(const auto &itr : nets) {
                        (itr->*dqm->msg_handler.n)(dqm->data.get(), dqm->data_len);
                     }
                  }
                  break;
               }
               case CTRLM_HANDLER_VOICE: {
                  ctrlm_voice_endpoint_t *voice = (ctrlm_voice_endpoint_t *)dqm->obj;
                  (voice->*dqm->msg_handler.v)(dqm->data.get(), dqm->data_len);
                  break;
               }
               case CTRLM_HANDLER_CONTROLLER: {
                 ctrlm_obj_controller_t *controller = (ctrlm_obj_controller_t *)dqm->obj;
                 if(controller) {
                    (controller->*dqm->msg_handler.c)(dqm->data.get(), dqm->data_len);
                 } else {
                    XLOGD_ERROR("Controller object NULL!");
                 }
                 break;
               }
               default: {
                  XLOGD_ERROR("unnkown handler type");
                  break;
               }
            }
            if(dqm->semaphore != NULL) {
               sem_post(dqm->semaphore);
            }
            delete dqm; msg = NULL;
            break;
         }
         default: {
            if (obj_net == 0 || !obj_net->message_dispatch(msg)) {
               XLOGD_ERROR("Network: %s. Unknown message type %u", (obj_net !=0?obj_net->name_get():"N/A"), hdr->type);
            }
            break;
         }
      }
      ctrlm_queue_msg_destroy(msg);
   } while(running);
   return(NULL);
}


gboolean ctrlm_is_binding_table_full(void) {
   return(g_ctrlm.bound_controller_qty >= CTRLM_MAIN_MAX_BOUND_CONTROLLERS);
}

bool ctrlm_is_pii_mask_enabled(void) {
   return(g_ctrlm.mask_pii);
}

gboolean ctrlm_timeout_recently_booted(gpointer user_data) {
   XLOGD_INFO("Timeout - Recently booted.");
   g_ctrlm.recently_booted             = FALSE;
   g_ctrlm.recently_booted_timeout_tag = 0;
   return(FALSE);
}

gboolean ctrlm_timeout_systemd_restart_delay(gpointer user_data) {
   XLOGD_INFO("Timeout - Update systemd restart delay to " CTRLM_RESTART_DELAY_SHORT "");
   sd_bus *bus = NULL;
   int bus_ret = 0;
   const char *env_variable = "CTRLM_RESTART_DELAY=" CTRLM_RESTART_DELAY_SHORT;

   if((bus_ret = sd_bus_open_system(&bus)) < 0) {
      XLOGD_ERROR("failed to open system bus");
      return(FALSE);
   }
   if((bus_ret = sd_bus_call_method(bus, "org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager", "SetEnvironment", NULL, NULL, "as", 1, env_variable)) < 0) {
      XLOGD_ERROR("failed to call SetEnvironment via system bus <%d>", bus_ret);
      return(FALSE);
   }
   if(bus) {
      sd_bus_unref(bus);
      bus = NULL;
   }
   return(FALSE);
}

gboolean ctrlm_main_iarm_call_status_get(ctrlm_main_iarm_call_status_t *status) {
   if(status == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_status_t *msg = (ctrlm_main_queue_msg_main_status_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_status_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STATUS;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->status            = status;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process STATUS_GET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_status_get_(ctrlm_main_iarm_call_status_t *status) {
   unsigned long index = 0;
   errno_t safec_rc = -1;

   if(g_ctrlm.networks.size() > CTRLM_MAIN_MAX_NETWORKS) {
      status->network_qty = CTRLM_MAIN_MAX_NETWORKS;
   } else {
      status->network_qty = g_ctrlm.networks.size();
   }
   for(auto const &itr : g_ctrlm.networks) {
      status->networks[index].id   = itr.first;
      status->networks[index].type = itr.second->type_get();
      index++;
   }

   status->result = CTRLM_IARM_CALL_RESULT_SUCCESS;

   safec_rc = strcpy_s(status->ctrlm_version, sizeof(status->ctrlm_version), CTRLM_VERSION);
   ERR_CHK(safec_rc);

   safec_rc = strcpy_s(status->ctrlm_commit_id, sizeof(status->ctrlm_commit_id), CTRLM_MAIN_COMMIT_ID);
   ERR_CHK(safec_rc);

   safec_rc = strncpy_s(status->stb_device_id, sizeof(status->stb_device_id), g_ctrlm.device_id.c_str(), CTRLM_MAIN_DEVICE_ID_MAX_LENGTH - 1);
   ERR_CHK(safec_rc);
   status->stb_device_id[CTRLM_MAIN_DEVICE_ID_MAX_LENGTH - 1] = '\0';

   safec_rc = strncpy_s(status->stb_receiver_id, sizeof(status->stb_receiver_id), g_ctrlm.receiver_id.c_str(), CTRLM_MAIN_RECEIVER_ID_MAX_LENGTH - 1);
   ERR_CHK(safec_rc);
   status->stb_receiver_id[CTRLM_MAIN_RECEIVER_ID_MAX_LENGTH - 1] = '\0';
}

gboolean ctrlm_main_iarm_call_ir_remote_usage_get(ctrlm_main_iarm_call_ir_remote_usage_t *ir_remote_usage) {
   if(ir_remote_usage == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_ir_remote_usage_t *msg = (ctrlm_main_queue_msg_ir_remote_usage_t *)g_malloc(sizeof(ctrlm_main_queue_msg_ir_remote_usage_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->type              = CTRLM_MAIN_QUEUE_MSG_TYPE_IR_REMOTE_USAGE;
   msg->ir_remote_usage   = ir_remote_usage;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process IR_REMOTE_USAGE_GET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_ir_remote_usage_get_(ctrlm_main_iarm_call_ir_remote_usage_t *ir_remote_usage) {
   ir_remote_usage->result                  = CTRLM_IARM_CALL_RESULT_SUCCESS;
   ir_remote_usage->today                   = g_ctrlm.today;
   ir_remote_usage->has_ir_xr2_yesterday    = g_ctrlm.ir_remote_usage_yesterday.has_ir_xr2;
   ir_remote_usage->has_ir_xr2_today        = g_ctrlm.ir_remote_usage_today.has_ir_xr2;
   ir_remote_usage->has_ir_xr5_yesterday    = g_ctrlm.ir_remote_usage_yesterday.has_ir_xr5;
   ir_remote_usage->has_ir_xr5_today        = g_ctrlm.ir_remote_usage_today.has_ir_xr5;
   ir_remote_usage->has_ir_xr11_yesterday   = g_ctrlm.ir_remote_usage_yesterday.has_ir_xr11;
   ir_remote_usage->has_ir_xr11_today       = g_ctrlm.ir_remote_usage_today.has_ir_xr11;
   ir_remote_usage->has_ir_xr15_yesterday   = g_ctrlm.ir_remote_usage_yesterday.has_ir_xr15;
   ir_remote_usage->has_ir_xr15_today       = g_ctrlm.ir_remote_usage_today.has_ir_xr15;
   ir_remote_usage->has_ir_remote_yesterday = g_ctrlm.ir_remote_usage_yesterday.has_ir_remote;
   ir_remote_usage->has_ir_remote_today     = g_ctrlm.ir_remote_usage_today.has_ir_remote;
}

gboolean ctrlm_main_iarm_call_pairing_metrics_get(ctrlm_main_iarm_call_pairing_metrics_t *pairing_metrics) {
   if(pairing_metrics == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_pairing_metrics_t *msg = (ctrlm_main_queue_msg_pairing_metrics_t *)g_malloc(sizeof(ctrlm_main_queue_msg_pairing_metrics_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->type              = CTRLM_MAIN_QUEUE_MSG_TYPE_PAIRING_METRICS;
   msg->pairing_metrics   = pairing_metrics;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process IR_REMOTE_USAGE_GET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_pairing_metrics_get_(ctrlm_main_iarm_call_pairing_metrics_t *pairing_metrics) {
   errno_t safec_rc                                        = -1;
   pairing_metrics->result                                 = CTRLM_IARM_CALL_RESULT_SUCCESS;
   pairing_metrics->num_screenbind_failures                = g_ctrlm.pairing_metrics.num_screenbind_failures;
   pairing_metrics->last_screenbind_error_timestamp        = g_ctrlm.pairing_metrics.last_screenbind_error_timestamp;
   pairing_metrics->last_screenbind_error_code             = g_ctrlm.pairing_metrics.last_screenbind_error_code;
   pairing_metrics->num_non_screenbind_failures            = g_ctrlm.pairing_metrics.num_non_screenbind_failures;
   pairing_metrics->last_non_screenbind_error_timestamp    = g_ctrlm.pairing_metrics.last_non_screenbind_error_timestamp;
   pairing_metrics->last_non_screenbind_error_code         = g_ctrlm.pairing_metrics.last_non_screenbind_error_code;
   pairing_metrics->last_non_screenbind_error_binding_type = g_ctrlm.pairing_metrics.last_non_screenbind_error_binding_type;

   safec_rc = strncpy_s(pairing_metrics->last_screenbind_remote_type, sizeof(pairing_metrics->last_screenbind_remote_type), g_ctrlm.pairing_metrics.last_screenbind_remote_type, CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH - 1);
   ERR_CHK(safec_rc);
   pairing_metrics->last_screenbind_remote_type[CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH - 1] = '\0';

   safec_rc = strncpy_s(pairing_metrics->last_non_screenbind_remote_type, sizeof(pairing_metrics->last_non_screenbind_remote_type), g_ctrlm.pairing_metrics.last_non_screenbind_remote_type, CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH - 1);
   ERR_CHK(safec_rc);
   pairing_metrics->last_non_screenbind_remote_type[CTRLM_MAIN_SOURCE_NAME_MAX_LENGTH - 1] = '\0';
   XLOGD_INFO("num_screenbind_failures <%lu>  last_screenbind_error_timestamp <%lums> last_screenbind_error_code <%s> last_screenbind_remote_type <%s>",
      pairing_metrics->num_screenbind_failures, pairing_metrics->last_screenbind_error_timestamp,
      ctrlm_bind_status_str(pairing_metrics->last_screenbind_error_code), pairing_metrics->last_non_screenbind_remote_type);
   XLOGD_INFO("num_non_screenbind_failures <%lu>  last_non_screenbind_error_timestamp <%lums> last_non_screenbind_error_code <%s> last_non_screenbind_remote_type <%s> last_non_screenbind_error_binding_type <%s>",
      pairing_metrics->num_non_screenbind_failures, pairing_metrics->last_non_screenbind_error_timestamp,
      ctrlm_bind_status_str(pairing_metrics->last_non_screenbind_error_code), pairing_metrics->last_non_screenbind_remote_type,
      ctrlm_rcu_binding_type_str((ctrlm_rcu_binding_type_t)pairing_metrics->last_non_screenbind_error_binding_type));
}

gboolean ctrlm_main_iarm_call_last_key_info_get(ctrlm_main_iarm_call_last_key_info_t *last_key_info) {
   if(last_key_info == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_last_key_info_t *msg = (ctrlm_main_queue_msg_last_key_info_t *)g_malloc(sizeof(ctrlm_main_queue_msg_last_key_info_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->type              = CTRLM_MAIN_QUEUE_MSG_TYPE_LAST_KEY_INFO;
   msg->last_key_info     = last_key_info;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process LAST_KEY_INFO_GET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_last_key_info_get_(ctrlm_main_iarm_call_last_key_info_t *last_key_info) {
   last_key_info->result                 = CTRLM_IARM_CALL_RESULT_SUCCESS;
   last_key_info->controller_id          = g_ctrlm.last_key_info.controller_id;
   last_key_info->source_type            = g_ctrlm.last_key_info.source_type;
   last_key_info->source_key_code        = g_ctrlm.last_key_info.source_key_code;
   last_key_info->timestamp              = g_ctrlm.last_key_info.timestamp;
   if(g_ctrlm.last_key_info.source_type == IARM_BUS_IRMGR_KEYSRC_RF) {
      last_key_info->is_screen_bind_mode  = false;
      last_key_info->remote_keypad_config = ctrlm_get_remote_keypad_config(g_ctrlm.last_key_info.source_name);
   } else {
      last_key_info->is_screen_bind_mode  = g_ctrlm.last_key_info.is_screen_bind_mode;
      last_key_info->remote_keypad_config = g_ctrlm.last_key_info.remote_keypad_config;
   }

   errno_t safec_rc = strcpy_s(last_key_info->source_name, sizeof(last_key_info->source_name), g_ctrlm.last_key_info.source_name);
   ERR_CHK(safec_rc);

   if(ctrlm_is_pii_mask_enabled()) {
      XLOGD_INFO("controller_id <%d>, key_code <*>, key_src <%d>, timestamp <%lldms>, is_screen_bind_mode <%d>, remote_keypad_config <%d>, sourceName <%s>", last_key_info->controller_id, last_key_info->source_type, last_key_info->timestamp, last_key_info->is_screen_bind_mode, last_key_info->remote_keypad_config, last_key_info->source_name);
   } else {
      XLOGD_INFO("controller_id <%d>, key_code <%ld>, key_src <%d>, timestamp <%lldms>, is_screen_bind_mode <%d>, remote_keypad_config <%d>, sourceName <%s>", last_key_info->controller_id, last_key_info->source_key_code, last_key_info->source_type, last_key_info->timestamp, last_key_info->is_screen_bind_mode, last_key_info->remote_keypad_config, last_key_info->source_name);
   }
}

gboolean ctrlm_main_iarm_call_network_status_get(ctrlm_main_iarm_call_network_status_t *status) {
   if(status == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_network_status_t msg = {0};

   sem_init(&semaphore, 0, 0);

   msg.status            = status;
   msg.status->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;
   msg.cmd_result        = &cmd_result;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_network_status, &msg, sizeof(msg), NULL, status->network_id);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process NETWORK_STATUS_GET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

gboolean ctrlm_main_iarm_call_property_set(ctrlm_main_iarm_call_property_t *property) {
   if(property == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_property_t *msg = (ctrlm_main_queue_msg_main_property_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_property_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_SET;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->property          = property;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process PROPERTY_SET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_property_set_(ctrlm_main_iarm_call_property_t *property) {
   if(property->network_id != CTRLM_MAIN_NETWORK_ID_ALL && !ctrlm_network_id_is_valid(property->network_id)) {
      property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("network id - Out of range %u", property->network_id);
      return;
   }

   switch(property->name) {
      case CTRLM_PROPERTY_BINDING_BUTTON_ACTIVE: {
         if(property->value == 0) {
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            ctrlm_timeout_destroy(&g_ctrlm.binding_button_timeout_tag);
            // If this was a change, fire the IARM event
            if (g_ctrlm.binding_button) {
               ctrlm_main_iarm_event_binding_button(false);
            }
            g_ctrlm.binding_button = false;
            XLOGD_INFO("BINDING BUTTON <INACTIVE>");
         } else if(property->value == 1) {
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            ctrlm_timeout_destroy(&g_ctrlm.binding_button_timeout_tag);
            // If this was a change, fire the IARM event
            if (!g_ctrlm.binding_button) {
               ctrlm_main_iarm_event_binding_button(true);
            }
            g_ctrlm.binding_button = true;
            // If screenbind is enabled, disable it
            if(g_ctrlm.binding_screen_active == true) {
               g_ctrlm.binding_screen_active = false;
               XLOGD_INFO("BINDING SCREEN <INACTIVE> -- Due to entering button button mode");
            }
            // Set a timer to stop binding button mode
            g_ctrlm.binding_button_timeout_tag = ctrlm_timeout_create(g_ctrlm.binding_button_timeout_val, ctrlm_timeout_binding_button, NULL);
            XLOGD_INFO("BINDING BUTTON <ACTIVE>");
         } else {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_ERROR("BINDING BUTTON ACTIVE - Invalid parameter 0x%08lX", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_BINDING_SCREEN_ACTIVE: {
         if(property->value == 0) {
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            ctrlm_timeout_destroy(&g_ctrlm.screen_bind_timeout_tag);
            g_ctrlm.binding_screen_active = false;
            XLOGD_INFO("SCREEN BIND STATE <INACTIVE>");
         } else if(property->value == 1) {
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            ctrlm_timeout_destroy(&g_ctrlm.screen_bind_timeout_tag);
            g_ctrlm.binding_screen_active = true;
            // If binding button is enabled, disable it
            if(g_ctrlm.binding_button == true) {
               g_ctrlm.binding_button = false;
               XLOGD_INFO("BINDING SCREEN <INACTIVE> -- Due to entering button button mode");
            }
            // Set a timer to stop screen bind mode
            g_ctrlm.screen_bind_timeout_tag = ctrlm_timeout_create(g_ctrlm.screen_bind_timeout_val, ctrlm_timeout_screen_bind, NULL);
            XLOGD_INFO("SCREEN BIND STATE <ACTIVE>");
         } else {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_ERROR("SCREEN BIND STATE - Invalid parameter 0x%08lX", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_BINDING_LINE_OF_SIGHT_ACTIVE: {
         property->result = CTRLM_IARM_CALL_RESULT_ERROR_READ_ONLY;
         XLOGD_ERROR("BINDING LINE OF SIGHT ACTIVE - Read only");
         break;
      }
      case CTRLM_PROPERTY_AUTOBIND_LINE_OF_SIGHT_ACTIVE:{
         property->result = CTRLM_IARM_CALL_RESULT_ERROR_READ_ONLY;
         XLOGD_ERROR("AUTOBIND LINE OF SIGHT ACTIVE - Read only");
         break;
      }
      case CTRLM_PROPERTY_ACTIVE_PERIOD_BUTTON: {
         if(property->value < CTRLM_PROPERTY_ACTIVE_PERIOD_BUTTON_VALUE_MIN || property->value > CTRLM_PROPERTY_ACTIVE_PERIOD_BUTTON_VALUE_MAX) {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_ERROR("ACTIVE PERIOD BUTTON - Out of range %lu", property->value);
         } else {
            g_ctrlm.binding_button_timeout_val = property->value;
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            XLOGD_INFO("ACTIVE PERIOD BUTTON %lu ms", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_ACTIVE_PERIOD_SCREENBIND: {
         if(property->value < CTRLM_PROPERTY_ACTIVE_PERIOD_SCREENBIND_VALUE_MIN || property->value > CTRLM_PROPERTY_ACTIVE_PERIOD_SCREENBIND_VALUE_MAX) {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_ERROR("ACTIVE PERIOD SCREENBIND - Out of range %lu", property->value);
         } else {
            g_ctrlm.screen_bind_timeout_val = property->value;
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            XLOGD_INFO("ACTIVE PERIOD SCREENBIND %lu ms", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_ACTIVE_PERIOD_ONE_TOUCH_AUTOBIND: {
         if(property->value < CTRLM_PROPERTY_ACTIVE_PERIOD_ONE_TOUCH_AUTOBIND_VALUE_MIN || property->value > CTRLM_PROPERTY_ACTIVE_PERIOD_ONE_TOUCH_AUTOBIND_VALUE_MAX) {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_TELEMETRY("ACTIVE PERIOD ONE-TOUCH AUTOBIND - Out of range %lu", property->value);
         } else {
            g_ctrlm.one_touch_autobind_timeout_val = property->value;
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            XLOGD_INFO("ACTIVE PERIOD ONE-TOUCH AUTOBIND %lu ms", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_ACTIVE_PERIOD_LINE_OF_SIGHT: {
         if(property->value < CTRLM_PROPERTY_ACTIVE_PERIOD_LINE_OF_SIGHT_VALUE_MIN || property->value > CTRLM_PROPERTY_ACTIVE_PERIOD_LINE_OF_SIGHT_VALUE_MAX) {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_ERROR("ACTIVE PERIOD LINE OF SIGHT - Out of range %lu", property->value);
         } else {
            g_ctrlm.line_of_sight_timeout_val = property->value;
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            XLOGD_INFO("ACTIVE PERIOD LINE OF SIGHT %lu ms", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_VALIDATION_TIMEOUT_INITIAL: {
         if(property->value < CTRLM_PROPERTY_VALIDATION_TIMEOUT_MIN || property->value > CTRLM_PROPERTY_VALIDATION_TIMEOUT_MAX) {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_ERROR("VALIDATION TIMEOUT INITIAL - Out of range %lu", property->value);
         } else {
            ctrlm_validation_timeout_initial_set(property->value);
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            XLOGD_INFO("VALIDATION TIMEOUT INITIAL %lu ms", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_VALIDATION_TIMEOUT_DURING: {
         if(property->value < CTRLM_PROPERTY_VALIDATION_TIMEOUT_MIN || property->value > CTRLM_PROPERTY_VALIDATION_TIMEOUT_MAX) {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_ERROR("VALIDATION TIMEOUT DURING - Out of range %lu", property->value);
         } else {
            ctrlm_validation_timeout_subsequent_set(property->value);
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            XLOGD_INFO("VALIDATION TIMEOUT DURING %lu ms", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_VALIDATION_MAX_ATTEMPTS: {
         if(property->value > CTRLM_PROPERTY_VALIDATION_MAX_ATTEMPTS_MAX) {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_ERROR("VALIDATION MAX ATTEMPTS - Out of range %lu", property->value);
         } else {
            ctrlm_validation_max_attempts_set(property->value);
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            XLOGD_INFO("VALIDATION MAX ATTEMPTS %lu times", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_CONFIGURATION_TIMEOUT: {
         if(property->value < CTRLM_PROPERTY_CONFIGURATION_TIMEOUT_MIN || property->value > CTRLM_PROPERTY_CONFIGURATION_TIMEOUT_MAX) {
            property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
            XLOGD_ERROR("CONFIGURATION TIMEOUT - Out of range %lu", property->value);
         } else {
            ctrlm_validation_timeout_configuration_set(property->value);
            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            XLOGD_INFO("CONFIGURATION TIMEOUT %lu ms", property->value);
         }
         break;
      }
      case CTRLM_PROPERTY_AUTO_ACK: {
         if(ctrlm_is_production_build()) {
            XLOGD_ERROR("AUTO ACK - unable to set in prod build");
         } else {
            g_ctrlm.auto_ack = property->value ? true : false;

            #ifdef CTRLM_NETWORK_RF4CE
            #if CTRLM_HAL_RF4CE_API_VERSION >= 16
            for(auto const &itr : g_ctrlm.networks) {
               if(itr.second->type_get() == CTRLM_NETWORK_TYPE_RF4CE) {
                  itr.second->property_set(CTRLM_HAL_NETWORK_PROPERTY_AUTO_ACK, &g_ctrlm.auto_ack);
               }
            }
            #endif
            #endif

            property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            XLOGD_INFO("AUTO ACK <%s>", property->value ? "enabled" : "disabled");
         }
         break;
      }
      default: {
         XLOGD_ERROR("Invalid Property %d", property->name);
         property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      }
   }
}

gboolean ctrlm_main_iarm_call_property_get(ctrlm_main_iarm_call_property_t *property) {
   if(property == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_property_t *msg = (ctrlm_main_queue_msg_main_property_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_property_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PROPERTY_GET;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->property          = property;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);


   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process PROPERTY_GET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_property_get_(ctrlm_main_iarm_call_property_t *property) {
   if(property->network_id != CTRLM_MAIN_NETWORK_ID_ALL && !ctrlm_network_id_is_valid(property->network_id)) {
      property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("network id - Out of range %u", property->network_id);
      return;
   }

   switch(property->name) {
      case CTRLM_PROPERTY_BINDING_BUTTON_ACTIVE: {
         property->value  = g_ctrlm.binding_button;
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("BINDING BUTTON <%s>", g_ctrlm.binding_button ? "ACTIVE" : "INACTIVE");
         break;
      }
      case CTRLM_PROPERTY_BINDING_SCREEN_ACTIVE: {
         property->value  = g_ctrlm.binding_screen_active;
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("BINDING SCREEN <%s>", g_ctrlm.binding_screen_active ? "ACTIVE" : "INACTIVE");
         break;
      }
      case CTRLM_PROPERTY_BINDING_LINE_OF_SIGHT_ACTIVE: {
         property->value  = g_ctrlm.line_of_sight;
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("BINDING LINE OF SIGHT <%s>", g_ctrlm.line_of_sight ? "ACTIVE" : "INACTIVE");
         break;
      }
      case CTRLM_PROPERTY_AUTOBIND_LINE_OF_SIGHT_ACTIVE: {
         property->value  = g_ctrlm.autobind;
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("AUTOBIND LINE OF SIGHT <%s>", g_ctrlm.autobind ? "ACTIVE" : "INACTIVE");
         break;
      }
      case CTRLM_PROPERTY_ACTIVE_PERIOD_BUTTON: {
         property->value  = g_ctrlm.binding_button_timeout_val;
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("ACTIVE PERIOD BUTTON %lu ms", property->value);
         break;
      }
      case CTRLM_PROPERTY_ACTIVE_PERIOD_SCREENBIND: {
         property->value  = g_ctrlm.screen_bind_timeout_val;
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("ACTIVE PERIOD SCREENBIND %lu ms", property->value);
         break;
      }
      case CTRLM_PROPERTY_ACTIVE_PERIOD_ONE_TOUCH_AUTOBIND: {
         property->value  = g_ctrlm.one_touch_autobind_timeout_val;
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("ACTIVE PERIOD ONE-TOUCH AUTOBIND %lu ms", property->value);
         break;
      }
      case CTRLM_PROPERTY_ACTIVE_PERIOD_LINE_OF_SIGHT: {
         property->value  = g_ctrlm.line_of_sight_timeout_val;
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("ACTIVE PERIOD LINE OF SIGHT %lu ms", property->value);
         break;
      }
      case CTRLM_PROPERTY_VALIDATION_TIMEOUT_INITIAL: {
         property->value  = ctrlm_validation_timeout_initial_get();
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("VALIDATION TIMEOUT INITIAL %lu ms", property->value);
         break;
      }
      case CTRLM_PROPERTY_VALIDATION_TIMEOUT_DURING: {
         property->value  = ctrlm_validation_timeout_subsequent_get();
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("VALIDATION TIMEOUT DURING %lu ms", property->value);
         break;
      }
      case CTRLM_PROPERTY_VALIDATION_MAX_ATTEMPTS: {
         property->value  = ctrlm_validation_max_attempts_get();
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("VALIDATION MAX ATTEMPTS %lu times", property->value);
         break;
      }
      case CTRLM_PROPERTY_CONFIGURATION_TIMEOUT: {
         property->value  = ctrlm_validation_timeout_configuration_get();
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("CONFIGURATION TIMEOUT %lu ms", property->value);
         break;
      }
      case CTRLM_PROPERTY_AUTO_ACK: {
         property->value  = g_ctrlm.auto_ack;
         property->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         XLOGD_INFO("AUTO ACK <%s>", property->value ? "enabled" : "disabled");
         break;
      }
      default: {
         XLOGD_ERROR("Invalid Property %d", property->name);
         property->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      }
   }
}


gboolean ctrlm_main_iarm_call_discovery_config_set(ctrlm_main_iarm_call_discovery_config_t *config) {
   if(config == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_discovery_config_t *msg = (ctrlm_main_queue_msg_main_discovery_config_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_discovery_config_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_DISCOVERY_CONFIG_SET;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->config            = config;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process DISCOVERY_CONFIG_SET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_discovery_config_set_(ctrlm_main_iarm_call_discovery_config_t *config) {
   if(config->network_id != CTRLM_MAIN_NETWORK_ID_ALL && !ctrlm_network_id_is_valid(config->network_id)) {
      config->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("network id - Out of range %u", config->network_id);
      return;
   }
   config->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
   ctrlm_controller_discovery_config_t discovery_config;
   discovery_config.enabled               = config->enable ? true : false;
   discovery_config.require_line_of_sight = config->require_line_of_sight ? true : false;

   for(auto const &itr : g_ctrlm.networks) {
      if(config->network_id == itr.first || config->network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
         itr.second->discovery_config_set(discovery_config);
         config->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
      }
   }
}

gboolean ctrlm_main_iarm_call_autobind_config_set(ctrlm_main_iarm_call_autobind_config_t *config) {
   if(config == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_autobind_config_t *msg = (ctrlm_main_queue_msg_main_autobind_config_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_autobind_config_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_AUTOBIND_CONFIG_SET;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->config            = config;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process AUTOBIND_CONFIG_SET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_autobind_config_set_(ctrlm_main_iarm_call_autobind_config_t *config) {
   if(config->network_id != CTRLM_MAIN_NETWORK_ID_ALL && !ctrlm_network_id_is_valid(config->network_id)) {
      config->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("network id - Out of range %u", config->network_id);
      return;
   }

   ctrlm_timeout_destroy(&g_ctrlm.one_touch_autobind_timeout_tag);

   if(config->threshold_pass < CTRLM_AUTOBIND_THRESHOLD_MIN || config->threshold_pass > CTRLM_AUTOBIND_THRESHOLD_MAX) {
      config->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("threshold pass - Out of range %u", config->threshold_pass);
   } else if(config->threshold_fail < CTRLM_AUTOBIND_THRESHOLD_MIN || config->threshold_fail > CTRLM_AUTOBIND_THRESHOLD_MAX) {
      config->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("threshold fail - Out of range %u", config->threshold_fail);
   } else {
      // If screenbind is enabled, disable it
      if(g_ctrlm.binding_screen_active == true) {
         g_ctrlm.binding_screen_active = false;
         XLOGD_INFO("BINDING SCREEN <INACTIVE> -- Due to autobind config being set");
      }

      config->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;

      ctrlm_controller_bind_config_t bind_config;
      bind_config.mode = CTRLM_PAIRING_MODE_ONE_TOUCH_AUTO_BIND;
      bind_config.data.autobind.enable         = (config->enable ? true : false);
      bind_config.data.autobind.pass_threshold = config->threshold_pass;
      bind_config.data.autobind.fail_threshold = config->threshold_fail;

      for(auto const &itr : g_ctrlm.networks) {
         if(config->network_id == itr.first || config->network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
            itr.second->binding_config_set(bind_config);
            config->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         }
      }
      // Set a timer to stop one touch autobind mode
      g_ctrlm.one_touch_autobind_timeout_tag = ctrlm_timeout_create(g_ctrlm.one_touch_autobind_timeout_val, ctrlm_timeout_one_touch_autobind, NULL);
   }
}

gboolean ctrlm_main_iarm_call_precommission_config_set(ctrlm_main_iarm_call_precommision_config_t *config) {
   if(config == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_precommision_config_t *msg = (ctrlm_main_queue_msg_main_precommision_config_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_precommision_config_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_PRECOMMISSION_CONFIG_SET;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->config            = config;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process PRECOMISSION_CONFIG_SET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_precommission_config_set_(ctrlm_main_iarm_call_precommision_config_t *config) {
   if(config->network_id != CTRLM_MAIN_NETWORK_ID_ALL && !ctrlm_network_id_is_valid(config->network_id)) {
      config->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("network id - Out of range %u", config->network_id);
      return;
   }
   if(config->controller_qty > CTRLM_MAIN_MAX_BOUND_CONTROLLERS) {
      config->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("controller qty - Out of range %lu", config->controller_qty);
      return;
   }
   unsigned long index;

   for(index = 0; index < config->controller_qty; index++) {
      g_ctrlm.precomission_table[config->controllers[index]] = true;
      XLOGD_INFO("Adding 0x%016llX", config->controllers[index]);
   }
   config->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
}

gboolean ctrlm_main_iarm_call_factory_reset(ctrlm_main_iarm_call_factory_reset_t *reset) {
   if(reset == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_factory_reset_t *msg = (ctrlm_main_queue_msg_main_factory_reset_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_factory_reset_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_FACTORY_RESET;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->reset             = reset;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process FACTORY_RESET request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_factory_reset_(ctrlm_main_iarm_call_factory_reset_t *reset) {
   if(reset->network_id != CTRLM_MAIN_NETWORK_ID_ALL && !ctrlm_network_id_is_valid(reset->network_id)) {
      reset->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("network id - Out of range %u", reset->network_id);
      return;
   }

   for(auto const &itr : g_ctrlm.networks) {
      if(reset->network_id == itr.first || reset->network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
         itr.second->factory_reset();
      }
   }

   ctrlm_recovery_factory_reset();

   reset->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
}

gboolean ctrlm_main_iarm_call_controller_unbind(ctrlm_main_iarm_call_controller_unbind_t *unbind) {
   if(unbind == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_controller_unbind_t *msg = (ctrlm_main_queue_msg_main_controller_unbind_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_controller_unbind_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

  sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROLLER_UNBIND;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->unbind            = unbind;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process CONTROLLER_UNBIND request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_controller_unbind_(ctrlm_main_iarm_call_controller_unbind_t *unbind) {
   if(!ctrlm_network_id_is_valid(unbind->network_id)) {
      unbind->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("network id - Out of range %u", unbind->network_id);
      return;
   }

   ctrlm_obj_network_t *obj_net = g_ctrlm.networks[unbind->network_id];

   obj_net->controller_unbind(unbind->controller_id, CTRLM_UNBIND_REASON_TARGET_USER);
   unbind->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
}

gboolean ctrlm_main_iarm_call_chip_status_get(ctrlm_main_iarm_call_chip_status_t *status) {
   if(status == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_chip_status_t msg = {0};

   sem_init(&semaphore, 0, 0);

   msg.status            = status;
   msg.status->result    = CTRLM_IARM_CALL_RESULT_ERROR;
   msg.semaphore         = &semaphore;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_chip_status, &msg, sizeof(msg), NULL, status->network_id);

   // Wait for the result semaphore to be signaled
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   return(true);
}

void ctrlm_event_handler_ir(const char *owner, IARM_EventId_t event_id, void *data, size_t len)
{
   IARM_Bus_IRMgr_EventData_t *ir_event = (IARM_Bus_IRMgr_EventData_t *)data;

   switch(ir_event->data.irkey.keySrc) {
      case IARM_BUS_IRMGR_KEYSRC_IR: {
         int key_code = ir_event->data.irkey.keyCode;
         int key_type = ir_event->data.irkey.keyType;
         if(key_code == KED_SETUP && key_type == KET_KEYDOWN) { // Received IR Line of sight code
            XLOGD_INFO("Setup IR code received.");
            // Cancel active line of sight timer (if active)
            ctrlm_timeout_destroy(&g_ctrlm.line_of_sight_timeout_tag);

            if(!g_ctrlm.line_of_sight) {
               ctrlm_main_iarm_event_binding_line_of_sight(TRUE);
            }
            // Set line of sight as active
            g_ctrlm.line_of_sight = TRUE;

            // Set a timer to clear the line of sight after a period of time
            g_ctrlm.line_of_sight_timeout_tag = ctrlm_timeout_create(g_ctrlm.line_of_sight_timeout_val, ctrlm_timeout_line_of_sight, NULL);
            return;
         } else if(key_code == KED_RF_PAIR_GHOST && key_type == KET_KEYDOWN) { // Received IR autobinding ghost code
            XLOGD_INFO("Autobind ghost code received.");
            // Cancel active autobind timer (if active)
            ctrlm_timeout_destroy(&g_ctrlm.autobind_timeout_tag);

            if(!g_ctrlm.autobind) {
               ctrlm_main_iarm_event_autobind_line_of_sight(TRUE);
            }
            // Set autobind as active
            g_ctrlm.autobind = TRUE;

            // Set a timer to clear the autobind after a period of time
            g_ctrlm.autobind_timeout_tag = ctrlm_timeout_create(g_ctrlm.autobind_timeout_val, ctrlm_timeout_autobind, NULL);
            return;
         }
      break;
      }
      case IARM_BUS_IRMGR_KEYSRC_FP: {
         if(0) { // TODO Pairing button was pressed
            // Cancel active line of sight timer (if active)
            ctrlm_timeout_destroy(&g_ctrlm.binding_button_timeout_tag);

            if(!g_ctrlm.binding_button) {
               ctrlm_main_iarm_event_binding_button(TRUE);
            }
            // Set button binding as active
            g_ctrlm.binding_button = TRUE;

            // Set a timer to clear the line of sight after a period of time
            g_ctrlm.binding_button_timeout_tag = ctrlm_timeout_create(g_ctrlm.binding_button_timeout_val, ctrlm_timeout_binding_button, NULL);
         }
         return;
      }
      case IARM_BUS_IRMGR_KEYSRC_RF: {
         XLOGD_DEBUG("RF key source received.");
         break;
      }
      default: {
         XLOGD_WARN("Other key source received. 0x%08x", ir_event->data.irkey.keySrc);
         break;
      }
   }

   //Listen for control events
   if(event_id == IARM_BUS_IRMGR_EVENT_CONTROL)
   {
      IARM_Bus_IRMgr_EventData_t *ir_event_data           = (IARM_Bus_IRMgr_EventData_t*)data;
      int key_code                                        = ir_event_data->data.irkey.keyCode;
      int key_src                                         = ir_event_data->data.irkey.keySrc;
      int key_tag                                         = ir_event_data->data.irkey.keyTag;
      int controller_id                                   = ir_event_data->data.irkey.keySourceId;
      bool need_to_update_db                              = false;
      bool need_to_update_last_key_info                   = false;
      bool send_on_control_event                          = false;
      ctrlm_ir_remote_type old_remote_type                = g_ctrlm.last_key_info.last_ir_remote_type;
      ctrlm_remote_keypad_config old_remote_keypad_config = g_ctrlm.last_key_info.remote_keypad_config;
      guchar last_source_type                             = g_ctrlm.last_key_info.source_type;
      gboolean  write_last_key_info                       = false;
      char last_source_name[CTRLM_RCU_RIB_ATTR_LEN_PRODUCT_NAME];
      char source_name[CTRLM_RCU_RIB_ATTR_LEN_PRODUCT_NAME];
      ctrlm_remote_keypad_config remote_keypad_config;
      string type;
      string str_data;
      errno_t safec_rc = -1;
      int ind = -1;

      safec_rc = strcpy_s(last_source_name, sizeof(last_source_name), g_ctrlm.last_key_info.source_name);
      ERR_CHK(safec_rc);

      if ((key_src != IARM_BUS_IRMGR_KEYSRC_IR) && (key_src != IARM_BUS_IRMGR_KEYSRC_RF)) {
         // NOTE: For now, we are explicitly ignoring keypresses from IP or FP sources!
         return;
      }

      //Check for day change
      need_to_update_db = ctrlm_main_handle_day_change_ir_remote_usage();

      if (len != sizeof(IARM_Bus_IRMgr_EventData_t)) {
         XLOGD_ERROR("ERROR - Got IARM_BUS_IRMGR_EVENT_CONTROL event with bad data length <%u>, should be <%u>!!", len, sizeof(IARM_Bus_IRMgr_EventData_t));
         return;
      }

      switch(key_code) {
         case KED_XR2V3:
         case KED_XR5V2:
         case KED_XR11V1:
         case KED_XR11V2: {
            g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_XR11V2;
            g_ctrlm.last_key_info.remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_HAS_SETUP_KEY_WITH_NUMBER_KEYS;
            g_ctrlm.last_key_info.source_type          = IARM_BUS_IRMGR_KEYSRC_IR;
            g_ctrlm.last_key_info.is_screen_bind_mode  = false;
            type                                       = "irRemote";
            str_data                                   = std::to_string(g_ctrlm.last_key_info.remote_keypad_config);
            send_on_control_event                      = true;
            need_to_update_last_key_info               = true;
            safec_rc = strncpy_s(source_name, sizeof(source_name), ctrlm_rcu_ir_remote_types_str(g_ctrlm.last_key_info.last_ir_remote_type), CTRLM_RCU_RIB_ATTR_LEN_PRODUCT_NAME - 1);
            ERR_CHK(safec_rc);
            source_name[CTRLM_RCU_RIB_ATTR_LEN_PRODUCT_NAME - 1] = '\0';
            switch(key_code) {
               case KED_XR2V3:
                  if(!g_ctrlm.ir_remote_usage_today.has_ir_xr2) {
                     g_ctrlm.ir_remote_usage_today.has_ir_xr2 = true;
                     need_to_update_db = true;
                  }
                  break;
               case KED_XR5V2:
                  if(!g_ctrlm.ir_remote_usage_today.has_ir_xr5) {
                     g_ctrlm.ir_remote_usage_today.has_ir_xr5 = true;
                     need_to_update_db = true;
                  }
                  break;
               case KED_XR11V1:
               case KED_XR11V2:
                  if(!g_ctrlm.ir_remote_usage_today.has_ir_xr11) {
                     g_ctrlm.ir_remote_usage_today.has_ir_xr11 = true;
                     need_to_update_db = true;
                  }
                  break;
            }
            break;
         }

         case KED_XR11_NOTIFY:
            g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_XR11V2;
            g_ctrlm.last_key_info.remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_HAS_SETUP_KEY_WITH_NUMBER_KEYS;
            g_ctrlm.last_key_info.source_type          = IARM_BUS_IRMGR_KEYSRC_IR;
            g_ctrlm.last_key_info.is_screen_bind_mode  = false;
            XLOGD_INFO("Received KED_XR11_NOTIFY");
            if((!g_ctrlm.ir_remote_usage_today.has_ir_remote) || (old_remote_type != g_ctrlm.last_key_info.last_ir_remote_type) || (old_remote_keypad_config != g_ctrlm.last_key_info.remote_keypad_config)) {
               //Don't use has_ir_xr11 because this could be XR2 or XR5.  Use has_ir_remote.
               g_ctrlm.ir_remote_usage_today.has_ir_remote = true;
               need_to_update_db = true;
            }
            break;
         case KED_XR15V1_NOTIFY:
            g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_XR15V1;
            g_ctrlm.last_key_info.remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_HAS_NO_SETUP_KEY_WITH_NUMBER_KEYS;
            g_ctrlm.last_key_info.source_type          = IARM_BUS_IRMGR_KEYSRC_IR;
            XLOGD_INFO("Received KED_XR15V1_NOTIFY");
            if((!g_ctrlm.ir_remote_usage_today.has_ir_xr15) || (old_remote_type != g_ctrlm.last_key_info.last_ir_remote_type) || (old_remote_keypad_config != g_ctrlm.last_key_info.remote_keypad_config)) {
               g_ctrlm.ir_remote_usage_today.has_ir_xr15 = true;
               need_to_update_db = true;
            }
            break;
         case KED_XR16V1_NOTIFY:
            g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_NA;
            g_ctrlm.last_key_info.remote_keypad_config = CTRLM_REMOTE_KEYPAD_CONFIG_HAS_NO_SETUP_KEY_WITH_NO_NUMBER_KEYS;
            g_ctrlm.last_key_info.source_type          = IARM_BUS_IRMGR_KEYSRC_IR;
            XLOGD_INFO("Received KED_XR16V1_NOTIFY");
            if((!g_ctrlm.ir_remote_usage_today.has_ir_remote) || (old_remote_type != g_ctrlm.last_key_info.last_ir_remote_type) || (old_remote_keypad_config != g_ctrlm.last_key_info.remote_keypad_config)) {
               g_ctrlm.ir_remote_usage_today.has_ir_remote = true;
               need_to_update_db = true;
            }
            break;
         case KED_SCREEN_BIND_NOTIFY:
            g_ctrlm.last_key_info.is_screen_bind_mode = true;
            XLOGD_INFO("Received KED_SCREEN_BIND_NOTIFY");
            break;
         case KED_VOLUMEUP:
         case KED_VOLUMEDOWN:
         case KED_MUTE:
         case KED_INPUTKEY:
         case KED_POWER:
         case KED_TVPOWER:
         case KED_RF_POWER:
         case KED_DISCRETE_POWER_ON:
         case KED_DISCRETE_POWER_STANDBY: {
            send_on_control_event        = true;
            need_to_update_last_key_info = true;
            type                         = "TV";
            break;
         }
         case KED_PUSH_TO_TALK:
            send_on_control_event        = true;
            need_to_update_last_key_info = true;
            type                         = "mic";
            break;
         default:
            break;
      }

      if(key_src == IARM_BUS_IRMGR_KEYSRC_RF) {
         ctrlm_controller_product_name_get(controller_id, source_name);
         remote_keypad_config = ctrlm_get_remote_keypad_config(source_name);
      } else {
         remote_keypad_config = g_ctrlm.last_key_info.remote_keypad_config;
         controller_id = -1;
         //Check to see if the tag was included.  If so, use it.
         ctrlm_check_for_key_tag(key_tag);
         safec_rc = strcpy_s(source_name, sizeof(source_name), ctrlm_rcu_ir_remote_types_str(g_ctrlm.last_key_info.last_ir_remote_type));
         ERR_CHK(safec_rc);
      }

      XLOGD_INFO("Got IARM_BUS_IRMGR_EVENT_CONTROL event, controller_id <%d>, controlCode <0x%02X>, src <%d>.", controller_id, (unsigned int)key_code, key_src);

      //Only save the last key info to the db if the source type (IR or RF) or the source name (XR11, XR15) have changed
      //It's not worth the writes to the db for every key.  It is important to know if we are IR/RF and XR11/XR15
      bool isValid = false;
      (last_source_type != key_src) ? isValid = true : (safec_rc = strcmp_s(last_source_name, sizeof(last_source_name), source_name, &ind));
      if((isValid) || ((safec_rc == EOK) && (ind != 0))) {
          write_last_key_info = true;
      } else if(safec_rc != EOK) {
        ERR_CHK(safec_rc);
      }

      if(send_on_control_event) {
         str_data = std::to_string(remote_keypad_config);
         ctrlm_rcu_iarm_event_control(controller_id, key_source_names[key_src], type.c_str(), str_data.c_str(), key_code, 0);
      }

      if(need_to_update_last_key_info) {
         ctrlm_update_last_key_info(controller_id, key_src, key_code, source_name, g_ctrlm.last_key_info.is_screen_bind_mode, write_last_key_info);
      }

      if(need_to_update_db) {
         ctrlm_property_write_ir_remote_usage();
      }
   }
   //Listen for key events to keep track of last key info 
   else if (event_id == IARM_BUS_IRMGR_EVENT_IRKEY)
   {
      IARM_Bus_IRMgr_EventData_t *ir_event_data = (IARM_Bus_IRMgr_EventData_t*)data;
      int      key_code                              = ir_event_data->data.irkey.keyCode;
      int      key_src                               = ir_event_data->data.irkey.keySrc;
      int      key_tag                               = ir_event_data->data.irkey.keyTag;
      int      controller_id                         = ir_event_data->data.irkey.keySourceId;
      guchar   last_source_type                      = g_ctrlm.last_key_info.source_type;
      gboolean  write_last_key_info                  = false;
      char last_source_name[CTRLM_RCU_RIB_ATTR_LEN_PRODUCT_NAME];
      char source_name[CTRLM_RCU_RIB_ATTR_LEN_PRODUCT_NAME];

      errno_t safec_rc = -1;
      int ind = -1;

      safec_rc = strcpy_s(last_source_name, sizeof(last_source_name), g_ctrlm.last_key_info.source_name);
      ERR_CHK(safec_rc);

      if ((ir_event_data->data.irkey.keyType == KET_KEYUP) || (ir_event_data->data.irkey.keyType == KET_KEYREPEAT)) {
         // Don't remember keyup or repeat events - only use keydown.
         return;
      }

      if ((key_src != IARM_BUS_IRMGR_KEYSRC_IR) && (key_src != IARM_BUS_IRMGR_KEYSRC_RF)) {
         // NOTE: For now, we are explicitly ignoring keypresses from IP or FP sources!
         return;
      }

      if (key_src != IARM_BUS_IRMGR_KEYSRC_RF) {
         controller_id = -1;
      }

      if(key_src == IARM_BUS_IRMGR_KEYSRC_RF) {
         ctrlm_controller_product_name_get(controller_id, source_name);
      } else {
         controller_id = -1;
         //Check to see if the tag was included.  If so, use it.
         ctrlm_check_for_key_tag(key_tag);
         safec_rc = strcpy_s(source_name, sizeof(source_name), ctrlm_rcu_ir_remote_types_str(g_ctrlm.last_key_info.last_ir_remote_type));
         ERR_CHK(safec_rc);
      }

      if (len != sizeof(IARM_Bus_IRMgr_EventData_t)) {
         XLOGD_ERROR("ERROR - Got IARM_BUS_IRMGR_EVENT_IRKEY event with bad data length <%u>, should be <%u>!!", len, sizeof(IARM_Bus_IRMgr_EventData_t));
         return;
      }

      //Only save the last key info to the db if the source type (IR or RF) or the source name (XR11, XR15) have changed
      //It's not worth the writes to the db for every key.  It is important to know if we are IR/RF and XR11/XR15
      bool isValid = false;
      (last_source_type != key_src) ? isValid = true : (safec_rc = strcmp_s(last_source_name, sizeof(last_source_name), source_name, &ind));
      if((isValid) || ((safec_rc == EOK) && (ind != 0))) {
          write_last_key_info = true;
      } else if(safec_rc != EOK) {
        ERR_CHK(safec_rc);
      }

      ctrlm_update_last_key_info(controller_id, key_src, key_code, source_name, g_ctrlm.last_key_info.is_screen_bind_mode, write_last_key_info);
   }
}

void ctrlm_check_for_key_tag(int key_tag) {
   switch(key_tag) {
      case XMP_TAG_PLATCO: {
         g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_PLATCO;
         break;
      }
      case XMP_TAG_XR11V2: {
         g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_XR11V2;
         break;
      }
      case XMP_TAG_XR15V1: {
         g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_XR15V1;
         break;
      }
      case XMP_TAG_XR15V2: {
         g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_XR15V2;
         break;
      }
      case XMP_TAG_XR16V1: {
         g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_XR16V1;
         break;
      }
      case XMP_TAG_XRAV1: {
         g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_XRAV1;
         break;
      }
      case XMP_TAG_XR20V1: {
         g_ctrlm.last_key_info.last_ir_remote_type  = CTRLM_IR_REMOTE_TYPE_XR20V1;
         break;
      }
      case XMP_TAG_COMCAST:
      case XMP_TAG_UNDEFINED:
      default: {
         break;
      }
   }
   if(key_tag != XMP_TAG_COMCAST) {
      XLOGD_DEBUG("key_tag <%s>", ctrlm_rcu_ir_remote_types_str(g_ctrlm.last_key_info.last_ir_remote_type));
   }
}

gboolean ctrlm_timeout_line_of_sight(gpointer user_data) {
   XLOGD_INFO("Timeout - Line of sight.");
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      g_ctrlm.line_of_sight_timeout_tag = 0;
      return(FALSE);
   }

   msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_LINE_OF_SIGHT;
   msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;

   ctrlm_main_queue_msg_push(msg);
   g_ctrlm.line_of_sight_timeout_tag = 0;
   return(FALSE);
}

gboolean ctrlm_timeout_autobind(gpointer user_data) {
   XLOGD_TELEMETRY("Timeout - Autobind.");
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      g_ctrlm.autobind_timeout_tag = 0;
      return(FALSE);
   }

   msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_TIMEOUT_AUTOBIND;
   msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;

   ctrlm_main_queue_msg_push(msg);
   g_ctrlm.autobind_timeout_tag = 0;
   return(FALSE);
}

gboolean ctrlm_timeout_binding_button(gpointer user_data) {
   XLOGD_INFO("Timeout - Binding button.");
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_close_pairing_window_t *msg = (ctrlm_main_queue_msg_close_pairing_window_t *)g_malloc(sizeof(ctrlm_main_queue_msg_close_pairing_window_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      g_ctrlm.binding_button_timeout_tag = 0;
      return(FALSE);
   }

   msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CLOSE_PAIRING_WINDOW;
   msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->reason     = CTRLM_CLOSE_PAIRING_WINDOW_REASON_TIMEOUT;

   ctrlm_main_queue_msg_push(msg);
   g_ctrlm.binding_button_timeout_tag = 0;
   return(FALSE);
}

gboolean ctrlm_timeout_screen_bind(gpointer user_data) {
   XLOGD_INFO("Timeout - Screen Bind.");
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_close_pairing_window_t *msg = (ctrlm_main_queue_msg_close_pairing_window_t *)g_malloc(sizeof(ctrlm_main_queue_msg_close_pairing_window_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      g_ctrlm.screen_bind_timeout_tag = 0;
      return(FALSE);
   }

   msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CLOSE_PAIRING_WINDOW;
   msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->reason     = CTRLM_CLOSE_PAIRING_WINDOW_REASON_TIMEOUT;

   ctrlm_main_queue_msg_push(msg);
   g_ctrlm.screen_bind_timeout_tag = 0;
   return(FALSE);
}

gboolean ctrlm_timeout_one_touch_autobind(gpointer user_data) {
   XLOGD_INFO("Timeout - One touch Autobind.");
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_close_pairing_window_t *msg = (ctrlm_main_queue_msg_close_pairing_window_t *)g_malloc(sizeof(ctrlm_main_queue_msg_close_pairing_window_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      g_ctrlm.one_touch_autobind_timeout_tag = 0;
      return(FALSE);
   }

   msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CLOSE_PAIRING_WINDOW;
   msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->reason     = CTRLM_CLOSE_PAIRING_WINDOW_REASON_TIMEOUT;

   ctrlm_main_queue_msg_push(msg);
   g_ctrlm.one_touch_autobind_timeout_tag = 0;
   return(FALSE);
}

void ctrlm_stop_binding_button(void) {
   XLOGD_INFO("Binding button stopped.");
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return;
   }

   msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_BUTTON;
   msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;

   ctrlm_main_queue_msg_push(msg);
}

void ctrlm_stop_binding_screen(void) {
   XLOGD_INFO("Screen bind mode stopped.");
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return;
   }

   msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_BINDING_SCREEN;
   msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;

   ctrlm_main_queue_msg_push(msg);
}

void ctrlm_stop_one_touch_autobind(void) {
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return;
   }

   msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_STOP_ONE_TOUCH_AUTOBIND;
   msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;

   ctrlm_main_queue_msg_push(msg);
}

void ctrlm_close_pairing_window(ctrlm_close_pairing_window_reason reason) {
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_close_pairing_window_t *msg = (ctrlm_main_queue_msg_close_pairing_window_t *)g_malloc(sizeof(ctrlm_main_queue_msg_close_pairing_window_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return;
   }

   msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CLOSE_PAIRING_WINDOW;
   msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->reason     = reason;

   ctrlm_main_queue_msg_push(msg);
}

void ctrlm_pairing_window_bind_status_set(ctrlm_bind_status_t bind_status) {
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_pairing_window_bind_status_t *msg = (ctrlm_main_queue_msg_pairing_window_bind_status_t *)g_malloc(sizeof(ctrlm_main_queue_msg_pairing_window_bind_status_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      g_ctrlm.binding_button_timeout_tag = 0;
      return;
   }

   msg->type        = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_BIND_STATUS_SET;
   msg->bind_status = bind_status;

   ctrlm_main_queue_msg_push(msg);
}

void ctrlm_discovery_remote_type_set(const char *remote_type_str) {
   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_discovery_remote_type_t *msg = (ctrlm_main_queue_msg_discovery_remote_type_t *)g_malloc(sizeof(ctrlm_main_queue_msg_discovery_remote_type_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return;
   }

   msg->type            = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_DISCOVERY_REMOTE_TYPE_SET;
   errno_t safec_rc = strcpy_s(msg->remote_type_str, sizeof(msg->remote_type_str), remote_type_str);
   ERR_CHK(safec_rc);

   ctrlm_main_queue_msg_push(msg);
}

const char* ctrlm_minidump_path_get() {
    return g_ctrlm.minidump_path.c_str();
}

void ctrlm_main_sat_enabled_set(gboolean sat_enabled) {
   g_ctrlm.sat_enabled = sat_enabled;
}

void ctrlm_main_invalidate_service_access_token(void) {
   if(g_ctrlm.sat_enabled) {
      sem_wait(&g_ctrlm.service_access_token_semaphore);
      // We want to poll for SAT token whether we have it or not
      XLOGD_INFO("Invalidating SAT Token...");
      g_ctrlm.has_service_access_token = false;
      #ifdef AUTH_ENABLED
      ctrlm_timeout_destroy(&g_ctrlm.service_access_token_expiration_tag);
      ctrlm_timeout_destroy(&g_ctrlm.authservice_poll_tag);
      g_ctrlm.authservice_poll_tag = ctrlm_timeout_create(g_ctrlm.recently_booted ? g_ctrlm.authservice_fast_poll_val : g_ctrlm.authservice_poll_val,
                                                               ctrlm_authservice_poll,
                                                               NULL);
      #endif
      sem_post(&g_ctrlm.service_access_token_semaphore);
   }
}

gboolean ctrlm_main_successful_init_get(void) {
   return(g_ctrlm.successful_init);
}

ctrlm_irdb_t* ctrlm_main_irdb_get() {
#ifdef IRDB_ENABLED
   return(g_ctrlm.irdb);
#else
   return(NULL);
#endif
}

ctrlm_auth_t* ctrlm_main_auth_get() {
#ifdef AUTH_ENABLED
   return(g_ctrlm.authservice);
#else
   return(NULL);
#endif
}

string ctrlm_receiver_id_get(){
   return g_ctrlm.receiver_id;
}

string ctrlm_device_id_get(){
   return g_ctrlm.device_id;
}

ctrlm_device_type_t ctrlm_device_type_get(){
   return g_ctrlm.device_type;
}

string ctrlm_stb_name_get(){
   return g_ctrlm.stb_name;
}

string ctrlm_device_mac_get() {
   return g_ctrlm.device_mac;
}

gboolean ctrlm_main_handle_day_change_ir_remote_usage() {
   time_t time_in_seconds = time(NULL);
   if(time_in_seconds < g_ctrlm.shutdown_time) {
      XLOGD_WARN("Current Time <%ld> is less than the last shutdown time <%ld>.  Wait until time updates.", time_in_seconds, g_ctrlm.shutdown_time);
      return(false);
   }
   guint32 today = time_in_seconds / (60 * 60 * 24);
   guint32 day_change = today - g_ctrlm.today;
   XLOGD_INFO("today <%u> g_ctrlm.today <%u>.", today, g_ctrlm.today);

   //If this is a different day...
   if(day_change != 0) {

      //If this is the next day...
      if(day_change == 1) {
         g_ctrlm.ir_remote_usage_yesterday.has_ir_xr2    = g_ctrlm.ir_remote_usage_today.has_ir_xr2;
         g_ctrlm.ir_remote_usage_yesterday.has_ir_xr5    = g_ctrlm.ir_remote_usage_today.has_ir_xr5;
         g_ctrlm.ir_remote_usage_yesterday.has_ir_xr11   = g_ctrlm.ir_remote_usage_today.has_ir_xr11;
         g_ctrlm.ir_remote_usage_yesterday.has_ir_xr15   = g_ctrlm.ir_remote_usage_today.has_ir_xr15;
         g_ctrlm.ir_remote_usage_yesterday.has_ir_remote = g_ctrlm.ir_remote_usage_today.has_ir_remote;
      } else {
         g_ctrlm.ir_remote_usage_yesterday.has_ir_xr2    = false;
         g_ctrlm.ir_remote_usage_yesterday.has_ir_xr5    = false;
         g_ctrlm.ir_remote_usage_yesterday.has_ir_xr11   = false;
         g_ctrlm.ir_remote_usage_yesterday.has_ir_xr15   = false;
         g_ctrlm.ir_remote_usage_yesterday.has_ir_remote = false;
      }

      g_ctrlm.ir_remote_usage_today.has_ir_xr2    = false;
      g_ctrlm.ir_remote_usage_today.has_ir_xr5    = false;
      g_ctrlm.ir_remote_usage_today.has_ir_xr11   = false;
      g_ctrlm.ir_remote_usage_today.has_ir_xr15   = false;
      g_ctrlm.ir_remote_usage_today.has_ir_remote = false;
      g_ctrlm.today = today;

      return(true);
   }

   return(false);
}

void ctrlm_property_write_ir_remote_usage(void) {
   guchar data[CTRLM_RF4CE_LEN_IR_REMOTE_USAGE];
   data[0]  = (guchar)(g_ctrlm.ir_remote_usage_today.has_ir_xr2);
   data[1]  = (guchar)(g_ctrlm.ir_remote_usage_today.has_ir_xr5);
   data[2]  = (guchar)(g_ctrlm.ir_remote_usage_today.has_ir_xr11);
   data[3]  = (guchar)(g_ctrlm.ir_remote_usage_today.has_ir_xr15);
   data[4]  = (guchar)(g_ctrlm.ir_remote_usage_today.has_ir_remote);
   data[5]  = (guchar)(g_ctrlm.ir_remote_usage_yesterday.has_ir_xr2);
   data[6]  = (guchar)(g_ctrlm.ir_remote_usage_yesterday.has_ir_xr5);
   data[7]  = (guchar)(g_ctrlm.ir_remote_usage_yesterday.has_ir_xr11);
   data[8]  = (guchar)(g_ctrlm.ir_remote_usage_yesterday.has_ir_xr15);
   data[9]  = (guchar)(g_ctrlm.ir_remote_usage_yesterday.has_ir_remote);
   data[10] = (guchar)(g_ctrlm.today);
   data[11] = (guchar)(g_ctrlm.today >> 8);
   data[12] = (guchar)(g_ctrlm.today >> 16);
   data[13] = (guchar)(g_ctrlm.today >> 24);

   XLOGD_INFO("Has XR2  IR today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_xr2, g_ctrlm.ir_remote_usage_yesterday.has_ir_xr2);
   XLOGD_INFO("Has XR5  IR today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_xr5, g_ctrlm.ir_remote_usage_yesterday.has_ir_xr5);
   XLOGD_INFO("Has XR11 IR today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_xr11, g_ctrlm.ir_remote_usage_yesterday.has_ir_xr11);
   XLOGD_INFO("Has XR15 IR today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_xr15, g_ctrlm.ir_remote_usage_yesterday.has_ir_xr15);
   XLOGD_INFO("Has IR Remote: today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_remote, g_ctrlm.ir_remote_usage_yesterday.has_ir_remote);

   ctrlm_db_ir_remote_usage_write(data, CTRLM_RF4CE_LEN_IR_REMOTE_USAGE);
}

guchar ctrlm_property_write_ir_remote_usage(guchar *data, guchar length) {
   if(data == NULL || length != CTRLM_RF4CE_LEN_IR_REMOTE_USAGE) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   ctrlm_ir_remote_usage ir_remote_usage_today;
   ctrlm_ir_remote_usage ir_remote_usage_yesterday;
   guint32               today;
   gboolean              ir_remote_usage_changed = false;
   gboolean              day_changed             = false;

   ir_remote_usage_today.has_ir_xr2        = data[0];
   ir_remote_usage_today.has_ir_xr5        = data[1];
   ir_remote_usage_today.has_ir_xr11       = data[2];
   ir_remote_usage_today.has_ir_xr15       = data[3];
   ir_remote_usage_today.has_ir_remote     = data[4];
   ir_remote_usage_yesterday.has_ir_xr2    = data[5];
   ir_remote_usage_yesterday.has_ir_xr5    = data[6];
   ir_remote_usage_yesterday.has_ir_xr11   = data[7];
   ir_remote_usage_yesterday.has_ir_xr15   = data[8];
   ir_remote_usage_yesterday.has_ir_remote = data[9];
   today                                   = ((data[13] << 24) | (data[12] << 16) | (data[11] << 8) | data[10]);
   
   if(g_ctrlm.ir_remote_usage_today.has_ir_xr2        != ir_remote_usage_today.has_ir_xr2       ||
      g_ctrlm.ir_remote_usage_yesterday.has_ir_xr2    != ir_remote_usage_yesterday.has_ir_xr2   ||
      g_ctrlm.ir_remote_usage_today.has_ir_xr5        != ir_remote_usage_today.has_ir_xr5       ||
      g_ctrlm.ir_remote_usage_yesterday.has_ir_xr5    != ir_remote_usage_yesterday.has_ir_xr5   ||
      g_ctrlm.ir_remote_usage_today.has_ir_xr11       != ir_remote_usage_today.has_ir_xr11      ||
      g_ctrlm.ir_remote_usage_yesterday.has_ir_xr11   != ir_remote_usage_yesterday.has_ir_xr11  ||
      g_ctrlm.ir_remote_usage_today.has_ir_xr15       != ir_remote_usage_today.has_ir_xr15      ||
      g_ctrlm.ir_remote_usage_yesterday.has_ir_xr15   != ir_remote_usage_yesterday.has_ir_xr15  ||
      g_ctrlm.ir_remote_usage_today.has_ir_remote     != ir_remote_usage_today.has_ir_remote    ||
      g_ctrlm.ir_remote_usage_yesterday.has_ir_remote != ir_remote_usage_yesterday.has_ir_remote ||
      g_ctrlm.today                                   != today) {
      // Store the data
      g_ctrlm.ir_remote_usage_today.has_ir_xr2         = ir_remote_usage_today.has_ir_xr2;
      g_ctrlm.ir_remote_usage_yesterday.has_ir_xr2     = ir_remote_usage_yesterday.has_ir_xr2;
      g_ctrlm.ir_remote_usage_today.has_ir_xr5         = ir_remote_usage_today.has_ir_xr5;
      g_ctrlm.ir_remote_usage_yesterday.has_ir_xr5     = ir_remote_usage_yesterday.has_ir_xr5;
      g_ctrlm.ir_remote_usage_today.has_ir_xr11        = ir_remote_usage_today.has_ir_xr11;
      g_ctrlm.ir_remote_usage_yesterday.has_ir_xr11    = ir_remote_usage_yesterday.has_ir_xr11;
      g_ctrlm.ir_remote_usage_today.has_ir_xr15        = ir_remote_usage_today.has_ir_xr15;
      g_ctrlm.ir_remote_usage_yesterday.has_ir_xr15    = ir_remote_usage_yesterday.has_ir_xr15;
      g_ctrlm.ir_remote_usage_today.has_ir_remote      = ir_remote_usage_today.has_ir_remote;
      g_ctrlm.ir_remote_usage_yesterday.has_ir_remote  = ir_remote_usage_yesterday.has_ir_remote;
      g_ctrlm.today                                    = today;
      ir_remote_usage_changed                          = true;
   }

   day_changed = ctrlm_main_handle_day_change_ir_remote_usage();

   if(day_changed || (!g_ctrlm.loading_db && ir_remote_usage_changed)) {
      ctrlm_property_write_ir_remote_usage();
   }

   return(length);
}

void ctrlm_property_read_ir_remote_usage(void) {
   guchar *data = NULL;
   guint32 length;
   ctrlm_db_ir_remote_usage_read(&data, &length);

   if(data == NULL) {
      XLOGD_WARN("Not read from DB - IR Remote Usage");
   } else {
      ctrlm_property_write_ir_remote_usage(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   XLOGD_INFO("Has XR2  IR today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_xr2, g_ctrlm.ir_remote_usage_yesterday.has_ir_xr2);
   XLOGD_INFO("Has XR5  IR today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_xr5, g_ctrlm.ir_remote_usage_yesterday.has_ir_xr5);
   XLOGD_INFO("Has XR11 IR today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_xr11, g_ctrlm.ir_remote_usage_yesterday.has_ir_xr11);
   XLOGD_INFO("Has XR15 IR today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_xr15, g_ctrlm.ir_remote_usage_yesterday.has_ir_xr15);
   XLOGD_INFO("Has IR Remote:  today <%d> yesterday <%d>", g_ctrlm.ir_remote_usage_today.has_ir_remote, g_ctrlm.ir_remote_usage_yesterday.has_ir_remote);
}

void ctrlm_property_write_pairing_metrics(void) {
   guchar data[CTRLM_RF4CE_LEN_PAIRING_METRICS];
   errno_t safec_rc = memcpy_s(data, sizeof(data), &g_ctrlm.pairing_metrics, CTRLM_RF4CE_LEN_PAIRING_METRICS);
   ERR_CHK(safec_rc);

   XLOGD_INFO("num_screenbind_failures <%lu>, last_screenbind_error_timestamp <%lums>, last_screenbind_error_code <%d>, last_screenbind_remote_type <%s>", g_ctrlm.pairing_metrics.num_screenbind_failures, g_ctrlm.pairing_metrics.last_screenbind_error_timestamp, g_ctrlm.pairing_metrics.last_screenbind_error_code, g_ctrlm.pairing_metrics.last_screenbind_remote_type);

   XLOGD_INFO("num_non_screenbind_failures <%lu>, last_non_screenbind_error_timestamp <%lums>, last_non_screenbind_error_code <%d>, last_non_screenbind_error_binding_type <%d> last_screenbind_remote_type <%s>", g_ctrlm.pairing_metrics.num_non_screenbind_failures, g_ctrlm.pairing_metrics.last_non_screenbind_error_timestamp, g_ctrlm.pairing_metrics.last_non_screenbind_error_code, g_ctrlm.pairing_metrics.last_non_screenbind_error_binding_type, g_ctrlm.pairing_metrics.last_non_screenbind_remote_type);

   ctrlm_db_pairing_metrics_write(data, CTRLM_RF4CE_LEN_PAIRING_METRICS);
}

guchar ctrlm_property_write_pairing_metrics(guchar *data, guchar length) {
   errno_t safec_rc = -1;
   int ind = -1;
   if(data == NULL || length != CTRLM_RF4CE_LEN_PAIRING_METRICS) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   ctrlm_pairing_metrics_t pairing_metrics;
   safec_rc = memcpy_s(&pairing_metrics, sizeof(ctrlm_pairing_metrics_t), data, CTRLM_RF4CE_LEN_PAIRING_METRICS);
   ERR_CHK(safec_rc);

   safec_rc = memcmp_s(&g_ctrlm.pairing_metrics, sizeof(ctrlm_pairing_metrics_t), &pairing_metrics, CTRLM_RF4CE_LEN_PAIRING_METRICS, &ind);
   ERR_CHK(safec_rc);
   if((safec_rc == EOK) && (ind != 0)) {
      safec_rc = memcpy_s(&g_ctrlm.pairing_metrics, sizeof(ctrlm_pairing_metrics_t), &pairing_metrics, CTRLM_RF4CE_LEN_PAIRING_METRICS);
      ERR_CHK(safec_rc);
   }

   if(!g_ctrlm.loading_db) {
      ctrlm_property_write_pairing_metrics();
   }
   return(length);
}

void ctrlm_property_read_pairing_metrics(void) {
   guchar *data = NULL;
   guint32 length;
   ctrlm_db_pairing_metrics_read(&data, &length);

   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Pairing Metrics");
   } else {
      ctrlm_property_write_pairing_metrics(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   XLOGD_INFO("num_screenbind_failures <%lu>, last_screenbind_error_timestamp <%lums>, last_screenbind_error_code <%d>, last_screenbind_remote_type <%s>", g_ctrlm.pairing_metrics.num_screenbind_failures, g_ctrlm.pairing_metrics.last_screenbind_error_timestamp, g_ctrlm.pairing_metrics.last_screenbind_error_code, g_ctrlm.pairing_metrics.last_screenbind_remote_type);

   XLOGD_INFO("num_non_screenbind_failures <%lu>, last_non_screenbind_error_timestamp <%lums>, last_non_screenbind_error_code <%d>, last_non_screenbind_error_binding_type <%d> last_screenbind_remote_type <%s>", g_ctrlm.pairing_metrics.num_non_screenbind_failures, g_ctrlm.pairing_metrics.last_non_screenbind_error_timestamp, g_ctrlm.pairing_metrics.last_non_screenbind_error_code, g_ctrlm.pairing_metrics.last_non_screenbind_error_binding_type, g_ctrlm.pairing_metrics.last_non_screenbind_remote_type);
}
void ctrlm_property_write_last_key_info(void) {
   guchar data[CTRLM_RF4CE_LEN_LAST_KEY_INFO];
   errno_t safec_rc = memcpy_s(data, sizeof(data), &g_ctrlm.last_key_info, CTRLM_RF4CE_LEN_LAST_KEY_INFO);
   ERR_CHK(safec_rc);

   if(ctrlm_is_pii_mask_enabled()) {
      if(g_ctrlm.last_key_info.controller_id == 0){
         XLOGD_TELEMETRY("controller_id <%d>", g_ctrlm.last_key_info.controller_id);
      } 
      XLOGD_INFO("controller_id <%d>, key_code <*>, key_src <%d>, timestamp <%lldms>, is_screen_bind_mode <%d>, remote_keypad_config <%d>, sourceName <%s>", g_ctrlm.last_key_info.controller_id, g_ctrlm.last_key_info.source_type, g_ctrlm.last_key_info.timestamp, g_ctrlm.last_key_info.is_screen_bind_mode, g_ctrlm.last_key_info.remote_keypad_config, g_ctrlm.last_key_info.source_name);
   } else {
      XLOGD_INFO("controller_id <%d>, key_code <%d>, key_src <%d>, timestamp <%lldms>, is_screen_bind_mode <%d>, remote_keypad_config <%d>, sourceName <%s>", g_ctrlm.last_key_info.controller_id, g_ctrlm.last_key_info.source_key_code, g_ctrlm.last_key_info.source_type, g_ctrlm.last_key_info.timestamp, g_ctrlm.last_key_info.is_screen_bind_mode, g_ctrlm.last_key_info.remote_keypad_config, g_ctrlm.last_key_info.source_name);
   }

   ctrlm_db_last_key_info_write(data, CTRLM_RF4CE_LEN_LAST_KEY_INFO);
}

guchar ctrlm_property_write_last_key_info(guchar *data, guchar length) {
   errno_t safec_rc = -1;
   int ind = -1;
   if(data == NULL || length != CTRLM_RF4CE_LEN_LAST_KEY_INFO) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   ctrlm_last_key_info last_key_info;
   safec_rc = memcpy_s(&last_key_info, sizeof(ctrlm_last_key_info), data, CTRLM_RF4CE_LEN_LAST_KEY_INFO);
   ERR_CHK(safec_rc);

   safec_rc = memcmp_s(&g_ctrlm.last_key_info, sizeof(ctrlm_last_key_info), &last_key_info, CTRLM_RF4CE_LEN_LAST_KEY_INFO, &ind);
   ERR_CHK(safec_rc);
   if((safec_rc == EOK) && (ind != 0)) {
      safec_rc = memcpy_s(&g_ctrlm.last_key_info, sizeof(ctrlm_last_key_info), &last_key_info, CTRLM_RF4CE_LEN_LAST_KEY_INFO);
      ERR_CHK(safec_rc);
   }

   if(!g_ctrlm.loading_db) {
      ctrlm_property_write_last_key_info();
   }
   return(length);
}

void ctrlm_property_read_last_key_info(void) {
   guchar *data = NULL;
   guint32 length;
   ctrlm_db_last_key_info_read(&data, &length);

   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Last Key Info");
   } else {
      ctrlm_property_write_last_key_info(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }

   //Reset is_screen_bind_mode
   g_ctrlm.last_key_info.is_screen_bind_mode = false;

   if(ctrlm_is_pii_mask_enabled()) {
      XLOGD_TELEMETRY("controller_id <%d>, key_code <*>, key_src <%d>, timestamp <%lldms>, is_screen_bind_mode <%d>, remote_keypad_config <%d>, sourceName <%s>",
       g_ctrlm.last_key_info.controller_id, g_ctrlm.last_key_info.source_type, g_ctrlm.last_key_info.timestamp, g_ctrlm.last_key_info.is_screen_bind_mode, g_ctrlm.last_key_info.remote_keypad_config, g_ctrlm.last_key_info.source_name);
   } else {
      XLOGD_TELEMETRY("controller_id <%d>, key_code <%d>, key_src <%d>, timestamp <%lldms>, is_screen_bind_mode <%d>, remote_keypad_config <%d>, sourceName <%s>",
       g_ctrlm.last_key_info.controller_id, g_ctrlm.last_key_info.source_key_code, g_ctrlm.last_key_info.source_type, g_ctrlm.last_key_info.timestamp, g_ctrlm.last_key_info.is_screen_bind_mode, g_ctrlm.last_key_info.remote_keypad_config, g_ctrlm.last_key_info.source_name);
   }
}

void ctrlm_update_last_key_info(int controller_id, guchar source_type, guint32 source_key_code, const char *source_name, gboolean is_screen_bind_mode, gboolean write_last_key_info) {
   // Get a epoch-based millisecond timestamp for this event
   long long key_time = time(NULL) * 1000LL;
   ctrlm_remote_keypad_config remote_keypad_config;
   errno_t safec_rc = -1;

   // Update the LastKeyInfo
   g_ctrlm.last_key_info.controller_id       = controller_id;
   g_ctrlm.last_key_info.source_type         = source_type;
   g_ctrlm.last_key_info.source_key_code     = source_key_code;
   g_ctrlm.last_key_info.timestamp           = key_time;
   g_ctrlm.last_key_info.is_screen_bind_mode = is_screen_bind_mode;
   if(source_name != NULL) {
      safec_rc = strcpy_s(g_ctrlm.last_key_info.source_name, sizeof(g_ctrlm.last_key_info.source_name), source_name);
      ERR_CHK(safec_rc);
   } else {
      safec_rc = strcpy_s(g_ctrlm.last_key_info.source_name, sizeof(g_ctrlm.last_key_info.source_name), "Invalid");
      ERR_CHK(safec_rc);
   }
   g_ctrlm.last_key_info.source_name[CTRLM_RCU_RIB_ATTR_LEN_PRODUCT_NAME - 1] = '\0';
#if 0
   //Do not update remote_keypad_config here as we want to keep what it was set to by an IR remote
   //and not clear it when a remote is paired.  We will override this to N/A for paired remotes
   //when the iarm call to get the last_key_info is called
   g_ctrlm.last_key_info.remote_keypad_config = remote_keypad_config;
#else
   if(source_type == IARM_BUS_IRMGR_KEYSRC_RF) {
      remote_keypad_config = ctrlm_get_remote_keypad_config(g_ctrlm.last_key_info.source_name);
      is_screen_bind_mode  = false;
   } else {
      remote_keypad_config = g_ctrlm.last_key_info.remote_keypad_config;
      is_screen_bind_mode  = g_ctrlm.last_key_info.is_screen_bind_mode;
   }
#endif

   if(write_last_key_info) {
      XLOGD_INFO("Writing last key info: controller_id <%d>, key_code <%d>, key_src <%d>, timestamp <%lldms>, is_screen_bind_mode <%d>, remote_keypad_config <%d>, sourceName <%s>", 
            g_ctrlm.last_key_info.controller_id, g_ctrlm.mask_pii ? -1 : g_ctrlm.last_key_info.source_key_code, g_ctrlm.last_key_info.source_type, 
            g_ctrlm.last_key_info.timestamp, is_screen_bind_mode, remote_keypad_config, g_ctrlm.last_key_info.source_name);

      ctrlm_property_write_last_key_info();
   }
}

gboolean ctrlm_main_iarm_call_control_service_set_values(ctrlm_main_iarm_call_control_service_settings_t *settings) {
   if(settings == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_control_service_settings_t *msg = (ctrlm_main_queue_msg_main_control_service_settings_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_control_service_settings_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_SET_VALUES;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->settings          = settings;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_SET_VALUES request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

gboolean ctrlm_main_iarm_call_control_service_get_values(ctrlm_main_iarm_call_control_service_settings_t *settings) {
   if(settings == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_control_service_settings_t *msg = (ctrlm_main_queue_msg_main_control_service_settings_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_control_service_settings_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_GET_VALUES;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->settings          = settings;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_GET_VALUES request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

gboolean ctrlm_main_iarm_call_control_service_can_find_my_remote(ctrlm_main_iarm_call_control_service_can_find_my_remote_t *can_find_my_remote) {
   if(can_find_my_remote == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_control_service_can_find_my_remote_t *msg = (ctrlm_main_queue_msg_main_control_service_can_find_my_remote_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_control_service_can_find_my_remote_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type        = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_CAN_FIND_MY_REMOTE;
   msg->header.network_id  = ctrlm_network_id_get(can_find_my_remote->network_type);
   msg->can_find_my_remote = can_find_my_remote;
   msg->semaphore          = &semaphore;
   msg->cmd_result         = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_CAN_FIND_MY_REMOTE request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

gboolean ctrlm_main_iarm_call_control_service_start_pairing_mode(ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing) {
   if(pairing == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_control_service_pairing_mode_t *msg = (ctrlm_main_queue_msg_main_control_service_pairing_mode_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_control_service_pairing_mode_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_START_PAIRING_MODE;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->pairing           = pairing;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_START_PAIRING_MODE request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

gboolean ctrlm_main_iarm_call_control_service_end_pairing_mode(ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing) {
   if(pairing == NULL) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }
   XLOGD_INFO("");

   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_main_control_service_pairing_mode_t *msg = (ctrlm_main_queue_msg_main_control_service_pairing_mode_t *)g_malloc(sizeof(ctrlm_main_queue_msg_main_control_service_pairing_mode_t));

   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   sem_init(&semaphore, 0, 0);

   msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_END_PAIRING_MODE;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->pairing           = pairing;
   msg->semaphore         = &semaphore;
   msg->cmd_result        = &cmd_result;

   ctrlm_main_queue_msg_push(msg);

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process CTRLM_MAIN_QUEUE_MSG_TYPE_MAIN_CONTROL_SERVICE_END_PAIRING_MODE request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result == CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      return(true);
   }
   return(false);
}

void ctrlm_main_iarm_call_control_service_start_pairing_mode_(ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing) {
   if(pairing->network_id != CTRLM_MAIN_NETWORK_ID_ALL && !ctrlm_network_id_is_valid(pairing->network_id)) {
      pairing->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("network id - Out of range %u", pairing->network_id);
      return;
   }

   /////////////////////////////////////////////////////
   //Stop any current pairing window and start a new one
   /////////////////////////////////////////////////////

   if(g_ctrlm.binding_button) {
      ctrlm_timeout_destroy(&g_ctrlm.binding_button_timeout_tag);
      g_ctrlm.binding_button = false;
      XLOGD_INFO("BINDING BUTTON <INACTIVE>");
   }
   if(g_ctrlm.binding_screen_active) {
      ctrlm_timeout_destroy(&g_ctrlm.screen_bind_timeout_tag);
      g_ctrlm.binding_screen_active = false;
      XLOGD_INFO("SCREEN BIND STATE <INACTIVE>");
   }
   if(g_ctrlm.one_touch_autobind_active) {
      ctrlm_stop_one_touch_autobind_(CTRLM_MAIN_NETWORK_ID_ALL);
      XLOGD_INFO("ONE TOUCH AUTOBIND <INACTIVE>");
   }
   g_ctrlm.pairing_window.active = true;
   g_ctrlm.pairing_window.pairing_mode = (ctrlm_pairing_modes_t)pairing->pairing_mode;
   g_ctrlm.pairing_window.restrict_by_remote = (ctrlm_pairing_restrict_by_remote_t)pairing->restrict_by_remote;
   ctrlm_pairing_window_bind_status_set_(CTRLM_BIND_STATUS_NO_DISCOVERY_REQUEST);
   g_ctrlm.autobind = false;

   switch(pairing->pairing_mode) {
      case CTRLM_PAIRING_MODE_BUTTON_BUTTON_BIND: {
         pairing->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         g_ctrlm.binding_button = true;
         // Set a timer to limit the binding mode window
         g_ctrlm.binding_button_timeout_tag = ctrlm_timeout_create(g_ctrlm.binding_button_timeout_val, ctrlm_timeout_binding_button, NULL);
         XLOGD_INFO("BINDING BUTTON <ACTIVE>");
         break;
      }
      case CTRLM_PAIRING_MODE_SCREEN_BIND: {
         pairing->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
         g_ctrlm.binding_screen_active = true;
         // Set a timer to limit the binding mode window
         g_ctrlm.screen_bind_timeout_tag = ctrlm_timeout_create(g_ctrlm.screen_bind_timeout_val, ctrlm_timeout_screen_bind, NULL);
         XLOGD_INFO("SCREEN BIND STATE <ACTIVE>");
         break;
      }
      case CTRLM_PAIRING_MODE_ONE_TOUCH_AUTO_BIND: {
         ctrlm_controller_bind_config_t config;
         g_ctrlm.one_touch_autobind_timeout_tag = ctrlm_timeout_create(g_ctrlm.one_touch_autobind_timeout_val, ctrlm_timeout_one_touch_autobind, NULL);
         g_ctrlm.one_touch_autobind_active = true;

         pairing->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
         config.mode = CTRLM_PAIRING_MODE_ONE_TOUCH_AUTO_BIND;
         config.data.autobind.enable         = true;
         config.data.autobind.pass_threshold = 1;
         config.data.autobind.fail_threshold = 5;

         for(auto const &itr : g_ctrlm.networks) {
            if(pairing->network_id == itr.first || pairing->network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
               itr.second->binding_config_set(config);
               pairing->result = CTRLM_IARM_CALL_RESULT_SUCCESS;
            }
         }
         break;
      }
      default: {
         XLOGD_ERROR("Invalid Pairing Mode %d", pairing->pairing_mode);
         pairing->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      }
   }
}

void ctrlm_main_iarm_call_control_service_end_pairing_mode_(ctrlm_main_iarm_call_control_service_pairing_mode_t *pairing) {
   if(pairing->network_id != CTRLM_MAIN_NETWORK_ID_ALL && !ctrlm_network_id_is_valid(pairing->network_id)) {
      pairing->result = CTRLM_IARM_CALL_RESULT_ERROR_INVALID_PARAMETER;
      XLOGD_ERROR("network id - Out of range %u", pairing->network_id);
      return;
   }
   ctrlm_close_pairing_window_(pairing->network_id, CTRLM_CLOSE_PAIRING_WINDOW_REASON_END);
   pairing->result       = CTRLM_IARM_CALL_RESULT_SUCCESS;
   pairing->pairing_mode = g_ctrlm.pairing_window.pairing_mode;
   pairing->bind_status  = g_ctrlm.pairing_window.bind_status;

   switch(g_ctrlm.pairing_window.pairing_mode) {
      case CTRLM_PAIRING_MODE_BUTTON_BUTTON_BIND: {
         XLOGD_INFO("BINDING BUTTON <INACTIVE>");
         break;
      }
      case CTRLM_PAIRING_MODE_SCREEN_BIND: {
         XLOGD_INFO("SCREEN BIND STATE <INACTIVE>");
         break;
      }
      case CTRLM_PAIRING_MODE_ONE_TOUCH_AUTO_BIND: {
         XLOGD_INFO("ONE TOUCH AUTOBIND STATE <INACTIVE>");
         break;
      }
      default: {
         XLOGD_ERROR("Invalid Pairing Mode %d", g_ctrlm.pairing_window.pairing_mode);
      }
   }
}

gboolean ctrlm_pairing_window_active_get() {
   return(g_ctrlm.pairing_window.active);
}

ctrlm_pairing_restrict_by_remote_t restrict_pairing_by_remote_get() {
   return(g_ctrlm.pairing_window.restrict_by_remote);
}

void ctrlm_stop_one_touch_autobind_(ctrlm_network_id_t network_id) {
   if (g_ctrlm.one_touch_autobind_active) {
      XLOGD_INFO("One Touch Autobind mode stopped.");
      ctrlm_timeout_destroy(&g_ctrlm.one_touch_autobind_timeout_tag);
      g_ctrlm.one_touch_autobind_active = FALSE;
      ctrlm_controller_bind_config_t config;
      config.mode = CTRLM_PAIRING_MODE_ONE_TOUCH_AUTO_BIND;
      config.data.autobind.enable         = false;
      config.data.autobind.pass_threshold = 3;
      config.data.autobind.fail_threshold = 5;
      for(auto const &itr : g_ctrlm.networks) {
         if(network_id == itr.first || network_id == CTRLM_MAIN_NETWORK_ID_ALL) {
            itr.second->binding_config_set(config);
         }
      }
   }
}

void ctrlm_close_pairing_window_(ctrlm_network_id_t network_id, ctrlm_close_pairing_window_reason reason) {
   errno_t safec_rc = -1;
   if(g_ctrlm.pairing_window.active) {
      g_ctrlm.pairing_window.active             = false;
      g_ctrlm.pairing_window.restrict_by_remote = CTRLM_PAIRING_RESTRICT_NONE;
      ctrlm_rf_pair_state_t rf_pair_state       = CTRLM_RF_PAIR_STATE_UNKNOWN;
      switch(reason) {
        case(CTRLM_CLOSE_PAIRING_WINDOW_REASON_PAIRING_SUCCESS): {
            ctrlm_pairing_window_bind_status_set_(CTRLM_BIND_STATUS_SUCCESS);
            rf_pair_state = CTRLM_RF_PAIR_STATE_PAIRING;
            break;
         }
         case(CTRLM_CLOSE_PAIRING_WINDOW_REASON_END): {
            g_ctrlm.autobind = true;
            rf_pair_state = CTRLM_RF_PAIR_STATE_FAILED;
            break;
         }
         case(CTRLM_CLOSE_PAIRING_WINDOW_REASON_TIMEOUT): {
            ctrlm_pairing_window_bind_status_set_(CTRLM_BIND_STATUS_BIND_WINDOW_TIMEOUT);
            g_ctrlm.autobind = true;
            rf_pair_state = CTRLM_RF_PAIR_STATE_FAILED;
            XLOGD_INFO("Result <%s>", ctrlm_rcu_validation_result_str(CTRLM_RCU_VALIDATION_RESULT_TIMEOUT));
            break;
         }
         default: {
            ctrlm_pairing_window_bind_status_set_(CTRLM_BIND_STATUS_NO_DISCOVERY_REQUEST);
            break;
         }
      }
      XLOGD_INFO("reason <%s>, bind status <%s>", ctrlm_close_pairing_window_reason_str(reason), ctrlm_bind_status_str(g_ctrlm.pairing_window.bind_status));

      std::map<ctrlm_network_id_t, ctrlm_obj_network_t *>::iterator network_itr;
      if (network_id != CTRLM_MAIN_NETWORK_ID_ALL) {
          network_itr = g_ctrlm.networks.find(network_id);
      } else {
          for (network_itr = g_ctrlm.networks.begin(); network_itr != g_ctrlm.networks.end(); network_itr++) {
              if (network_itr->second->type_get() == CTRLM_NETWORK_TYPE_RF4CE) {
                  break;
              }
          }
      }

      if (network_itr != g_ctrlm.networks.end()) {
          network_itr->second->set_rf_pair_state(rf_pair_state);
          network_itr->second->iarm_event_rcu_status();
      }
   }
   if (g_ctrlm.binding_button) {
      ctrlm_timeout_destroy(&g_ctrlm.binding_button_timeout_tag);
      g_ctrlm.binding_button = FALSE;
      //Save the non screen bind pairing metrics
      if(g_ctrlm.pairing_window.bind_status != CTRLM_BIND_STATUS_SUCCESS) {
         g_ctrlm.pairing_metrics.num_non_screenbind_failures++;
         g_ctrlm.pairing_metrics.last_non_screenbind_error_timestamp = time(NULL);
         g_ctrlm.pairing_metrics.last_non_screenbind_error_code = g_ctrlm.pairing_window.bind_status;
         g_ctrlm.pairing_metrics.last_non_screenbind_error_binding_type = CTRLM_RCU_BINDING_TYPE_BUTTON;
         if(g_ctrlm.pairing_window.bind_status != CTRLM_BIND_STATUS_NO_DISCOVERY_REQUEST) {
            safec_rc = strcpy_s(g_ctrlm.pairing_metrics.last_non_screenbind_remote_type, sizeof(g_ctrlm.pairing_metrics.last_non_screenbind_remote_type), g_ctrlm.discovery_remote_type);
            ERR_CHK(safec_rc);
         } else {
            g_ctrlm.pairing_metrics.last_non_screenbind_remote_type[0] = '\0';
         }
         ctrlm_property_write_pairing_metrics();
      }
   }
   if (g_ctrlm.binding_screen_active) {
      ctrlm_timeout_destroy(&g_ctrlm.screen_bind_timeout_tag);
      g_ctrlm.binding_screen_active = FALSE;
      //Save the screen bind pairing metrics
      if(g_ctrlm.pairing_window.bind_status != CTRLM_BIND_STATUS_SUCCESS) {
         g_ctrlm.pairing_metrics.num_screenbind_failures++;
         g_ctrlm.pairing_metrics.last_screenbind_error_timestamp = time(NULL);
         g_ctrlm.pairing_metrics.last_screenbind_error_code = g_ctrlm.pairing_window.bind_status;
         if(g_ctrlm.pairing_window.bind_status != CTRLM_BIND_STATUS_NO_DISCOVERY_REQUEST) {
            safec_rc = strcpy_s(g_ctrlm.pairing_metrics.last_screenbind_remote_type, sizeof(g_ctrlm.pairing_metrics.last_screenbind_remote_type), g_ctrlm.discovery_remote_type);
            ERR_CHK(safec_rc);
         } else {
            g_ctrlm.pairing_metrics.last_screenbind_remote_type[0] = '\0';
         }
         ctrlm_property_write_pairing_metrics();
      }
   }
   if (g_ctrlm.one_touch_autobind_active) {
      ctrlm_stop_one_touch_autobind_(network_id);
      //Save the non screen bind pairing metrics
      if(g_ctrlm.pairing_window.bind_status != CTRLM_BIND_STATUS_SUCCESS) {
         g_ctrlm.pairing_metrics.num_non_screenbind_failures++;
         g_ctrlm.pairing_metrics.last_non_screenbind_error_timestamp = time(NULL);
         g_ctrlm.pairing_metrics.last_non_screenbind_error_code = g_ctrlm.pairing_window.bind_status;
         g_ctrlm.pairing_metrics.last_non_screenbind_error_binding_type = CTRLM_RCU_BINDING_TYPE_AUTOMATIC;
         if(g_ctrlm.pairing_window.bind_status != CTRLM_BIND_STATUS_NO_DISCOVERY_REQUEST) {
            safec_rc = strcpy_s(g_ctrlm.pairing_metrics.last_non_screenbind_remote_type, sizeof(g_ctrlm.pairing_metrics.last_non_screenbind_remote_type), g_ctrlm.discovery_remote_type);
            ERR_CHK(safec_rc);
         } else {
            g_ctrlm.pairing_metrics.last_non_screenbind_remote_type[0] = '\0';
         }
         ctrlm_property_write_pairing_metrics();
      }
   }
}

void ctrlm_pairing_window_bind_status_set_(ctrlm_bind_status_t bind_status) {
   // Don't overwrite the ASB failure with HAL failure
   if((bind_status == CTRLM_BIND_STATUS_HAL_FAILURE) && (g_ctrlm.pairing_window.bind_status == CTRLM_BIND_STATUS_ASB_FAILURE)) {
      XLOGD_INFO("Ignore Binding Status <%s>.  This is really an <%s>.", ctrlm_bind_status_str(bind_status), ctrlm_bind_status_str(g_ctrlm.pairing_window.bind_status));
      return;
   }

   switch(bind_status) {
      case CTRLM_BIND_STATUS_NO_DISCOVERY_REQUEST:
      case CTRLM_BIND_STATUS_NO_PAIRING_REQUEST:
      case CTRLM_BIND_STATUS_UNKNOWN_FAILURE:
         //These are likely intermediate or temporary status's.  Only log with DEBUG on.
         XLOGD_DEBUG("Binding Status <%s>.", ctrlm_bind_status_str(bind_status));
         break;
      case CTRLM_BIND_STATUS_SUCCESS:
      case CTRLM_BIND_STATUS_HAL_FAILURE:
      case CTRLM_BIND_STATUS_CTRLM_BLACKOUT:
      case CTRLM_BIND_STATUS_ASB_FAILURE:
      case CTRLM_BIND_STATUS_STD_KEY_EXCHANGE_FAILURE:
      case CTRLM_BIND_STATUS_PING_FAILURE:
      case CTRLM_BIND_STATUS_VALILDATION_FAILURE:
      case CTRLM_BIND_STATUS_RIB_UPDATE_FAILURE:
      case CTRLM_BIND_STATUS_BIND_WINDOW_TIMEOUT:
         XLOGD_INFO("Binding Status <%s>.", ctrlm_bind_status_str(bind_status));
         break;
      default:
         break;
   }
   g_ctrlm.pairing_window.bind_status = bind_status;
}

void ctrlm_discovery_remote_type_set_(const char *remote_type_str) {
   // Don't overwrite the ASB failure with HAL failure
   if(remote_type_str == NULL) {
      XLOGD_ERROR("remote_type_str is NULL.");
      return;
   }
   errno_t safec_rc = strcpy_s(g_ctrlm.discovery_remote_type, sizeof(g_ctrlm.discovery_remote_type), remote_type_str);
   ERR_CHK(safec_rc);
   g_ctrlm.discovery_remote_type[CTRLM_HAL_RF4CE_USER_STRING_SIZE-1] = '\0';
   XLOGD_INFO("discovery_remote_type <%s>.", g_ctrlm.discovery_remote_type);
}

void ctrlm_property_write_shutdown_time(void) {
   guchar data[CTRLM_RF4CE_LEN_SHUTDOWN_TIME];
   data[0]  = (guchar)(g_ctrlm.shutdown_time);
   data[1]  = (guchar)(g_ctrlm.shutdown_time >> 8);
   data[2]  = (guchar)(g_ctrlm.shutdown_time >> 16);
   data[3]  = (guchar)(g_ctrlm.shutdown_time >> 24);

   ctrlm_db_shutdown_time_write(data, CTRLM_RF4CE_LEN_SHUTDOWN_TIME);
}

guchar ctrlm_property_write_shutdown_time(guchar *data, guchar length) {
   if(data == NULL || length != CTRLM_RF4CE_LEN_SHUTDOWN_TIME) {
      XLOGD_ERROR("INVALID PARAMETERS");
      return(0);
   }
   time_t shutdown_time = ((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]);
   
   if(g_ctrlm.shutdown_time != shutdown_time) {
      // Store the data
      g_ctrlm.shutdown_time = shutdown_time;

      if(!g_ctrlm.loading_db) {
         ctrlm_property_write_shutdown_time();
      }
   }

   return(length);
}

void ctrlm_property_read_shutdown_time(void) {
   guchar *data = NULL;
   guint32 length;
   ctrlm_db_shutdown_time_read(&data, &length);

   if(data == NULL) {
      XLOGD_WARN("Not read from DB - IR shutdown time");
   } else {
      ctrlm_property_write_shutdown_time(data, length);
      ctrlm_db_free(data);
      data = NULL;
   }
}

time_t ctrlm_shutdown_time_get(void) {
   return(g_ctrlm.shutdown_time);
}


#if CTRLM_HAL_RF4CE_API_VERSION >= 9
void ctrlm_crash_recovery_check() {
   uint32_t crash_count      = 0;
   uint32_t invalid_hal_nvm  = 0;
   uint32_t invalid_ctrlm_db = 0;

   XLOGD_INFO("Checking if recovering from pre-init crash");

#ifdef CTRLM_NETWORK_HAS_HAL_NVM
   ctrlm_recovery_property_get(CTRLM_RECOVERY_INVALID_HAL_NVM, &invalid_hal_nvm);
#endif

   ctrlm_recovery_property_get(CTRLM_RECOVERY_INVALID_CTRLM_DB, &invalid_ctrlm_db);
   // Clear invalid DB flag
   if(invalid_ctrlm_db) {
      invalid_ctrlm_db = 0;
      ctrlm_recovery_property_set(CTRLM_RECOVERY_INVALID_CTRLM_DB, &invalid_ctrlm_db);
      invalid_ctrlm_db = 1;
   }

   // Get crash count
   ctrlm_recovery_property_get(CTRLM_RECOVERY_CRASH_COUNT, &crash_count);

   // Start logic for recovery
   if(crash_count >= 2 * g_ctrlm.crash_recovery_threshold && (invalid_hal_nvm || invalid_ctrlm_db)) { // Using the backup did not work.. Worst case scenario
      XLOGD_FATAL("failed to recover with backup NVM, %s", (invalid_hal_nvm ? "failure due to hal NVM.. Worst case scenario removing HAL NVM and ctrlm DB" : "failure due to ctrlm DB.. Removing ctrlm DB"));
      // Remove both NVM and ctrlm DB
      if(!ctrlm_file_delete(g_ctrlm.db_path.c_str(), true)) {
         XLOGD_WARN("Failed to remove ctrlm DB.. It is possible it no longer exists");
      }
      // Set recovery mode in rf4ce object
      if(invalid_hal_nvm) {
         for(auto const &itr : g_ctrlm.networks) {
            if(itr.second->type_get() == CTRLM_NETWORK_TYPE_RF4CE) {
               itr.second->recovery_set(CTRLM_RECOVERY_TYPE_RESET);
            }
         }
#ifdef CTRLM_NETWORK_HAS_HAL_NVM
         //Clear invalid NVM flag 
         invalid_hal_nvm = 0;
         ctrlm_recovery_property_set(CTRLM_RECOVERY_INVALID_HAL_NVM, &invalid_hal_nvm);
#endif
      }
      // Set crash back to 0
      crash_count = 0;
      ctrlm_recovery_property_set(CTRLM_RECOVERY_CRASH_COUNT, &crash_count);
   }
   else if(crash_count >= 2 * g_ctrlm.crash_recovery_threshold) {
      XLOGD_TELEMETRY("Entered a reboot loop in which both NVM and ctrlm DB are valid");
   }
   else if(crash_count >= g_ctrlm.crash_recovery_threshold) { // We entered a rebooting loop and met the threshold
      XLOGD_ERROR("Failed to initialize %u times, trying to restore backup of HAL NVM and ctrlm DB", crash_count);

      // Check Timestamps to ensure consistency across backup files
      guint64    hal_nvm_ts  = 0;
      guint64    ctrlm_db_ts = 0;

#ifdef CTRLM_NETWORK_HAS_HAL_NVM
      if(FALSE == ctrlm_file_timestamp_get(HAL_NVM_BACKUP, &hal_nvm_ts)) {
         XLOGD_ERROR("Failed to read timestamp of HAL NVM backup");
         // Increment crash count
         crash_count = crash_count + 1;
         ctrlm_recovery_property_set(CTRLM_RECOVERY_CRASH_COUNT, &crash_count);
         return;
      }
#endif
      if(FALSE == ctrlm_file_timestamp_get(CTRLM_NVM_BACKUP, &ctrlm_db_ts)) {
         XLOGD_ERROR("Failed to read timestamp of ctrlm db backup");
         // Increment crash count
         crash_count = crash_count + 1;
         ctrlm_recovery_property_set(CTRLM_RECOVERY_CRASH_COUNT, &crash_count);
         return;
      }

      // Compare the timestamps
      if(hal_nvm_ts != ctrlm_db_ts) {
         XLOGD_ERROR("The timestamps of the hal nvm and ctrlm db files do not match.. Cannot restore inconsistent backups");
         crash_count = 2 * g_ctrlm.crash_recovery_threshold;
         ctrlm_recovery_property_set(CTRLM_RECOVERY_CRASH_COUNT, &crash_count);
         return;
      }

      // Set Recovery mode in rf4ce object
      for(auto const &itr : g_ctrlm.networks) {
         if(itr.second->type_get() == CTRLM_NETWORK_TYPE_RF4CE) {
            itr.second->recovery_set(CTRLM_RECOVERY_TYPE_BACKUP);
         }
      }
      // Restore backup for ctrlm NVM
      if(FALSE == ctrlm_file_copy(CTRLM_NVM_BACKUP, g_ctrlm.db_path.c_str(), TRUE, TRUE)) {
         XLOGD_WARN("Failed to restore ctrlm DB backup, removing current DB...");
         ctrlm_file_delete(g_ctrlm.db_path.c_str(), true);
      }
      // Increment crash count
      crash_count = crash_count + 1;
      ctrlm_recovery_property_set(CTRLM_RECOVERY_CRASH_COUNT, &crash_count);
   }
   else {
      XLOGD_WARN("Failed to initialize %u times", crash_count);
      // Increment crash count
      crash_count = crash_count + 1;
      ctrlm_recovery_property_set(CTRLM_RECOVERY_CRASH_COUNT, &crash_count);
   }

}

void ctrlm_backup_data() {

#if ( __GLIBC__ == 2 ) && ( __GLIBC_MINOR__ >= 20 )
   gint64 t;
#else
   GTimeVal t;
#endif
   // Back up network NVM files
   for(auto const &itr : g_ctrlm.networks) {
      if(itr.second->type_get() == CTRLM_NETWORK_TYPE_RF4CE) {
         if(FALSE == itr.second->backup_hal_nvm()) {
            XLOGD_ERROR("Failed to back up RF4CE HAL NVM");
            return;
         }
      }
   }

   // Back up ctrlm db
   if(FALSE == ctrlm_db_backup()) {
      ctrlm_file_delete(CTRLM_NVM_BACKUP, true);
#ifdef CTRLM_NETWORK_HAS_HAL_NVM
      remove(HAL_NVM_BACKUP);
#endif
      return;
   }
glong tv_sec = 0;
   // Get timestamps so we know backup data is consistent
#if ( __GLIBC__ == 2 ) && ( __GLIBC_MINOR__ >= 20 )
   t = g_get_real_time ();
   tv_sec = t/G_USEC_PER_SEC;
#else
   g_get_current_time(&t);
   tv_sec = t.tv_sec;
#endif
#ifdef CTRLM_NETWORK_HAS_HAL_NVM
   if(FALSE == ctrlm_file_timestamp_set(HAL_NVM_BACKUP, tv_sec)) {
      ctrlm_file_delete(CTRLM_NVM_BACKUP, true);
      remove(HAL_NVM_BACKUP);
      return;
   }
#endif

   if(FALSE == ctrlm_file_timestamp_set(CTRLM_NVM_BACKUP, tv_sec)) {
      ctrlm_file_delete(CTRLM_NVM_BACKUP, true);
#ifdef CTRLM_NETWORK_HAS_HAL_NVM
      remove(HAL_NVM_BACKUP);
#endif
      return;
   }
}
#endif

gboolean ctrlm_message_queue_delay(gpointer data) {
   if(data) {
      ctrlm_main_queue_msg_push(data);
   }
   return(FALSE);
}

gboolean ctrlm_ntp_check(gpointer user_data) {
   ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)malloc(sizeof(ctrlm_main_queue_msg_header_t));
   if(msg) {
      msg->type = CTRLM_MAIN_QUEUE_MSG_TYPE_NTP_CHECK;
      msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;
      ctrlm_main_queue_msg_push(msg);
   } else {
      XLOGD_ERROR("failed to allocate time check");
   }
   return(false);
}

gboolean ctrlm_power_state_change(ctrlm_power_state_t power_state) {
   //Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_power_state_change_t *msg = (ctrlm_main_queue_power_state_change_t *)g_malloc(sizeof(ctrlm_main_queue_power_state_change_t));
   if(NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return(false);
   }

   msg->header.type = CTRLM_MAIN_QUEUE_MSG_TYPE_POWER_STATE_CHANGE;
   msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
   msg->new_state = power_state;
   ctrlm_main_queue_msg_push(msg);
   return(true);
}

ctrlm_controller_id_t ctrlm_last_used_controller_get(ctrlm_network_type_t network_type) {
   if (network_type == CTRLM_NETWORK_TYPE_RF4CE) {
      return g_ctrlm.last_key_info.controller_id;
   }
   XLOGD_ERROR("Not supported for %s", ctrlm_network_type_str(network_type).c_str());
   return CTRLM_MAIN_CONTROLLER_ID_INVALID;
}

void ctrlm_controller_product_name_get(ctrlm_controller_id_t controller_id, char *source_name) {
   // Signal completion of the operation
   sem_t semaphore;
   ctrlm_main_status_cmd_result_t cmd_result = CTRLM_MAIN_STATUS_REQUEST_PENDING;

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_product_name_t msg;
   errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
   ERR_CHK(safec_rc);

   sem_init(&semaphore, 0, 0);
   msg.controller_id   = controller_id;
   msg.product_name    = source_name;
   msg.semaphore       = &semaphore;
   msg.cmd_result      = &cmd_result;

   ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_controller_product_name, &msg, sizeof(msg), NULL, 1); // TODO: Hack, using default RF4CE network ID.

   // Wait for the result semaphore to be signaled
   XLOGD_DEBUG("Waiting for main thread to process CONTROLLER_TYPE request");
   sem_wait(&semaphore);
   sem_destroy(&semaphore);

   if(cmd_result != CTRLM_MAIN_STATUS_REQUEST_SUCCESS) {
      XLOGD_ERROR("ERROR getting product name!!");
      return;
   }
}

void control_service_values_read_from_db() {
   guchar *data = NULL;
   guint32 length;

#ifdef ASB
   //ASB enabled
   gboolean asb_enabled = CTRLM_ASB_ENABLED_DEFAULT;
   ctrlm_db_asb_enabled_read(&data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - ASB Enabled.  Using default of <%s>.", asb_enabled ? "true" : "false");
      // Write new asb_enabled flag to NVM
      ctrlm_db_asb_enabled_write((guchar *)&asb_enabled, CTRLM_ASB_ENABLED_LEN); 
   } else {
      asb_enabled = data[0];
      ctrlm_db_free(data);
      data = NULL;
      XLOGD_INFO("ASB Enabled read from DB <%s>", asb_enabled ? "YES" : "NO");
   }
   g_ctrlm.cs_values.asb_enable = asb_enabled;
#endif
   // Open Chime Enabled
   gboolean open_chime_enabled = CTRLM_OPEN_CHIME_ENABLED_DEFAULT;
   ctrlm_db_open_chime_enabled_read(&data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Open Chime Enabled.  Using default of <%s>.", open_chime_enabled ? "true" : "false");
      // Write new open_chime_enabled flag to NVM
      ctrlm_db_open_chime_enabled_write((guchar *)&open_chime_enabled, CTRLM_OPEN_CHIME_ENABLED_LEN); 
   } else {
      open_chime_enabled = data[0];
      ctrlm_db_free(data);
      data = NULL;
      XLOGD_INFO("Open Chime Enabled read from DB <%s>", open_chime_enabled ? "YES" : "NO");
   }
   g_ctrlm.cs_values.chime_open_enable = open_chime_enabled;

   // Close Chime Enabled
   gboolean close_chime_enabled = CTRLM_CLOSE_CHIME_ENABLED_DEFAULT;
   ctrlm_db_close_chime_enabled_read(&data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Close Chime Enabled.  Using default of <%s>.", close_chime_enabled ? "true" : "false");
      // Write new close_chime_enabled flag to NVM
      ctrlm_db_close_chime_enabled_write((guchar *)&close_chime_enabled, CTRLM_CLOSE_CHIME_ENABLED_LEN); 
   } else {
      close_chime_enabled = data[0];
      ctrlm_db_free(data);
      data = NULL;
      XLOGD_INFO("Close Chime Enabled read from DB <%s>", close_chime_enabled ? "YES" : "NO");
   }
   g_ctrlm.cs_values.chime_close_enable = close_chime_enabled;

   // Privacy Chime Enabled
   gboolean privacy_chime_enabled = CTRLM_PRIVACY_CHIME_ENABLED_DEFAULT;
   ctrlm_db_privacy_chime_enabled_read(&data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Privacy Chime Enabled.  Using default of <%s>.", privacy_chime_enabled ? "true" : "false");
      // Write new privacy_chime_enabled flag to NVM
      ctrlm_db_privacy_chime_enabled_write((guchar *)&privacy_chime_enabled, CTRLM_PRIVACY_CHIME_ENABLED_LEN); 
   } else {
      privacy_chime_enabled = data[0];
      ctrlm_db_free(data);
      data = NULL;
      XLOGD_INFO("Privacy Chime Enabled read from DB <%s>", privacy_chime_enabled ? "YES" : "NO");
   }
   g_ctrlm.cs_values.chime_privacy_enable = privacy_chime_enabled;

   // Conversational Mode
   unsigned char conversational_mode = CTRLM_CONVERSATIONAL_MODE_DEFAULT;
   ctrlm_db_conversational_mode_read(&data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Conversational Mode.  Using default of <%u>.", conversational_mode);
      // Write new conversational mode to NVM
      ctrlm_db_conversational_mode_write((guchar *)&conversational_mode, CTRLM_CONVERSATIONAL_MODE_LEN); 
   } else {
      conversational_mode = data[0];
      ctrlm_db_free(data);
      data = NULL;
      if(conversational_mode > CTRLM_MAX_CONVERSATIONAL_MODE) {
         conversational_mode = CTRLM_MAX_CONVERSATIONAL_MODE;
         XLOGD_WARN("Conversational Mode from DB was out of range, falling back to default <%u>", conversational_mode);
      } else {
         XLOGD_INFO("Conversational Mode read from DB <%u>", conversational_mode);
      }
   }
   g_ctrlm.cs_values.conversational_mode = conversational_mode;

   // Chime Volume
   ctrlm_chime_volume_t chime_volume = CTRLM_CHIME_VOLUME_DEFAULT;
   ctrlm_db_chime_volume_read(&data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - Chime Volume.  Using default of <%u>.", chime_volume);
      // Write new chime_volume to NVM
      ctrlm_db_chime_volume_write((guchar *)&chime_volume, CTRLM_CHIME_VOLUME_LEN); 
   } else {
      chime_volume = (ctrlm_chime_volume_t)data[0];
      ctrlm_db_free(data);
      data = NULL;
      if((chime_volume < CTRLM_CHIME_VOLUME_LOW) || (chime_volume > CTRLM_CHIME_VOLUME_HIGH)) {
         chime_volume = CTRLM_CHIME_VOLUME_DEFAULT;
         XLOGD_WARN("Chime Volume from DB was out of range, falling back to default <%u>", chime_volume);
      } else {
         XLOGD_INFO("Chime Volume read from DB <%u>", chime_volume);
      }
   }
   g_ctrlm.cs_values.chime_volume = chime_volume;

   // IR Command Repeats
   unsigned char ir_command_repeats = CTRLM_IR_COMMAND_REPEATS_DEFAULT;
   ctrlm_db_ir_command_repeats_read(&data, &length);
   if(data == NULL) {
      XLOGD_WARN("Not read from DB - IR Command Repeats.  Using default of <%u>.", ir_command_repeats);
      // Write new ir_command_repeats to NVM
      ctrlm_db_ir_command_repeats_write((guchar *)&ir_command_repeats, CTRLM_IR_COMMAND_REPEATS_LEN); 
   } else {
      ir_command_repeats = data[0];
      ctrlm_db_free(data);
      data = NULL;
      if((ir_command_repeats < CTRLM_MIN_IR_COMMAND_REPEATS) || (ir_command_repeats > CTRLM_MAX_IR_COMMAND_REPEATS)) {
         ir_command_repeats = CTRLM_IR_COMMAND_REPEATS_DEFAULT;
         XLOGD_WARN("IR Command Repeats from DB was out of range, falling back to default <%u>", ir_command_repeats);
      } else {
         XLOGD_INFO("IR Command Repeats read from DB <%u>", ir_command_repeats);
      }
   }
   g_ctrlm.cs_values.ir_repeats = ir_command_repeats;

   // Call the network cs_values funcitons
   for(auto const &itr : g_ctrlm.networks) {
      itr.second->cs_values_set(&g_ctrlm.cs_values, true);
   }
}

ctrlm_voice_t *ctrlm_get_voice_obj() {
   return(g_ctrlm.voice_session);
}

ctrlm_telemetry_t *ctrlm_get_telemetry_obj() {
   return(g_ctrlm.telemetry);
}

gboolean ctrlm_start_iarm(gpointer user_data) {
   // Register iarm calls and events
   XLOGD_INFO(": Register iarm calls and events");
   // IARM Events that we are listening to from other processes
   IARM_Bus_RegisterEventHandler(IARM_BUS_IRMGR_NAME, IARM_BUS_IRMGR_EVENT_IRKEY, ctrlm_event_handler_ir);
   IARM_Bus_RegisterEventHandler(IARM_BUS_IRMGR_NAME, IARM_BUS_IRMGR_EVENT_CONTROL, ctrlm_event_handler_ir);
   ctrlm_main_iarm_init();
#ifdef SYSTEMD_NOTIFY
   XLOGD_INFO("Notifying systemd of successful initialization");
   sd_notifyf(0, "READY=1\nSTATUS=ctrlm-main has successfully initialized\nMAINPID=%lu", (unsigned long)getpid());
#endif
   return false;
}

ctrlm_power_state_t ctrlm_main_get_power_state(void) {
   return(g_ctrlm.power_state);
}

void ctrlm_trigger_startup_actions(void) {
   ctrlm_main_queue_msg_header_t *msg = (ctrlm_main_queue_msg_header_t *)g_malloc(sizeof(ctrlm_main_queue_msg_header_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
   } else {
      msg->type       = CTRLM_MAIN_QUEUE_MSG_TYPE_STARTUP;
      msg->network_id = CTRLM_MAIN_NETWORK_ID_ALL;
      ctrlm_main_queue_msg_push((gpointer)msg);
   }
}
