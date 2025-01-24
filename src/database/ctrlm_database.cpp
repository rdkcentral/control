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
#include <string.h>
#include <glib.h>
#include <sqlite3.h>
#include <limits.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_rcu.h"
#include "ctrlm_database.h"
#include "ctrlm_recovery.h"

using namespace std;

#define CTRLM_DB_FILE_NAME                        "/opt/control.sql"
#define CTRLM_DB_TABLE_CTRLMGR                    "ctrlm_global"                // Global table for control manager,  only one
#define CTRLM_DB_CTRLMGR_KEY_VERSION              "version"
#define CTRLM_DB_DEVICE_UPDATE_KEY_SESSION_ID     "device_update_session_id"
#define CTRLM_DB_VOICE_KEY_SETTINGS               "voice_settings"
#define CTRLM_DB_TABLE_IR_CONTROLLER              "ir_controller"
#define CTRLM_DB_TABLE_NET_RF4CE_GLOBAL           "rf4ce_%02X_global"           // Global table for an RF4CE network, one per RF4CE network
#define CTRLM_DB_TABLE_NET_RF4CE_CONTROLLER_LIST  "rf4ce_%02X_controller_list"  // Controller list for an RF4CE network, one per RF4CE network
#define CTRLM_DB_TABLE_NET_RF4CE_CONTROLLER_ENTRY "rf4ce_%02X_controller_%02X"  // RF4CE Controller table, one per controller
#define CTRLM_DB_TABLE_NET_IP_GLOBAL              "ip_%02X_global"              // Global table for an IP network, one per IP network
#define CTRLM_DB_TABLE_NET_IP_CONTROLLER_LIST     "ip_%02X_controller_list"     // Controller list for an IP network, one per IP network
#define CTRLM_DB_TABLE_NET_IP_CONTROLLER_ENTRY    "ip_%02X_controller_%02X"     // IP Controller table, one per controller
#define CTRLM_DB_TABLE_NET_IP                     "ip_%02X"
#define CTRLM_DB_TABLE_NET_BLE_GLOBAL             "ble_%02X_global"              // Global table for an IP network, one per IP network
#define CTRLM_DB_TABLE_NET_BLE_CONTROLLER_LIST    "ble_%02X_controller_list"     // Controller list for an IP network, one per IP network
#define CTRLM_DB_TABLE_NET_BLE_CONTROLLER_ENTRY   "ble_%02X_controller_%02X"     // IP Controller table, one per controller
#define CTRLM_DB_TABLE_NET_BLE                    "ble_%02X"
#define CTRLM_DB_TABLE_TARGET_IRDB_STATUS         "target_irdb_status"
#define CTRLM_DB_IR_REMOTE_USAGE                  "ir_remote_usage"
#define CTRLM_DB_PAIRING_METRICS                  "pairing_metrics"
#define CTRLM_DB_LAST_KEY_INFO                    "last_key_info"
#define CTRLM_DB_SHUTDOWN_TIME                    "shutdown_time"
#ifdef ASB
#define CTRLM_DB_ASB_ENABLED                      "asb_enabled"
#endif
#define CTRLM_DB_OPEN_CHIME_ENABLED               "open_chime_enabled"
#define CTRLM_DB_CLOSE_CHIME_ENABLED              "close_chime_enabled"
#define CTRLM_DB_PRIVACY_CHIME_ENABLED            "privacy_chime_enabled"
#define CTRLM_DB_CONVERSATIONAL_MODE              "conversational_mode"
#define CTRLM_DB_CHIME_VOLUME                     "chime_volume"
#define CTRLM_DB_IR_COMMAND_REPEATS               "ir_command_repeats"
#define CTRLM_DB_DEVICE_UPDATE_SESSION_STATE      "du_session_state"
#define CTRLM_DB_TV_IR_CODE_ID                    "tv_ir_code_id"
#define CTRLM_DB_AVR_IR_CODE_ID                   "avr_ir_code_id"

#define CTRLM_DB_TABLE_VOICE                      "ctrlm_voice"

#define CONTROLLER_TABLE_NAME_MAX_LEN        (32)
#define CONTROLLER_KEY_NAME_MAX_LEN          (40)

#define CTRLM_DB_VERSION                           "1"
#define CTRLM_DB_DEVICE_UPDATE_SESSION_ID_DEFAULT  (0)

typedef enum {
   // Network based messages
   CTRLM_DB_QUEUE_MSG_TYPE_WRITE_UINT64       = 0,
   CTRLM_DB_QUEUE_MSG_TYPE_WRITE_STRING       = 1,
   CTRLM_DB_QUEUE_MSG_TYPE_WRITE_BLOB         = 2,
   CTRLM_DB_QUEUE_MSG_TYPE_TERMINATE          = 3,
   CTRLM_DB_QUEUE_MSG_TYPE_WRITE_FILE         = 4,
   CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_DESTROY = 5,
   CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_CREATE  = 6,
   CTRLM_DB_QUEUE_MSG_TYPE_BACKUP             = 7,
#ifdef DEEPSLEEP_CLOSE_DB
   CTRLM_DB_QUEUE_MSG_TYPE_POWER_STATE_CHANGE = 8,
#endif
   CTRLM_DB_QUEUE_MSG_TYPE_WRITE_ATTR         = 9,
   CTRLM_DB_QUEUE_MSG_TYPE_TICKLE             = CTRLM_MAIN_QUEUE_MSG_TYPE_TICKLE
} ctrlm_db_queue_msg_type_t;

typedef struct {
   ctrlm_db_queue_msg_type_t type;
} ctrlm_db_queue_msg_header_t;

typedef struct {
   ctrlm_db_queue_msg_header_t header;
   char                        table[CONTROLLER_TABLE_NAME_MAX_LEN];
   char                        key[CONTROLLER_KEY_NAME_MAX_LEN];
   sqlite_uint64               value;
} ctrlm_db_queue_msg_write_uint64_t;

typedef struct {
   ctrlm_db_queue_msg_header_t header;
   char                        table[CONTROLLER_TABLE_NAME_MAX_LEN];
   char                        key[CONTROLLER_KEY_NAME_MAX_LEN];
   guchar *                    value;
} ctrlm_db_queue_msg_write_string_t;

typedef struct {
   ctrlm_db_queue_msg_header_t header;
   char                        table[CONTROLLER_TABLE_NAME_MAX_LEN];
   char                        key[CONTROLLER_KEY_NAME_MAX_LEN];
   guchar *                    value;
   guint32                     length;
} ctrlm_db_queue_msg_write_blob_t;

typedef struct {
   ctrlm_db_queue_msg_header_t header;
   std::weak_ptr<ctrlm_db_attr_t> attr;
} ctrlm_db_queue_msg_write_attr_t;

typedef struct {
   ctrlm_db_queue_msg_header_t header;
   char *                      path;
   guchar *                    data;
   guint32                     length;
} ctrlm_db_queue_msg_write_file_t;

typedef struct {
   ctrlm_db_queue_msg_header_t header;
   ctrlm_network_id_t          network_id;
   ctrlm_controller_id_t       controller_id;
} ctrlm_db_queue_msg_controller_t;

typedef struct {
   ctrlm_db_queue_msg_header_t header;
   bool *                      ret;
} ctrlm_db_queue_msg_backup_t;

#ifdef DEEPSLEEP_CLOSE_DB
typedef struct {
   ctrlm_db_queue_msg_header_t header;
   gboolean                    waking_up;
} ctrlm_db_queue_msg_power_state_change_t;
#endif

typedef struct {
   sqlite3 *                  handle;
   gboolean                   created_default_db;

   GThread *                  main_thread;
   GAsyncQueue *              queue;
   sem_t                      semaphore;

#ifdef DEEPSLEEP_CLOSE_DB
   char                       path[PATH_MAX];
   sem_t                      ds_signal;
#endif

   vector<ctrlm_network_id_t> rf4ce_network_list;
   vector<ctrlm_network_id_t> ip_network_list;
   vector<ctrlm_network_id_t> ble_network_list;

   bool                       voice_is_valid;
} ctrlm_db_global_t;

typedef struct {
   guint32                    version;
   guint64                    device_update_session_id;
} ctrlm_db_global_data_t;

ctrlm_db_global_t      g_ctrlm_db;
ctrlm_db_global_data_t g_ctrlm_db_global;

static bool     ctrlm_db_open(const char *db_path);
static void     ctrlm_db_close();
static bool     ctrlm_db_is_initialized();
static void     ctrlm_db_default();
static void     ctrlm_db_default_networks();
static void     ctrlm_db_print();
static int      ctrlm_db_integrity_check_result(void *param, int argc, char **argv, char **column);
static bool     ctrlm_db_verify_integrity();
static bool     ctrlm_db_vacuum();
static void     ctrlm_db_cache();
static gpointer ctrlm_db_thread(gpointer param);
static void     ctrlm_db_queue_msg_destroy(gpointer msg);

static const char *ctrlm_db_errmsg(int rc);

static bool ctrlm_db_table_exists(const char *table);
static bool ctrlm_db_key_exists(const char *table, const char *key);
static int  ctrlm_db_delete(const char *table, const char *key, bool pattern=false);

static int  ctrlm_db_read_uint64(const char *table, const char *key, sqlite_uint64 *value);
static int  ctrlm_db_read_int(const char *table, const char *key, int *value);
//static int  ctrlm_db_read_str(const char *table, const char *key, guchar **value);
static int  ctrlm_db_read_blob(const char *table, const char *key, guchar **value, guint32 *length);

static void ctrlm_db_write_uint64( const char *table, const char *key, sqlite_uint64 value);
static void ctrlm_db_write_uint64_(const char *table, const char *key, sqlite_uint64 value);
//static void ctrlm_db_write_int (const char *table, const char *key, int value);
//static void ctrlm_db_write_int_(const char *table, const char *key, int value);
static void ctrlm_db_write_str (const char *table, const char *key, const guchar *value);
static void ctrlm_db_write_str_(const char *table, const char *key, const guchar *value);
static void ctrlm_db_write_blob (const char *table, const char *key, const guchar *value, guint32 length);
static void ctrlm_db_write_blob_(const char *table, const char *key, const guchar *value, guint32 length);
static void ctrlm_db_write_file_(const char *path, const guchar *data, guint32 length);

#ifdef DEEPSLEEP_CLOSE_DB
static void ctrlm_db_power_state_change_(gboolean waking_up);
#endif
static int  ctrlm_db_insert_or_update(const char *table, const char *key, const int *value_int, const sqlite3_int64 *value_int64, const guchar *value_str, guint32 blob_length);
#if 0
static int  ctrlm_db_insert(const char *table, const char *key, const int *value_int, const char *value_str, guint32 blob_length);
static int  ctrlm_db_update(const char *table, const char *key, const int *value_int, const char *value_str, guint32 blob_length);
#endif
static int  ctrlm_db_select(const char *table, const char *key, int *value_int, sqlite_uint64 *value_int64, guchar **value_str, guint32 *value_len);
static int  ctrlm_db_select_keys(const char *table, vector<char *> *keys_str, vector<ctrlm_controller_id_t> *keys_int);

static void ctrlm_db_controller_destroy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
static void ctrlm_db_controller_create(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);

// Read/write functions that will get exposed to outside world
static void ctrlm_db_read_globals(ctrlm_db_global_data_t *db);

static void ctrlm_db_rf4ce_global_table_name(ctrlm_network_id_t network_id, char *table);
static void ctrlm_db_rf4ce_controller_list_table_name(ctrlm_network_id_t network_id, char *table);
static void ctrlm_db_rf4ce_controller_entry_table_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, char *table);
static void ctrlm_db_rf4ce_network_create(ctrlm_network_id_t network_id);
static void ctrlm_db_ip_global_table_name(ctrlm_network_id_t network_id, char *table);
static void ctrlm_db_ip_controller_list_table_name(ctrlm_network_id_t network_id, char *table);
static void ctrlm_db_ip_controller_entry_table_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, char *table);
static void ctrlm_db_ip_network_create(ctrlm_network_id_t network_id);
static void ctrlm_db_ble_global_table_name(ctrlm_network_id_t network_id, char *table);
static void ctrlm_db_ble_controller_list_table_name(ctrlm_network_id_t network_id, char *table);
static void ctrlm_db_ble_controller_entry_table_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, char *table);
static void ctrlm_db_ble_network_create(ctrlm_network_id_t network_id);
static void ctrlm_db_delete_legacy_entries(void);

bool ctrlm_db_open(const char *db_path) {
   int rc;

   g_ctrlm_db.created_default_db = false;
   g_ctrlm_db.voice_is_valid     = true; // This gets set to false IF the voice table is created when creating default DB

   if (false == ctrlm_utils_move_file_to_secure_nvm(db_path)) {
      XLOGD_ERROR("Failed to move file <%s> to secure area!!!!!!!!!!!!!!!!!!!", db_path);
      return(false);
   }

   // check for presence of database file and if not present, create it
   rc = sqlite3_open(db_path, &g_ctrlm_db.handle);
   if(rc) {
      XLOGD_ERROR("Can't open database: rc <%d> <%s>", rc, ctrlm_db_errmsg(rc));
      sqlite3_close(g_ctrlm_db.handle);
      sqlite3_shutdown();
      return(false);
   }

   // Use extended result codes to get more information about error conditions
   if(SQLITE_OK != sqlite3_extended_result_codes(g_ctrlm_db.handle, 1)) {
      XLOGD_WARN("sqlite unable to use extended result codes");
   }

   return(true);
}

void ctrlm_db_close() {
   if(g_ctrlm_db.handle != NULL) {
      sqlite3_close(g_ctrlm_db.handle);
      g_ctrlm_db.handle = NULL;
   }
}

gboolean ctrlm_db_init(const char *db_path) {
   g_ctrlm_db.created_default_db = false;
#ifdef DEEPSLEEP_CLOSE_DB
   errno_t safec_rc = strcpy_s(g_ctrlm_db.path, sizeof(g_ctrlm_db.path), db_path);
   ERR_CHK(safec_rc);
   sem_init(&g_ctrlm_db.ds_signal, 0, 0);
#endif

   // check for presence of database file and if not present, create it
   if(false == ctrlm_db_open(db_path)) {
       return(FALSE);
   }
   ctrlm_db_cache();

   // Verify the integrity of the database
   if(!ctrlm_db_verify_integrity()) {
      XLOGD_TELEMETRY("Database is corrupt. Try to recover...");
      if (ctrlm_db_vacuum() && ctrlm_db_verify_integrity()) {
         XLOGD_ERROR("Database recovery succeeded....");
      } else {
         XLOGD_ERROR("Database recovery failed....");
         sqlite3_close(g_ctrlm_db.handle);
         sqlite3_shutdown();
      return(FALSE);
      }
   }

   ctrlm_db_delete_legacy_entries();
   ctrlm_db_cache();

   ctrlm_db_print();

   // Launch a thread to handle DB writes asynchronously
   // Create an asynchronous queue to receive incoming messages from the networks
   g_ctrlm_db.queue = g_async_queue_new_full(ctrlm_db_queue_msg_destroy);

   // Initialize semaphore
   sem_init(&g_ctrlm_db.semaphore, 0, 0);

   g_ctrlm_db.main_thread = g_thread_new("ctrlm_db", ctrlm_db_thread, NULL);

   // Block until initialization is complete or a timeout occurs
   XLOGD_INFO("Waiting for database thread initialization...");
   sem_wait(&g_ctrlm_db.semaphore);

   return(TRUE);
}

void ctrlm_db_terminate(void) {
   XLOGD_INFO("clean up");

   if(g_ctrlm_db.main_thread != NULL) {

      ctrlm_db_queue_msg_header_t *msg = (ctrlm_db_queue_msg_header_t *)g_malloc(sizeof(ctrlm_db_queue_msg_header_t));
      if(msg == NULL) {
         XLOGD_ERROR("Out of memory");
      } else {
         struct timespec end_time;
         clock_gettime(CLOCK_REALTIME, &end_time);
         end_time.tv_sec += 5;
         msg->type = CTRLM_DB_QUEUE_MSG_TYPE_TERMINATE;

         XLOGD_INFO("Waiting for database thread termination...");
         ctrlm_db_queue_msg_push((gpointer)msg);

         // Block until termination is acknowledged or a timeout occurs
         int acknowledged = sem_timedwait(&g_ctrlm_db.semaphore, &end_time);

         if(acknowledged==-1) { // no response received
            XLOGD_INFO("Do NOT wait for thread to exit");
         } else {
            // Wait for thread to exit
            XLOGD_INFO("Waiting for thread to exit");
            g_thread_join(g_ctrlm_db.main_thread);
            XLOGD_INFO("thread exited.");
         }
      }
   }
#ifdef DEEPSLEEP_CLOSE_DB
   sem_destroy(&g_ctrlm_db.ds_signal);
#endif
   sem_destroy(&g_ctrlm_db.semaphore);

   ctrlm_db_close();
   sqlite3_shutdown();
}

void ctrlm_db_queue_msg_push(gpointer msg) {
   g_async_queue_push(g_ctrlm_db.queue, msg);
}

void ctrlm_db_queue_msg_push_front(gpointer msg) {
   g_async_queue_push_front(g_ctrlm_db.queue, msg);
}

void ctrlm_db_queue_msg_destroy(gpointer msg) {
   if(msg) {
      XLOGD_DEBUG("Free %p", msg);
      g_free(msg);
   }
}

bool ctrlm_db_backup() {
   bool ret                         = FALSE;
   ctrlm_db_queue_msg_backup_t *msg = (ctrlm_db_queue_msg_backup_t *)g_malloc(sizeof(ctrlm_db_queue_msg_backup_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return ret;
   }
   msg->header.type = CTRLM_DB_QUEUE_MSG_TYPE_BACKUP;
   msg->ret         = &ret;
   ctrlm_db_queue_msg_push((gpointer)msg);
   sem_wait(&g_ctrlm_db.semaphore);
   return ret;
} 

void ctrlm_db_power_state_change(gboolean waking_up) {
#ifdef DEEPSLEEP_CLOSE_DB
   ctrlm_db_queue_msg_power_state_change_t *msg = (ctrlm_db_queue_msg_power_state_change_t *)g_malloc(sizeof(ctrlm_db_queue_msg_power_state_change_t));
   if(NULL == msg) {
      XLOGD_ERROR("Failed to allocate memory");
      g_assert(0);
      return;
   }

   msg->header.type = CTRLM_DB_QUEUE_MSG_TYPE_POWER_STATE_CHANGE;
   msg->waking_up   = waking_up;

   if(waking_up) {
      ctrlm_db_queue_msg_push_front(msg);
      sem_post(&g_ctrlm_db.ds_signal);
   } else {
      ctrlm_db_queue_msg_push(msg);
   }
#else
   XLOGD_INFO("No action for DB");
#endif
}

#ifdef DEEPSLEEP_CLOSE_DB
void ctrlm_db_power_state_change_(gboolean waking_up) {

   if(waking_up == true ) {
      XLOGD_INFO("Opening DB due to coming out of DEEP_SLEEP");
      ctrlm_db_open(g_ctrlm_db.path);
   } else {
      XLOGD_INFO("Closing DB due to DEEP_SLEEP");
      ctrlm_db_close();
   }
}
#endif

gpointer ctrlm_db_thread(gpointer param) {
   bool running = true;
#ifdef DEEPSLEEP_CLOSE_DB
   bool ds_sem_wait = false;
#endif
   XLOGD_INFO("Started");

   // Unblock the caller that launched this thread
   sem_post(&g_ctrlm_db.semaphore);

   XLOGD_INFO("Enter main loop");
   do {
      gpointer msg = g_async_queue_pop(g_ctrlm_db.queue);
      if(msg == NULL) {
         XLOGD_ERROR("NULL message received");
         continue;
      }

      ctrlm_db_queue_msg_header_t *header = (ctrlm_db_queue_msg_header_t *)msg;
      switch(header->type) {
         case CTRLM_DB_QUEUE_MSG_TYPE_TERMINATE: {
            XLOGD_INFO("TERMINATE");
            sem_post(&g_ctrlm_db.semaphore);
            running = false;
            break;
         }
         case CTRLM_DB_QUEUE_MSG_TYPE_TICKLE: {
            XLOGD_DEBUG("TICKLE");
            ctrlm_thread_monitor_msg_t *thread_monitor_msg = (ctrlm_thread_monitor_msg_t *) msg;
            *thread_monitor_msg->response = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
            break;
         }
         case CTRLM_DB_QUEUE_MSG_TYPE_WRITE_UINT64: {
            ctrlm_db_queue_msg_write_uint64_t *uint64 = (ctrlm_db_queue_msg_write_uint64_t *)msg;
            XLOGD_DEBUG("WRITE UINT64 %s:%s:0x%016llX", uint64->table, uint64->key, uint64->value);
            ctrlm_db_write_uint64_(uint64->table, uint64->key, uint64->value);
            break;
         }
         case CTRLM_DB_QUEUE_MSG_TYPE_WRITE_STRING: {
            ctrlm_db_queue_msg_write_string_t *string = (ctrlm_db_queue_msg_write_string_t *)msg;
            XLOGD_DEBUG("WRITE STRING %s:%s:%s", string->table, string->key, string->value);
            ctrlm_db_write_str_(string->table, string->key, string->value);
            break;
         }
         case CTRLM_DB_QUEUE_MSG_TYPE_WRITE_BLOB: {
            ctrlm_db_queue_msg_write_blob_t *blob = (ctrlm_db_queue_msg_write_blob_t *)msg;
            XLOGD_DEBUG("WRITE BLOB %s:%s:%u", blob->table, blob->key, blob->length);
            ctrlm_print_data_hex(__FUNCTION__, blob->value, blob->length, 16);
            ctrlm_db_write_blob_(blob->table, blob->key, blob->value, blob->length);
            break;
         }
        case CTRLM_DB_QUEUE_MSG_TYPE_WRITE_ATTR: {
            ctrlm_db_queue_msg_write_attr_t *attr = (ctrlm_db_queue_msg_write_attr_t *)msg;
            XLOGD_DEBUG("WRITE ATTR");
            if (auto shared_attr_ptr = attr->attr.lock()) {
               shared_attr_ptr->write_db(g_ctrlm_db.handle);
            }
            attr->attr.reset(); delete attr; msg = NULL;
            break;
         }
         case CTRLM_DB_QUEUE_MSG_TYPE_WRITE_FILE: {
            ctrlm_db_queue_msg_write_file_t *file_data = (ctrlm_db_queue_msg_write_file_t *)msg;
            XLOGD_DEBUG("WRITE FILE %s %u bytes", file_data->path, file_data->length);
            ctrlm_db_write_file_(file_data->path, file_data->data, file_data->length);
            ctrlm_hal_free(file_data->data);
            break;
         }
         case CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_DESTROY: {
            ctrlm_db_queue_msg_controller_t *controller = (ctrlm_db_queue_msg_controller_t *)msg;
            XLOGD_DEBUG("DESTROY CONTROLLER (%u, %u)", controller->network_id, controller->controller_id);
            ctrlm_db_controller_destroy(controller->network_id, controller->controller_id);
            break;
         }
         case CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_CREATE: {
            ctrlm_db_queue_msg_controller_t *controller = (ctrlm_db_queue_msg_controller_t *)msg;
            XLOGD_DEBUG("CREATE CONTROLLER (%u, %u)", controller->network_id, controller->controller_id);
            ctrlm_db_controller_create(controller->network_id, controller->controller_id);
            break;
         }
         case CTRLM_DB_QUEUE_MSG_TYPE_BACKUP: {
            ctrlm_db_queue_msg_backup_t *backup = (ctrlm_db_queue_msg_backup_t *)msg;
            XLOGD_DEBUG("BACKUP DATABASE");
            if (false == ctrlm_utils_move_file_to_secure_nvm(CTRLM_NVM_BACKUP)) {
               XLOGD_ERROR("Failed to move file <%s> to secure area!!!!!!!!!!!!!!!!!!!", CTRLM_NVM_BACKUP);
               *(backup->ret) = FALSE;
            } else {
               if(FALSE == ctrlm_file_copy(sqlite3_db_filename(g_ctrlm_db.handle, "main"), CTRLM_NVM_BACKUP, TRUE, TRUE)) {
                  XLOGD_ERROR("Failed to back up ctrlm DB");
                  *(backup->ret) = FALSE;
               } else {
                  XLOGD_INFO("ctrlm DB backed up successfully");
                  *(backup->ret) = TRUE;
               }
            }
            sem_post(&g_ctrlm_db.semaphore);
            break;
         }
#ifdef DEEPSLEEP_CLOSE_DB
         case CTRLM_DB_QUEUE_MSG_TYPE_POWER_STATE_CHANGE: {
            XLOGD_DEBUG("POWER STATE CHANGE");
            ctrlm_db_queue_msg_power_state_change_t *power_state_change = (ctrlm_db_queue_msg_power_state_change_t *)msg;
            ds_sem_wait = power_state_change->waking_up ? false : true;
            ctrlm_db_power_state_change_(power_state_change->waking_up);
            break;
         }
#endif
         default: {
            break;
         }
      }
      ctrlm_db_queue_msg_destroy(msg);

#ifdef DEEPSLEEP_CLOSE_DB
      if(ds_sem_wait == true) {
         sem_wait(&g_ctrlm_db.ds_signal);
      }
#endif
   } while(running);
   return(NULL);
}

bool ctrlm_db_table_exists(const char *table) {
   bool ret = false;
   if(table == NULL) {
      XLOGD_WARN("invalid parameters!");
   } else {
      sqlite3_stmt *p_stmt = NULL;
      int rc;
      stringstream sql;
      sql << "SELECT * FROM " << table << ";";

      rc = sqlite3_prepare(g_ctrlm_db.handle, sql.str().c_str(), -1, &p_stmt, 0);
      if(rc != SQLITE_OK){
         XLOGD_TELEMETRY("Unable to prepare sql statement <%s> rc <%d> <%s>!", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      } else {
         ret = true;
         rc = sqlite3_finalize(p_stmt);
         if(rc != SQLITE_OK){
            XLOGD_ERROR("Unable to finalize! <%s> rc <%d> <%s>", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
         }
      }
   }
   XLOGD_DEBUG("%s %s!", table, (ret ? "exists" : "doesn't exist"));
   return(ret);
}

// Global read / write functions
void ctrlm_db_version_write(int version) {
   ctrlm_db_write_uint64(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CTRLMGR_KEY_VERSION, version);
}

void ctrlm_db_version_read(int *version) {
   *version = g_ctrlm_db_global.version;
}

void ctrlm_db_device_update_session_id_write(unsigned char session_id) {
   g_ctrlm_db_global.device_update_session_id = session_id;
   ctrlm_db_write_uint64(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_DEVICE_UPDATE_KEY_SESSION_ID, session_id);
}

void ctrlm_db_device_update_session_id_read(unsigned char *session_id) {
   *session_id = g_ctrlm_db_global.device_update_session_id;
}

void ctrlm_db_voice_settings_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_VOICE_KEY_SETTINGS, data, length);
}

void ctrlm_db_voice_settings_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_VOICE_KEY_SETTINGS, data, length);
}

void ctrlm_db_ir_rf_database_write(ctrlm_key_code_t key_code, guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, ctrlm_key_code_str(key_code), data, length);
}

void ctrlm_db_ir_rf_database_read(ctrlm_key_code_t key_code, guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, ctrlm_key_code_str(key_code), data, length);
}

void ctrlm_db_ir_rf_database_delete(ctrlm_key_code_t key_code) {
   int rc = ctrlm_db_delete(CTRLM_DB_TABLE_CTRLMGR, ctrlm_key_code_str(key_code));
   if(rc != 0) {
      XLOGD_INFO("error deleting key %s", ctrlm_key_code_str(key_code));
   }
}

void ctrlm_db_target_irdb_status_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_TABLE_TARGET_IRDB_STATUS, data, length);
}

void ctrlm_db_target_irdb_status_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_TABLE_TARGET_IRDB_STATUS, data, length);
}

void ctrlm_db_ir_remote_usage_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_IR_REMOTE_USAGE, data, length);
}

void ctrlm_db_ir_remote_usage_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_IR_REMOTE_USAGE, data, length);
}

void ctrlm_db_pairing_metrics_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_PAIRING_METRICS, data, length);
}

void ctrlm_db_pairing_metrics_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_PAIRING_METRICS, data, length);
}

void ctrlm_db_last_key_info_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_LAST_KEY_INFO, data, length);
}

void ctrlm_db_last_key_info_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_LAST_KEY_INFO, data, length);
}

void ctrlm_db_shutdown_time_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_SHUTDOWN_TIME, data, length);
}

void ctrlm_db_shutdown_time_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_SHUTDOWN_TIME, data, length);
}

#ifdef ASB
void ctrlm_db_asb_enabled_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_ASB_ENABLED, data, length);
}

void ctrlm_db_asb_enabled_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_ASB_ENABLED, data, length);
}
#endif

void ctrlm_db_open_chime_enabled_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_OPEN_CHIME_ENABLED, data, length);
}

void ctrlm_db_open_chime_enabled_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_OPEN_CHIME_ENABLED, data, length);
}

void ctrlm_db_close_chime_enabled_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CLOSE_CHIME_ENABLED, data, length);
}

void ctrlm_db_close_chime_enabled_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CLOSE_CHIME_ENABLED, data, length);
}

void ctrlm_db_privacy_chime_enabled_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_PRIVACY_CHIME_ENABLED, data, length);
}

void ctrlm_db_privacy_chime_enabled_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_PRIVACY_CHIME_ENABLED, data, length);
}

void ctrlm_db_conversational_mode_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CONVERSATIONAL_MODE, data, length);
}

void ctrlm_db_conversational_mode_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CONVERSATIONAL_MODE, data, length);
}

void ctrlm_db_chime_volume_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CHIME_VOLUME, data, length);
}

void ctrlm_db_chime_volume_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CHIME_VOLUME, data, length);
}

void ctrlm_db_ir_command_repeats_write(guchar *data, guint32 length) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_IR_COMMAND_REPEATS, data, length);
}

void ctrlm_db_ir_command_repeats_read(guchar **data, guint32 *length) {
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_IR_COMMAND_REPEATS, data, length);
}

void ctrlm_db_tv_ir_code_id_write(const std::string id) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_TV_IR_CODE_ID, (const guchar*) id.c_str(), id.length());
}

void ctrlm_db_tv_ir_code_id_read(std::string &id) {
   guchar *data = NULL;
   guint32 length = 0;
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_TV_IR_CODE_ID, &data, &length);
   if(NULL != data) {
      id.assign((char *)data, length);
      ctrlm_db_free(data);
   } else {
      XLOGD_WARN("Failed to load tv_ir_code_id from db");
   }
}

void ctrlm_db_avr_ir_code_id_write(const std::string id) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_AVR_IR_CODE_ID, (const guchar*) id.c_str(), id.length());
}

void ctrlm_db_avr_ir_code_id_read(std::string &id) {
   guchar *data = NULL;
   guint32 length = 0;
   ctrlm_db_read_blob(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_AVR_IR_CODE_ID, &data, &length);
   if(NULL != data) {
      id.assign((char *)data, length);
      ctrlm_db_free(data);
   } else {
      XLOGD_WARN("Failed to load avr_ir_code_id from db");
   }
}

const char *ctrlm_db_errmsg(int rc) {
   if(rc == SQLITE_ROW || rc == SQLITE_DONE) { // these are non-error result codes
      return("");
   }
   return(sqlite3_errmsg(g_ctrlm_db.handle));
}

bool ctrlm_db_key_exists(const char *table, const char *key) {
   if(table == NULL || key == NULL) {
      XLOGD_WARN("invalid parameters!");
      return(-1);
   }

   sqlite3_stmt *p_stmt = NULL;
   int rc;
   errno_t safec_rc = -1;
   int ind = -1;
   stringstream sql;
   sql << "SELECT key FROM " << table << ";";

   rc = sqlite3_prepare(g_ctrlm_db.handle, sql.str().c_str(), -1, &p_stmt, 0);
   if(rc != SQLITE_OK){
      XLOGD_TELEMETRY("Unable to prepare sql statement <%s> rc <%d> <%s>!", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      return(false);
   }

   bool found = false;
   do {
      rc = sqlite3_step(p_stmt);

      if(rc == SQLITE_ROW){
         const unsigned char *col_name = sqlite3_column_text(p_stmt, 0);

         if(col_name == NULL) {
            XLOGD_ERROR("NULL column name");
            continue;
         }
         XLOGD_DEBUG("key <%s>", col_name);

         safec_rc = strcmp_s(key, strlen(key), (const char *)col_name, &ind);
         ERR_CHK(safec_rc);
         if((safec_rc == EOK) && (!ind)) {
            found = true;
            break;
         }
      } else if(rc != SQLITE_DONE) {
         XLOGD_ERROR("Unable to step! <%s> rc <%d> <%s>", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      }
   } while(rc == SQLITE_ROW);

   rc = sqlite3_finalize(p_stmt);
   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to finalize! <%s> rc <%d> <%s>", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      return(false);
   }
   return(found);
}

// Read a 64-bit integer from the specifed table and key
int  ctrlm_db_read_uint64(const char *table, const char *key, sqlite_uint64 *value) {
   return(ctrlm_db_select(table, key, NULL, value, NULL, NULL));
}

// Read an integer from the specifed table and key
int ctrlm_db_read_int(const char *table, const char *key, int *value) {
   return(ctrlm_db_select(table, key, value, NULL, NULL, NULL));
}

// Read a string from the specifed table and key (must free string after use)
//int ctrlm_db_read_str(const char *table, const char *key, guchar **value) {
//   return(ctrlm_db_select(table, key, NULL, NULL, value, NULL));
//}

// Read a blob from the specifed table and key (must free blob after use)
int ctrlm_db_read_blob(const char *table, const char *key, guchar **value, guint32 *length) {
   return(ctrlm_db_select(table, key, NULL, NULL, value, length));
}

void ctrlm_db_free(guchar *value) {
   if(value != NULL) {
      g_free(value);
   }
}

void ctrlm_db_write_uint64(const char *table, const char *key, sqlite_uint64 value) {
   errno_t safec_rc = -1;
   ctrlm_db_queue_msg_write_uint64_t *msg = (ctrlm_db_queue_msg_write_uint64_t *)g_malloc(sizeof(ctrlm_db_queue_msg_write_uint64_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   msg->header.type = CTRLM_DB_QUEUE_MSG_TYPE_WRITE_UINT64;
   safec_rc = strncpy_s(msg->table, sizeof(msg->table), table, CONTROLLER_TABLE_NAME_MAX_LEN - 1);
   ERR_CHK(safec_rc);
   msg->table[CONTROLLER_TABLE_NAME_MAX_LEN - 1] = '\0';
   safec_rc = strncpy_s(msg->key, sizeof(msg->key),  key,   CONTROLLER_KEY_NAME_MAX_LEN - 1);
   ERR_CHK(safec_rc);
   msg->key[CONTROLLER_KEY_NAME_MAX_LEN - 1] = '\0';
   msg->value = value;

   ctrlm_db_queue_msg_push((gpointer)msg);
}

// Write an unsigned 64-bit integer to the specifed table and key
void ctrlm_db_write_uint64_(const char *table, const char *key, sqlite_uint64 value) {
   //XLOGD_INFO("Table <%s> Key <%s> Value <0x%016llX>", table, key, value);
   ctrlm_db_insert_or_update(table, key, NULL, (sqlite_int64 *)&value, NULL, 0);
}

// Write an integer to the specifed table and key
//void ctrlm_db_write_int(const char *table, const char *key, int value) {
//   XLOGD_INFO("Table <%s> Key <%s> Value <%d>", table, key, value);
//   ctrlm_db_insert_or_update(table, key, &value, NULL, NULL, 0);
//}

void ctrlm_db_write_str(const char *table, const char *key, const guchar *value) {
   errno_t safec_rc = -1;
   size_t length = strlen((const char *)value);
   size_t msg_len = sizeof(ctrlm_db_queue_msg_write_string_t) + length + 1;
   ctrlm_db_queue_msg_write_string_t *msg = (ctrlm_db_queue_msg_write_string_t *)g_malloc(msg_len);
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   safec_rc = memset_s(msg, msg_len, 0, msg_len);
   ERR_CHK(safec_rc);
   msg->header.type = CTRLM_DB_QUEUE_MSG_TYPE_WRITE_STRING;
   safec_rc = strcpy_s(msg->table, sizeof(msg->table), table);
   ERR_CHK(safec_rc);
   safec_rc = strcpy_s(msg->key,  sizeof(msg->key), key);
   ERR_CHK(safec_rc);
   msg->value = (guchar *) &msg[1];
   safec_rc = strcpy_s((char *)(msg->value), length + 1, (const char *)value);
   ERR_CHK(safec_rc);

   ctrlm_db_queue_msg_push((gpointer)msg);
}

// Write a string to the specifed table and key
void ctrlm_db_write_str_(const char *table, const char *key, const guchar *value) {
   //XLOGD_INFO("Table <%s> Key <%s> Value <%s>", table, key, value);
   ctrlm_db_insert_or_update(table, key, NULL, NULL, value, 0);
}

void ctrlm_db_write_blob(const char *table, const char *key, const guchar *value, guint32 length) {
   errno_t safec_rc = -1;
   size_t msg_len = sizeof(ctrlm_db_queue_msg_write_blob_t) + length;
   ctrlm_db_queue_msg_write_blob_t *msg = (ctrlm_db_queue_msg_write_blob_t *)g_malloc(msg_len);
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   safec_rc = memset_s(msg, msg_len, 0, msg_len);
   ERR_CHK(safec_rc);
   msg->header.type = CTRLM_DB_QUEUE_MSG_TYPE_WRITE_BLOB;
   safec_rc = strncpy_s(msg->table, sizeof(msg->table),table, CONTROLLER_TABLE_NAME_MAX_LEN - 1);
   ERR_CHK(safec_rc);
   msg->table[CONTROLLER_TABLE_NAME_MAX_LEN - 1] = '\0';
   safec_rc = strncpy_s(msg->key, sizeof(msg->key),  key,   CONTROLLER_KEY_NAME_MAX_LEN - 1);
   ERR_CHK(safec_rc);
   msg->key[CONTROLLER_KEY_NAME_MAX_LEN - 1] = '\0';
   msg->value  = (guchar *)&msg[1];
   msg->length = length;
   safec_rc = memcpy_s(msg->value, msg->length, value, length);
   ERR_CHK(safec_rc);

   ctrlm_db_queue_msg_push((gpointer)msg);
}
// Write a blob to the specifed table and key
void ctrlm_db_write_blob_(const char *table, const char *key, const guchar *value, guint32 length) {
   //XLOGD_INFO("Table <%s> Key <%s> Blob Length <%u>", table, key, length);
   ctrlm_db_insert_or_update(table, key, NULL, NULL, value, length);
}

// Insert or update a key/value pair in the database
int ctrlm_db_insert_or_update(const char *table, const char *key, const int *value_int, const sqlite3_int64 *value_int64, const guchar *value_str, guint32 blob_length) {
   sqlite3_stmt *p_stmt = NULL;
   int rc;
   stringstream sql;

   if(table != NULL && key != NULL) {
      sql << "INSERT OR REPLACE INTO " << table << "(key,value) VALUES (?,?);";   //CID:80608 - Reverse_inull
   }
   else {
      XLOGD_ERROR("Invalid table or key");
      return(-1);
   }
   if(value_int == NULL && value_int64 == NULL && value_str == NULL) {
      XLOGD_ERROR("Invalid values");
      return(-1);
   }

   rc = sqlite3_prepare(g_ctrlm_db.handle, sql.str().c_str(), -1, &p_stmt, 0);
   if(rc != SQLITE_OK){
      XLOGD_TELEMETRY("Unable to prepare sql statement <%s> rc <%d> <%s>!", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      return(-1);
   }

   rc = sqlite3_bind_text(p_stmt, 1, key, -1, SQLITE_STATIC); // key name is the first variable
   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to bind first parameter! rc <%d> <%s>", rc, ctrlm_db_errmsg(rc));
      sqlite3_finalize(p_stmt);
      return(-1);
   }

   if(value_int != NULL) { // integer data
      rc = sqlite3_bind_int(p_stmt, 2, *value_int);
   } else if(value_int64 != NULL) {
      rc = sqlite3_bind_int64(p_stmt, 2, *value_int64);
   } else if(blob_length != 0) { // blob of data
      rc = sqlite3_bind_blob(p_stmt, 2, value_str, blob_length, SQLITE_STATIC);
   } else { // string data
      rc = sqlite3_bind_text(p_stmt, 2, (const char*)value_str, -1, SQLITE_STATIC);
   }

   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to bind second parameter! rc <%d> <%s>", rc, ctrlm_db_errmsg(rc));
      sqlite3_finalize(p_stmt);
      return(-1);
   }

   rc = sqlite3_step(p_stmt);
   if(rc == SQLITE_ROW){
      XLOGD_ERROR("Unable to step! <%s> rc <%d> <%s>", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      sqlite3_finalize(p_stmt);
      return(-1);
   }

   rc = sqlite3_finalize(p_stmt);
   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to finalize! <%s> rc <%d> <%s>", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      return(-1);
   }
   return(0);
}

#if 0
// Insert a new key/value pair in the database
int ctrlm_db_insert(const char *table, const char *key, const int *value_int, const char *value_str, guint32 blob_length) {
   sqlite3_stmt *p_stmt = NULL;
   int rc;
   stringstream sql;
   sql << "INSERT INTO " << table << "(key,value) VALUES (?,?);";

   if(table == NULL || key == NULL) {
      XLOGD_ERROR("Invalid table or key");
      return(-1);
   }
   if(value_int == NULL && value_str == NULL) {
      XLOGD_ERROR("Invalid values");
      return(-1);
   } else if(value_int != NULL && value_str != NULL) {
      XLOGD_ERROR("Multiple values specified");
      return(-1);
   }

   rc = sqlite3_prepare(g_ctrlm_db.handle, sql.str().c_str(), -1, &p_stmt, 0);
   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to prepare sql statement <%s> rc <%d> <%s>!", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      return(-1);
   }

   rc = sqlite3_bind_text(p_stmt, 1, key, -1, SQLITE_STATIC); // key name is the first variable
   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to bind first parameter! rc <%d> <%s>", rc, ctrlm_db_errmsg(rc));
      sqlite3_finalize(p_stmt);
      return(-1);
   }

   if(value_int != NULL) { // integer data
      rc = sqlite3_bind_int(p_stmt, 2, *value_int);
   } else if(blob_length != 0) { // blob of data
      rc = sqlite3_bind_blob(p_stmt, 2, value_str, blob_length, SQLITE_STATIC);
   } else { // string data
      rc = sqlite3_bind_text(p_stmt, 2, value_str, -1, SQLITE_STATIC);
   }

   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to bind second parameter! rc <%d> <%s>", rc, ctrlm_db_errmsg(rc));
      sqlite3_finalize(p_stmt);
      return(-1);
   }

   rc = sqlite3_step(p_stmt);
   if(rc == SQLITE_ROW){
      XLOGD_ERROR("Unable to step! <%s> rc <%d> <%s>", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      sqlite3_finalize(p_stmt);
      return(-1);
   }

   rc = sqlite3_finalize(p_stmt);
   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to finalize! <%s> rc <%d> <%s>", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      return(-1);
   }
   return(0);
}

// Update an existing key/value pair in the database
int ctrlm_db_update(const char *table, const char *key, const int *value_int, const char *value_str, guint32 blob_length) {
   sqlite3_stmt *p_stmt = NULL;
   int rc;
   stringstream sql;
   sql << "UPDATE " << table << " SET value=? WHERE key=?;";

   if(table == NULL || key == NULL) {
      XLOGD_ERROR("Invalid table or key");
      return(-1);
   }
   if(value_int == NULL && value_str == NULL) {
      XLOGD_ERROR("Invalid values");
      return(-1);
   } else if(value_int != NULL && value_str != NULL) {
      XLOGD_ERROR("Multiple values specified");
      return(-1);
   }

   rc = sqlite3_prepare(g_ctrlm_db.handle, sql.str().c_str(), -1, &p_stmt, 0);
   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to prepare sql statement <%s> rc <%d> <%s>!", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      return(-1);
   }

   if(value_int != NULL) { // integer data
      rc = sqlite3_bind_int(p_stmt, 1, *value_int);
   } else if(blob_length != 0) { // blob of data
      rc = sqlite3_bind_blob(p_stmt, 1, value_str, blob_length, SQLITE_STATIC);
   } else { // string data
      rc = sqlite3_bind_text(p_stmt, 1, value_str, -1, SQLITE_STATIC);
   }

   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to bind first parameter! rc <%d> <%s>", rc, ctrlm_db_errmsg(rc));
      sqlite3_finalize(p_stmt);
      return(-1);
   }

   rc = sqlite3_bind_text(p_stmt, 2, key, -1, SQLITE_STATIC); // key name is the second variable
   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to bind second parameter! rc <%d> <%s>", rc, ctrlm_db_errmsg(rc));
      sqlite3_finalize(p_stmt);
      return(-1);
   }

   rc = sqlite3_step(p_stmt);
   if(rc == SQLITE_ROW){
      XLOGD_ERROR("Unable to step! <%s> rc <%d> <%s>", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      sqlite3_finalize(p_stmt);
      return(-1);
   }

   rc = sqlite3_finalize(p_stmt);
   if(rc != SQLITE_OK){
      XLOGD_ERROR("Unable to finalize! <%s> rc <%d> <%s>", sql.str().c_str(), rc, ctrlm_db_errmsg(rc));
      return(-1);
   }

   return(0);
}
#endif

// Select (read) the keys from the specified table
int ctrlm_db_select_keys(const char *table, vector<char *> *keys_str, vector<ctrlm_controller_id_t> *keys_int) {
   int retval = 0;
   if(table == NULL) {
      XLOGD_WARN("invalid parameters!");
      return(-1);
   }
   if(keys_str == NULL && keys_int == NULL) {
      XLOGD_WARN("invalid keys parameters!");
      return(-1);
   }

   string sql = "SELECT key FROM ";
   sql.append(table);
   sql.append(";");

   sqlite3_stmt *p_stmt = NULL;
   int rc =  sqlite3_prepare_v2(g_ctrlm_db.handle, sql.c_str(), sql.length(), &p_stmt, NULL);
   if(rc != SQLITE_OK) {
      XLOGD_TELEMETRY("Unable to prepare sql statement <%s> rc <%d> <%s>!", sql.c_str(), rc, ctrlm_db_errmsg(rc));
      return(-1);
   }

   do {
      rc = sqlite3_step(p_stmt);
      if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
         XLOGD_TELEMETRY("SQL step error: rc <%d> <%s>", rc, ctrlm_db_errmsg(rc));
         sqlite3_finalize(p_stmt);
         return(-1);
      }
      if(rc == SQLITE_ROW) {
         const unsigned char *text = sqlite3_column_text(p_stmt, 0);
         int byte_qty = sqlite3_column_bytes(p_stmt, 0);

         if(keys_int != NULL) { // Integer values
            unsigned long long value_int = strtoll((const char *)text, NULL, 16);
            XLOGD_DEBUG("int 0x%016llX", value_int);
            keys_int->insert(keys_int->end(), value_int);
         }
         if(keys_str != NULL) { // Text values
            XLOGD_DEBUG("str <%s>", text);

            char *value_str = (char *)g_malloc(byte_qty + 1);
            if(value_str == NULL) {
               XLOGD_ERROR("out of memory! %d bytes requested", byte_qty + 1);
            } else {
               errno_t safec_rc = memcpy_s(value_str, byte_qty + 1, text, byte_qty);
               ERR_CHK(safec_rc);
               value_str[byte_qty] = '\0';
               keys_str->insert(keys_str->end(), value_str);
            }
         }
      }
   } while(rc == SQLITE_ROW); // more rows available

   rc = sqlite3_finalize(p_stmt);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL finalize error: <%s> rc <%d> <%s>", sql.c_str(), rc, ctrlm_db_errmsg(rc));
   }

   return(retval);
}

// Select (read) an existing key/value pair from the database
int ctrlm_db_select(const char *table, const char *key, int *value_int, sqlite_uint64 *value_int64, guchar **value_str, guint32 *value_len) {
   int retval = -1;
   errno_t safec_rc = -1;
   if(table == NULL || key == NULL) {
      XLOGD_WARN("invalid parameters!");
      return(retval);
   }
   if(value_int == NULL && value_int64 == NULL && value_str == NULL) {
      XLOGD_ERROR("Invalid values");
      return(retval);
   }

   string sql = "SELECT value FROM ";
   sql.append(table);
   sql.append(" WHERE key='");
   sql.append(key);
   sql.append("';");

   sqlite3_stmt *p_stmt = NULL;
   int rc =  sqlite3_prepare_v2(g_ctrlm_db.handle, sql.c_str(), sql.length(), &p_stmt, NULL);
   if(rc != SQLITE_OK) {
      XLOGD_TELEMETRY("Unable to prepare sql statement <%s> rc <%d> <%s>!", sql.c_str(), rc, ctrlm_db_errmsg(rc));
      return(retval);
   }

   rc = sqlite3_step(p_stmt);
   if(rc != SQLITE_ROW) {
      if(rc != SQLITE_DONE) {  // log an error if the return code is not DONE
         XLOGD_TELEMETRY("SQL step error: <%s> rc <%d> <%s>", sql.c_str(), rc, ctrlm_db_errmsg(rc));
      }
      sqlite3_finalize(p_stmt);
      return(retval);
   }

   if(value_int != NULL) { // Integer value
      *value_int = sqlite3_column_int(p_stmt, 0);
      retval = 0;
   } else if(value_int64 != NULL) {
      *value_int64 = sqlite3_column_int64(p_stmt, 0);
      retval = 0;
   } else if(value_len != NULL) { // Blob value
      const unsigned char *blob = (unsigned char *)sqlite3_column_blob(p_stmt, 0);
      int byte_qty = sqlite3_column_bytes(p_stmt, 0);
      if(blob == NULL || byte_qty <= 0) {
         //XLOGD_ERROR("zero length blob!");
         *value_str = NULL;
         *value_len = 0;
      } else {
         *value_str = (guchar *)g_malloc(byte_qty);
         if(*value_str == NULL) {
            XLOGD_ERROR("out of memory! %d bytes requested", byte_qty);
         } else {
            safec_rc = memcpy_s(*value_str, byte_qty, blob, byte_qty);
            ERR_CHK(safec_rc);
            *value_len = (guint32)byte_qty;
            retval = 0;
         }
      }
   } else { // Text value
      const unsigned char *text = sqlite3_column_text(p_stmt, 0);
      int byte_qty = sqlite3_column_bytes(p_stmt, 0);

      if(text == NULL || byte_qty <= 0) {
         //XLOGD_ERROR("invalid string length %d. <%s>", byte_qty, sql.c_str());
         *value_str = NULL;
      } else {
         *value_str = (guchar *)g_malloc(byte_qty + 1);
         if(*value_str == NULL) {
            XLOGD_ERROR("out of memory! %d bytes requested", byte_qty + 1);
         } else {
            safec_rc = memcpy_s(*value_str, byte_qty + 1, text, byte_qty);
            ERR_CHK(safec_rc);
            (*value_str)[byte_qty] = '\0';
            retval = 0;
         }
      }
   }

   rc = sqlite3_step(p_stmt);
   if(rc == SQLITE_ROW) { // more rows available
      XLOGD_WARN("SQL step more rows available! Something is wrong...");
   }


   rc = sqlite3_finalize(p_stmt);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL finalize error: <%s> rc <%d> <%s>", sql.c_str(), rc, ctrlm_db_errmsg(rc));
   }

   return(retval);
}

// Delete an existing key/value pair from the database
int ctrlm_db_delete(const char *table, const char *key, bool pattern) {
   char *err_msg = NULL;
   if(table == NULL || key == NULL) {
      XLOGD_WARN("invalid parameters!");
      return(-1);
   }

   string sql = "DELETE FROM ";
   sql.append(table);
   if(!pattern) {
      sql.append(" WHERE key='");
      sql.append(key);
      sql.append("';");
   } else {
      sql.append(" WHERE key LIKE '");
      sql.append(key);
      sql.append("';");
   }

   int rc = sqlite3_exec(g_ctrlm_db.handle, sql.c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      if(err_msg) {
         sqlite3_free(err_msg);
      }
      return(-1);
   }

   return(0);
}

bool ctrlm_db_is_initialized() {
   int value;
   if(ctrlm_db_read_int(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CTRLMGR_KEY_VERSION, &value)) {
      XLOGD_INFO("SQL DB not present");
      return(false);
   }

   return(true);
}

gboolean ctrlm_db_created_default(void) {
   return(g_ctrlm_db.created_default_db);
}

void ctrlm_db_ir_controller_create(std::string &table) {
   char *err_msg = NULL;

   table = string(CTRLM_DB_TABLE_IR_CONTROLLER);

   // Create the IR controller table
   stringstream sql_global;
   sql_global << "CREATE TABLE IF NOT EXISTS " << CTRLM_DB_TABLE_IR_CONTROLLER << "(key TEXT PRIMARY KEY, value TEXT);";
   int rc = sqlite3_exec(g_ctrlm_db.handle, sql_global.str().c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      if(err_msg) {
         sqlite3_free(err_msg);
      }
      return;
   }
}

void ctrlm_db_voice_create() {
   if(!ctrlm_db_table_exists(CTRLM_DB_TABLE_VOICE)) {
      char *err_msg = NULL;
      g_ctrlm_db.voice_is_valid = false; // This is needed to allow voice component to transfer to new DB structure
      // Create the voice table
      int rc = sqlite3_exec(g_ctrlm_db.handle, "CREATE TABLE " CTRLM_DB_TABLE_VOICE "(key TEXT PRIMARY KEY, value TEXT);", NULL, NULL, &err_msg);
      if(rc != SQLITE_OK) {
         XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
         if(err_msg) {
            sqlite3_free(err_msg);
         }
      }
      else {
         XLOGD_INFO("voice DB created");
      }
   }
}

void ctrlm_db_default() {
   char *err_msg = NULL;

   // Create the global table
   int rc = sqlite3_exec(g_ctrlm_db.handle, "CREATE TABLE " CTRLM_DB_TABLE_CTRLMGR "(key TEXT PRIMARY KEY, value TEXT);", NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      if(err_msg) {
         sqlite3_free(err_msg);
      }
      return;
   }

   // insert the 'version' key value pair
   if(ctrlm_db_insert_or_update(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CTRLMGR_KEY_VERSION, NULL, NULL, (const guchar *)CTRLM_DB_VERSION, 0)) {
      XLOGD_ERROR("error unable to insert version");
   }

   // insert the last download session id key value pair
   ctrlm_db_write_uint64(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_DEVICE_UPDATE_KEY_SESSION_ID, (sqlite_uint64)CTRLM_DB_DEVICE_UPDATE_SESSION_ID_DEFAULT);

   ctrlm_db_default_networks();

   ctrlm_db_voice_create();

   XLOGD_INFO("initialized to defaults");
}

void ctrlm_db_default_networks() {
   vector<ctrlm_network_id_t> network_list;

   ctrlm_network_list_get(&network_list);

   XLOGD_INFO("%u networks", network_list.size());

   // For each RF4CE network...
   for(unsigned long index = 0; index < network_list.size(); index++) {
      ctrlm_network_id_t network_id = network_list[index];

      XLOGD_INFO("Checking Network Id %u", network_id);

      switch(ctrlm_network_type_get(network_id)) {
         case CTRLM_NETWORK_TYPE_RF4CE:
         {
            XLOGD_INFO("RF4CE Network Id %u", network_id);
            ctrlm_db_rf4ce_network_create(network_id);
            break;
         }
         case CTRLM_NETWORK_TYPE_IP:
         {
            XLOGD_INFO("IP Network Id %u", network_id);
            ctrlm_db_ip_network_create(network_id);
            break;
         }
         case CTRLM_NETWORK_TYPE_BLUETOOTH_LE:
         {
            XLOGD_INFO("BLE Network Id %u", network_id);
            ctrlm_db_ble_network_create(network_id);
            break;
         }
         default:
         {
            break;
         }
      }
   }
}

void ctrlm_db_cache() {

   // Check to make sure DB is not empty
   if(!ctrlm_db_is_initialized()) {
      g_ctrlm_db.created_default_db = true;
      ctrlm_db_default();  // Initialize DB with default data
   }

   // Read control manager global options
   ctrlm_db_read_globals(&g_ctrlm_db_global);

   // make sure we have all the networks we are supposed to in case more are added later
   ctrlm_network_list_get(&g_ctrlm_db.rf4ce_network_list);
   ctrlm_network_list_get(&g_ctrlm_db.ip_network_list);
   ctrlm_network_list_get(&g_ctrlm_db.ble_network_list);

   XLOGD_INFO("%u networks", g_ctrlm_db.rf4ce_network_list.size());

   // Remove non RF4CE networks...
   for(vector<ctrlm_network_id_t>::iterator it = g_ctrlm_db.rf4ce_network_list.begin(); it < g_ctrlm_db.rf4ce_network_list.end(); it++) {
      if(ctrlm_network_type_get(*it) != CTRLM_NETWORK_TYPE_RF4CE) {
         it = g_ctrlm_db.rf4ce_network_list.erase(it);
      }
   }

   // Remove non IP networks...
   for(vector<ctrlm_network_id_t>::iterator it = g_ctrlm_db.ip_network_list.begin(); it < g_ctrlm_db.ip_network_list.end(); it++) {
      if(ctrlm_network_type_get(*it) != CTRLM_NETWORK_TYPE_IP) {
         it = g_ctrlm_db.ip_network_list.erase(it);
      }
   }

   // Remove non BLE networks...
   for(vector<ctrlm_network_id_t>::iterator it = g_ctrlm_db.ble_network_list.begin(); it < g_ctrlm_db.ble_network_list.end(); it++) {
      if(ctrlm_network_type_get(*it) != CTRLM_NETWORK_TYPE_BLUETOOTH_LE) {
         it = g_ctrlm_db.ble_network_list.erase(it);
      }
   }

   XLOGD_INFO("%u RF4CE networks", g_ctrlm_db.rf4ce_network_list.size());

   // Check to make sure we have all the expected networks
   for(vector<ctrlm_network_id_t>::iterator it = g_ctrlm_db.rf4ce_network_list.begin(); it < g_ctrlm_db.rf4ce_network_list.end(); it++) {
      XLOGD_INFO("Checking RF4CE Network Id %u", *it);

      // This will create the network tables if they are not already present
      ctrlm_db_rf4ce_network_create(*it);
   }

   XLOGD_INFO("%u IP networks", g_ctrlm_db.ip_network_list.size());

   // Check to make sure we have all the expected networks
   for(vector<ctrlm_network_id_t>::iterator it = g_ctrlm_db.ip_network_list.begin(); it < g_ctrlm_db.ip_network_list.end(); it++) {
      XLOGD_INFO("Checking IP Network Id %u", *it);

      // This will create the network tables if they are not already present
      ctrlm_db_ip_network_create(*it);
   }

   XLOGD_INFO("%u BLE networks", g_ctrlm_db.ble_network_list.size());

   // Check to make sure we have all the expected networks
   for(vector<ctrlm_network_id_t>::iterator it = g_ctrlm_db.ble_network_list.begin(); it < g_ctrlm_db.ble_network_list.end(); it++) {
      XLOGD_INFO("Checking BLE Network Id %u", *it);

      // This will create the network tables if they are not already present
      ctrlm_db_ble_network_create(*it);
   }

   // Create Voice table if it doesn't exist
   ctrlm_db_voice_create();
}

void ctrlm_db_print() {

   XLOGD_INFO("--- Control Manager ---");
   XLOGD_INFO("Database version %d", g_ctrlm_db_global.version);
   XLOGD_INFO("Last device update session ID %llu", g_ctrlm_db_global.device_update_session_id);

   vector<ctrlm_network_id_t> network_ids;
   ctrlm_db_rf4ce_networks_list(&network_ids);

   for(vector<ctrlm_network_id_t>::iterator it_network = network_ids.begin(); it_network < network_ids.end(); it_network++) {
      vector<ctrlm_controller_id_t> controller_ids;

      XLOGD_INFO("RF4CE Network Id %u", *it_network);
      ctrlm_db_rf4ce_controllers_list(*it_network, &controller_ids);

      if(controller_ids.size() == 0) {
         XLOGD_INFO("<No Controllers>");
      }
      for(vector<ctrlm_controller_id_t>::iterator it_controller = controller_ids.begin(); it_controller < controller_ids.end(); it_controller++) {
         unsigned long long          ieee_address = 0;
         ctrlm_rcu_binding_type_t    binding_type = CTRLM_RCU_BINDING_TYPE_INVALID;
         ctrlm_rcu_validation_type_t validation_type = CTRLM_RCU_VALIDATION_TYPE_INVALID;
         time_t                      time_binding = 0;
         char                        time_str[40] = "";

         ctrlm_db_rf4ce_read_ieee_address(*it_network, *it_controller, &ieee_address);
         ctrlm_db_rf4ce_read_binding_type(*it_network, *it_controller, &binding_type);
         ctrlm_db_rf4ce_read_validation_type(*it_network, *it_controller, &validation_type);
         ctrlm_db_rf4ce_read_time_binding(*it_network, *it_controller, &time_binding);

         const tm* loc_time = localtime(&time_binding);
         if (loc_time != 0) {
            strftime(time_str, 40, "%x - %I:%M:%S %p", loc_time);
         }

         XLOGD_INFO("Controller Id %u IEEE 0x%016llX Binding <%s> Validation <%s> Bound <%s>", *it_controller, ieee_address, ctrlm_rcu_binding_type_str(binding_type), ctrlm_rcu_validation_type_str(validation_type), time_str);

      }
   }
}

void ctrlm_db_read_globals(ctrlm_db_global_data_t *db) {
   int     version                  = 0;
   guint64 device_update_session_id = 0;

   ctrlm_db_read_int(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_CTRLMGR_KEY_VERSION, &version);
   ctrlm_db_read_uint64(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_DEVICE_UPDATE_KEY_SESSION_ID, (sqlite_uint64*)&device_update_session_id);

   db->version                     = version;
   db->device_update_session_id    = device_update_session_id;
}

guint32 ctrlm_db_global_version_get() {
    return(g_ctrlm_db_global.version);
}

void ctrlm_db_rf4ce_networks_list(vector<ctrlm_network_id_t> *network_ids) {
   *network_ids = g_ctrlm_db.rf4ce_network_list;
}

void ctrlm_db_rf4ce_controllers_list(ctrlm_network_id_t network_id, vector<ctrlm_controller_id_t> *controller_ids) {
   char table_name_controller_list[CONTROLLER_TABLE_NAME_MAX_LEN];

   ctrlm_db_rf4ce_controller_list_table_name(network_id, table_name_controller_list);

   // Read the controller list from the database
   ctrlm_db_select_keys(table_name_controller_list, NULL, controller_ids);
}

void ctrlm_db_rf4ce_network_create(ctrlm_network_id_t network_id) {
   char table_name_controller_list[CONTROLLER_TABLE_NAME_MAX_LEN];
   char table_name_global[CONTROLLER_TABLE_NAME_MAX_LEN];
   char *err_msg = NULL;

   ctrlm_db_rf4ce_global_table_name(network_id, table_name_global);
   ctrlm_db_rf4ce_controller_list_table_name(network_id, table_name_controller_list);

   // Create the rf4ce global table
   stringstream sql_global;
   sql_global << "CREATE TABLE IF NOT EXISTS " << table_name_global << "(key TEXT PRIMARY KEY, value TEXT);";
   int rc = sqlite3_exec(g_ctrlm_db.handle, sql_global.str().c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      if(err_msg) {
         sqlite3_free(err_msg);
      }
      return;
   }

   // Create the rf4ce controllers table
   stringstream sql_controllers;
   sql_controllers << "CREATE TABLE IF NOT EXISTS " << table_name_controller_list << "(key TEXT PRIMARY KEY, value TEXT);";
   rc = sqlite3_exec(g_ctrlm_db.handle, sql_controllers.str().c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", err_msg, rc, ctrlm_db_errmsg(rc));
      sqlite3_free(err_msg);
      return;
   }
}

void ctrlm_db_rf4ce_controller_create(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   ctrlm_db_queue_msg_controller_t *msg = (ctrlm_db_queue_msg_controller_t *)g_malloc(sizeof(ctrlm_db_queue_msg_controller_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   msg->header.type   = CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_CREATE;
   msg->network_id    = network_id;
   msg->controller_id = controller_id;

   ctrlm_db_queue_msg_push((gpointer)msg);
}

void ctrlm_db_rf4ce_controller_destroy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {

   ctrlm_db_queue_msg_controller_t *msg = (ctrlm_db_queue_msg_controller_t *)g_malloc(sizeof(ctrlm_db_queue_msg_controller_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   msg->header.type   = CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_DESTROY;
   msg->network_id    = network_id;
   msg->controller_id = controller_id;

   ctrlm_db_queue_msg_push((gpointer)msg);
}

int ctrlm_db_integrity_check_result(void *param, int argc, char **argv, char **column) {
   bool& db_error = *(bool*)param;
   errno_t safec_rc = -1;
   int ind = -1;
   size_t strSize = strlen("ok");
   for (int i=0; i < argc; ++i) {
      safec_rc = strcmp_s("ok", strSize, argv[i], &ind);
      ERR_CHK(safec_rc);
      if ((safec_rc == EOK) &&(ind != 0)) {
         XLOGD_ERROR("%s", argv[i]);
         db_error = true;
      } else {
         XLOGD_INFO("healthy");
      }
   }
   return 0;
}

bool ctrlm_db_verify_integrity() {
   char *err_msg = NULL;
   bool db_error = false;
   // Execute pragma integrity_check
   int rc = sqlite3_exec(g_ctrlm_db.handle, "PRAGMA integrity_check;", ctrlm_db_integrity_check_result, &db_error, &err_msg);
   if(rc != SQLITE_OK || db_error) {
      XLOGD_TELEMETRY("integrity_check failed %s", (err_msg ? err_msg : ""));
      if(err_msg) {
         sqlite3_free(err_msg);
      }
      return(FALSE);
   }
   return(TRUE);
}

bool ctrlm_db_vacuum() {
   int rc = sqlite3_exec(g_ctrlm_db.handle, "VACUUM;", NULL, NULL, NULL);
   if(rc != SQLITE_OK) {
      XLOGD_ERROR("database vacuum attempt failed. rc <%d> <%s>", rc, ctrlm_db_errmsg(rc));
   }
   return (rc == SQLITE_OK);
}

void ctrlm_db_rf4ce_global_table_name(ctrlm_network_id_t network_id, char *table) {
   errno_t safec_rc = sprintf_s(table, CONTROLLER_TABLE_NAME_MAX_LEN, CTRLM_DB_TABLE_NET_RF4CE_GLOBAL, network_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
}

void ctrlm_db_rf4ce_controller_list_table_name(ctrlm_network_id_t network_id, char *table) {
   errno_t safec_rc = sprintf_s(table, CONTROLLER_TABLE_NAME_MAX_LEN, CTRLM_DB_TABLE_NET_RF4CE_CONTROLLER_LIST, network_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
}

void ctrlm_db_rf4ce_controller_entry_table_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, char *table) {
   errno_t safec_rc = sprintf_s(table, CONTROLLER_TABLE_NAME_MAX_LEN, CTRLM_DB_TABLE_NET_RF4CE_CONTROLLER_ENTRY, network_id, controller_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }

   // TODO would it be better to look up the table name from the controller list instead?
}

void ctrlm_db_rf4ce_write_dsp_configuration_xr19(ctrlm_network_id_t network_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_global_table_name(network_id, table);

   ctrlm_db_write_blob(table, "dsp_configuration_xr19", data, length);
}

void ctrlm_db_rf4ce_read_dsp_configuration_xr19(ctrlm_network_id_t network_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_global_table_name(network_id, table);

   ctrlm_db_read_blob(table, "dsp_configuration_xr19", data, length);
}

void ctrlm_db_rf4ce_read_ieee_address(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned long long *ieee_address) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_uint64(table, "ieee_address", ieee_address);
}

void ctrlm_db_rf4ce_read_binding_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_type_t *binding_type) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "binding_type", &value);
   *binding_type = (ctrlm_rcu_binding_type_t) value;
}

void ctrlm_db_rf4ce_write_binding_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_type_t binding_type) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "binding_type", binding_type);
}

void ctrlm_db_rf4ce_read_validation_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_validation_type_t *validation_type) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "validation_type", &value);
   *validation_type = (ctrlm_rcu_validation_type_t) value;
}

void ctrlm_db_rf4ce_write_validation_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_validation_type_t validation_type) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "validation_type", validation_type);
}

void ctrlm_db_rf4ce_read_time_binding(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_binding) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "time_binding", &value);
   *time_binding = (time_t) value;
}

void ctrlm_db_rf4ce_read_time_last_heartbeat(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_last_heartbeat) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "time_last_heartbeat", &value);
   *time_last_heartbeat = (time_t) value;
}

void ctrlm_db_rf4ce_write_time_last_heartbeat(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t time_last_heartbeat) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "time_last_heartbeat", time_last_heartbeat);
}

void ctrlm_db_rf4ce_read_peripheral_id(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "peripheral_id", data, length);
}

void ctrlm_db_rf4ce_write_peripheral_id(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "peripheral_id", data, length);
}

void ctrlm_db_rf4ce_read_rf_statistics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "rf_statistics", data, length);
}

void ctrlm_db_rf4ce_write_rf_statistics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "rf_statistics", data, length);
}

void ctrlm_db_rf4ce_read_voice_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "voice_cmd_counts", data, length);
}

void ctrlm_db_rf4ce_write_voice_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "voice_cmd_counts", data, length);
}

void ctrlm_db_rf4ce_read_firmware_updated(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "firmware_updated", data, length);
}

void ctrlm_db_rf4ce_write_firmware_updated(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "firmware_updated", data, length);
}

void ctrlm_db_rf4ce_read_reboot_diagnostics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "reboot_diagnostics", data, length);
}

void ctrlm_db_rf4ce_write_reboot_diagnostics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "reboot_diagnostics", data, length);
}

void ctrlm_db_rf4ce_read_memory_statistics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "memory_statistics", data, length);
}

void ctrlm_db_rf4ce_write_memory_statistics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "memory_statistics", data, length);
}

void ctrlm_db_rf4ce_read_time_last_checkin_for_device_update(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "last_checkin_for_device_update", data, length);
}

void ctrlm_db_rf4ce_write_time_last_checkin_for_device_update(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "last_checkin_for_device_update", data, length);
}

void ctrlm_db_rf4ce_read_polling_methods(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 *polling_methods) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   guint64 temp_polling_methods = 0;
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_uint64(table, "polling_methods", (sqlite_uint64*)&temp_polling_methods);
   if(polling_methods) *polling_methods = (guint8) temp_polling_methods;
}

void ctrlm_db_rf4ce_write_polling_methods(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 polling_methods) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "polling_methods", polling_methods);
}

void ctrlm_db_rf4ce_read_polling_configuration_mac(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "polling_configuration_mac", data, length);
}

void ctrlm_db_rf4ce_write_polling_configuration_mac(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "polling_configuration_mac", data, length);
}

void ctrlm_db_rf4ce_read_polling_configuration_heartbeat(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "polling_configuration_heartbeat", data, length);
}

void ctrlm_db_rf4ce_write_polling_configuration_heartbeat(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "polling_configuration_heartbeat", data, length);
}

void ctrlm_db_rf4ce_read_rib_configuration_complete(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, int *status) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "rib_configuration_complete", &value);
   *status = (int) value;
}

void ctrlm_db_rf4ce_write_rib_configuration_complete(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, int status) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "rib_configuration_complete", status);
}

void ctrlm_db_rf4ce_read_binding_security_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_security_type_t *type) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "binding_security_type", &value);
   *type = (ctrlm_rcu_binding_security_type_t) value;
}

void ctrlm_db_rf4ce_write_binding_security_type(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_rcu_binding_security_type_t type) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "binding_security_type", type);
}

#ifdef ASB
void ctrlm_db_rf4ce_read_asb_key_derivation_method(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned char *method) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "asb_key_derivation_method", &value);
   *method = (unsigned char) value;
}

void ctrlm_db_rf4ce_write_asb_key_derivation_method(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned char method) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "asb_key_derivation_method", method);
}
#endif

void ctrlm_db_rf4ce_read_privacy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "privacy", data, length);
}

void ctrlm_db_rf4ce_write_controller_capabilities(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, const guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "controller_capabilities", data, length);
}

void ctrlm_db_rf4ce_read_controller_capabilities(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "controller_capabilities", data, length);
}

void ctrlm_db_rf4ce_write_privacy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "privacy", data, length);
}

void ctrlm_db_rf4ce_read_far_field_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "far_field_metrics", data, length);
}

void ctrlm_db_rf4ce_write_far_field_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "far_field_metrics", data, length);
}

void ctrlm_db_rf4ce_read_dsp_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "dsp_metrics", data, length);
}

void ctrlm_db_rf4ce_write_dsp_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "dsp_metrics", data, length);
}

void ctrlm_db_rf4ce_read_time_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_metrics) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "time_metrics", &value);
   *time_metrics = (time_t) value;
}

void ctrlm_db_rf4ce_write_time_metrics(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t time_metrics) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "time_metrics", time_metrics);
}

void ctrlm_db_rf4ce_read_uptime_privacy_info(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "uptime_privacy_info", data, length);
}

void ctrlm_db_rf4ce_write_uptime_privacy_info(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "uptime_privacy_info", data, length);
}

void ctrlm_db_rf4ce_read_device_update_session_state(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   ctrlm_db_read_blob(table, CTRLM_DB_DEVICE_UPDATE_SESSION_STATE, data, length);
}

void ctrlm_db_rf4ce_write_device_update_session_state(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   ctrlm_db_write_blob(table, CTRLM_DB_DEVICE_UPDATE_SESSION_STATE, data, length);
}

void ctrlm_db_rf4ce_destroy_device_update_session_state(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);
   int rc;
   if(ctrlm_db_key_exists(table, CTRLM_DB_DEVICE_UPDATE_SESSION_STATE)) {
      // Delete the entry in controller's table
      rc = ctrlm_db_delete(table, CTRLM_DB_DEVICE_UPDATE_SESSION_STATE);
      if(rc != 0) {
         XLOGD_INFO("error deleting key %s", CTRLM_DB_DEVICE_UPDATE_SESSION_STATE);
      }
   }
}

bool ctrlm_db_rf4ce_exists_device_update_session_state(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   return(ctrlm_db_key_exists(table, CTRLM_DB_DEVICE_UPDATE_SESSION_STATE));
}

void ctrlm_db_write_file_(const char *path, const guchar *data, guint32 length) {
    // will not throw exceptions in case of error unless exception mask is configured explicitly
    ofstream file(path, ios::out | ios::binary);
    if(file.is_open() == false) {
       XLOGD_ERROR("Cannot open crash dump file %s", path);
       return;
    }
    file.write((char *)data, length).flush();
    if(file.rdstate() & ofstream::failbit) {
       XLOGD_ERROR("Error writing crash dump to %s", path);
    }
}

void ctrlm_db_rf4ce_write_file(const char *path, guchar *data, guint32 length) {
   int path_len = strlen(path) + 1;
   ctrlm_db_queue_msg_write_file_t *msg = (ctrlm_db_queue_msg_write_file_t *)g_malloc(sizeof(ctrlm_db_queue_msg_write_file_t) + path_len);
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   msg->header.type = CTRLM_DB_QUEUE_MSG_TYPE_WRITE_FILE;
   msg->path =  (char *)&msg[1];
   errno_t safec_rc = memcpy_s(msg->path, path_len, path, path_len);
   ERR_CHK(safec_rc);
   msg->data  = data;
   msg->length = length; // length of externally allocated buffer

   ctrlm_db_queue_msg_push((gpointer)msg);
}

void ctrlm_db_rf4ce_read_mfg_test_result(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "mfg_test_result", data, length);
}

void ctrlm_db_rf4ce_write_mfg_test_result(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "mfg_test_result", data, length);
}

void ctrlm_db_rf4ce_read_ota_failures_type_z_count(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "ota_failures", data, length);
}

void ctrlm_db_rf4ce_write_ota_failures_type_z_count(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 ota_failures) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   guchar data[1] = {0};
   ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table);

   data[0] = (guchar) ota_failures;
   ctrlm_db_write_blob(table, "ota_failures", data, 1);
}

void ctrlm_db_voice_settings_remove() {
   int rc;
   if(ctrlm_db_key_exists(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_VOICE_KEY_SETTINGS)) {
      // Delete the entry in global DB
      rc = ctrlm_db_delete(CTRLM_DB_TABLE_CTRLMGR, CTRLM_DB_VOICE_KEY_SETTINGS);
      if(rc != 0) {
         XLOGD_INFO("error deleting key %s", CTRLM_DB_VOICE_KEY_SETTINGS);
      }
   }
}

void ctrlm_db_controller_destroy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   char table_name_controller_list[CONTROLLER_TABLE_NAME_MAX_LEN];
   char table_name_controller_entry[CONTROLLER_TABLE_NAME_MAX_LEN];
   char key[3];
   int  rc;

   XLOGD_INFO("network id %u controller id %u", network_id, controller_id);

   switch(ctrlm_network_type_get(network_id)) {
      case CTRLM_NETWORK_TYPE_IP:
      {
         ctrlm_db_ip_controller_list_table_name(network_id, table_name_controller_list);
         ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table_name_controller_entry);
         break;
      }
      case CTRLM_NETWORK_TYPE_BLUETOOTH_LE:
      {
         ctrlm_db_ble_controller_list_table_name(network_id, table_name_controller_list);
         ctrlm_db_ble_controller_entry_table_name(network_id, controller_id, table_name_controller_entry);
         break;
      }
      case CTRLM_NETWORK_TYPE_RF4CE:
      default:
      {
         ctrlm_db_rf4ce_controller_list_table_name(network_id, table_name_controller_list);
         ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table_name_controller_entry);
         break;
      }
   }

   errno_t safec_rc = sprintf_s(key, sizeof(key), "%02X", controller_id);
   if(safec_rc < EOK) {
       ERR_CHK(safec_rc);
   }

   // Delete the entry in the RF4CE controller table
   if(!ctrlm_db_key_exists(table_name_controller_list, key)) {
      XLOGD_WARN("key %s doesn't exist", key);
   } else {
      // Delete the entry in the controller list
      rc = ctrlm_db_delete(table_name_controller_list, key);
      if(rc != 0) {
         XLOGD_INFO("error deleting key %s", key);
      }
   }

   // Delete the controller's table
   stringstream sql;
   char *err_msg = NULL;
   sql << "DROP TABLE IF EXISTS " << table_name_controller_entry << ";";
   rc = sqlite3_exec(g_ctrlm_db.handle, sql.str().c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      if(err_msg) {
         sqlite3_free(err_msg);
      }
      return;
   }
}

void ctrlm_db_controller_create(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   char table_name_controller_list[CONTROLLER_TABLE_NAME_MAX_LEN];
   char table_name_controller_entry[CONTROLLER_TABLE_NAME_MAX_LEN];
   char key[3];

   XLOGD_INFO("network id %u controller id %u", network_id, controller_id);

   switch(ctrlm_network_type_get(network_id)) {
      case CTRLM_NETWORK_TYPE_IP:
      {
         ctrlm_db_ip_controller_list_table_name(network_id, table_name_controller_list);
         ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table_name_controller_entry);
         break;
      }
      case CTRLM_NETWORK_TYPE_BLUETOOTH_LE:
      {
         ctrlm_db_ble_controller_list_table_name(network_id, table_name_controller_list);
         ctrlm_db_ble_controller_entry_table_name(network_id, controller_id, table_name_controller_entry);
         break;
      }
      case CTRLM_NETWORK_TYPE_RF4CE:
      default:
      {
         ctrlm_db_rf4ce_controller_list_table_name(network_id, table_name_controller_list);
         ctrlm_db_rf4ce_controller_entry_table_name(network_id, controller_id, table_name_controller_entry);
         break;
      }
   }

   errno_t safec_rc = sprintf_s(key, sizeof(key), "%02X", controller_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }

   // Create an entry in the RF4CE controller table pointing to the controller's table name
   if(ctrlm_db_key_exists(table_name_controller_list, key)) {
      XLOGD_WARN("key %s already exists", key);
   } else {
      // Add an entry to the controller list
      ctrlm_db_write_str(table_name_controller_list, key, (guchar *)table_name_controller_entry);
   }

   // Create the controller's table
   char *err_msg = NULL;
   stringstream sql;
   sql << "CREATE TABLE IF NOT EXISTS " << table_name_controller_entry << "(key TEXT PRIMARY KEY, value TEXT);";
   int rc = sqlite3_exec(g_ctrlm_db.handle, sql.str().c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      if(err_msg) {
         sqlite3_free(err_msg);
      }
      return;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IP Network Database Code
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ctrlm_db_ip_controllers_list(ctrlm_network_id_t network_id, vector<ctrlm_controller_id_t> *controller_ids) {
   char table_name_controller_list[CONTROLLER_TABLE_NAME_MAX_LEN];

   ctrlm_db_ip_controller_list_table_name(network_id, table_name_controller_list);

   // Read the controller list from the database
   ctrlm_db_select_keys(table_name_controller_list, NULL, controller_ids);
}

void ctrlm_db_ip_network_create(ctrlm_network_id_t network_id) {
   char table_name_controller_list[CONTROLLER_TABLE_NAME_MAX_LEN];
   char table_name_global[CONTROLLER_TABLE_NAME_MAX_LEN];
   char *err_msg = NULL;

   ctrlm_db_ip_global_table_name(network_id, table_name_global);
   ctrlm_db_ip_controller_list_table_name(network_id, table_name_controller_list);

   // Create the ip global table
   stringstream sql_global;
   sql_global << "CREATE TABLE IF NOT EXISTS " << table_name_global << "(key TEXT PRIMARY KEY, value TEXT);";
   int rc = sqlite3_exec(g_ctrlm_db.handle, sql_global.str().c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      if(err_msg) {
         sqlite3_free(err_msg);
      }
      return;
   }

   // Create the ip controllers table
   stringstream sql_controllers;
   sql_controllers << "CREATE TABLE IF NOT EXISTS " << table_name_controller_list << "(key TEXT PRIMARY KEY, value TEXT);";
   rc = sqlite3_exec(g_ctrlm_db.handle, sql_controllers.str().c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>",  (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      sqlite3_free(err_msg);
      return;
   }
}

void ctrlm_db_ip_controller_create(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   ctrlm_db_queue_msg_controller_t *msg = (ctrlm_db_queue_msg_controller_t *)g_malloc(sizeof(ctrlm_db_queue_msg_controller_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   msg->header.type   = CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_CREATE;
   msg->network_id    = network_id;
   msg->controller_id = controller_id;

   ctrlm_db_queue_msg_push((gpointer)msg);
}

void ctrlm_db_ip_controller_destroy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {

   ctrlm_db_queue_msg_controller_t *msg = (ctrlm_db_queue_msg_controller_t *)g_malloc(sizeof(ctrlm_db_queue_msg_controller_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   msg->header.type   = CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_DESTROY;
   msg->network_id    = network_id;
   msg->controller_id = controller_id;

   ctrlm_db_queue_msg_push((gpointer)msg);
}

void ctrlm_db_ip_global_table_name(ctrlm_network_id_t network_id, char *table) {
   errno_t safec_rc = sprintf_s(table, CONTROLLER_TABLE_NAME_MAX_LEN, CTRLM_DB_TABLE_NET_IP_GLOBAL, network_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
}

void ctrlm_db_ip_controller_list_table_name(ctrlm_network_id_t network_id, char *table) {
   errno_t safec_rc = sprintf_s(table, CONTROLLER_TABLE_NAME_MAX_LEN, CTRLM_DB_TABLE_NET_IP_CONTROLLER_LIST, network_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
}

void ctrlm_db_ip_controller_entry_table_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, char *table) {
   errno_t safec_rc = sprintf_s(table, CONTROLLER_TABLE_NAME_MAX_LEN, CTRLM_DB_TABLE_NET_IP_CONTROLLER_ENTRY, network_id, controller_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }

   // TODO would it be better to look up the table name from the controller list instead?
}

void ctrlm_db_ip_read_controller_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "controller_name", data, length);
}

void ctrlm_db_ip_write_controller_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "controller_name", data, length);
}

void ctrlm_db_ip_read_controller_manufacturer(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "controller_manufacturer", data, length);
}

void ctrlm_db_ip_write_controller_manufacturer(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "controller_manufacturer", data, length);
}

void ctrlm_db_ip_read_controller_model(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "controller_model", data, length);
}

void ctrlm_db_ip_write_controller_model(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "controller_model", data, length);
}

void ctrlm_db_ip_read_authentication_token(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar **data, guint32 *length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_read_blob(table, "authentication_token", data, length);
}

void ctrlm_db_ip_write_authentication_token(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guchar *data, guint32 length) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_blob(table, "authentication_token", data, length);
}

void ctrlm_db_ip_read_time_binding(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_binding) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "time_binding", &value);
   *time_binding = (time_t) value;
}

void ctrlm_db_ip_write_time_binding(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t time_binding) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "time_binding", time_binding);
}

void ctrlm_db_ip_read_time_last_key(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t *time_last_key) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "time_last_key", &value);
   *time_last_key = (time_t) value;
}

void ctrlm_db_ip_write_time_last_key(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, time_t time_last_key) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "time_last_key", time_last_key);
}

void ctrlm_db_ip_read_permissions(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned char *permissions) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);
   unsigned long long value = 0;
   ctrlm_db_read_uint64(table, "permissions", &value);
   *permissions = (unsigned char) value;
}

void ctrlm_db_ip_write_permissions(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, unsigned char permissions) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ip_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "permissions", permissions);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BLE Network Database Code
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ctrlm_db_ble_controllers_list(ctrlm_network_id_t network_id, vector<ctrlm_controller_id_t> *controller_ids) {
   char table_name_controller_list[CONTROLLER_TABLE_NAME_MAX_LEN];

   ctrlm_db_ble_controller_list_table_name(network_id, table_name_controller_list);

   // Read the controller list from the database
   ctrlm_db_select_keys(table_name_controller_list, NULL, controller_ids);
}

void ctrlm_db_ble_network_create(ctrlm_network_id_t network_id) {
   char table_name_controller_list[CONTROLLER_TABLE_NAME_MAX_LEN];
   char table_name_global[CONTROLLER_TABLE_NAME_MAX_LEN];
   char *err_msg = NULL;

   ctrlm_db_ble_global_table_name(network_id, table_name_global);
   ctrlm_db_ble_controller_list_table_name(network_id, table_name_controller_list);

   // Create the ble global table
   stringstream sql_global;
   sql_global << "CREATE TABLE IF NOT EXISTS " << table_name_global << "(key TEXT PRIMARY KEY, value TEXT);";
   int rc = sqlite3_exec(g_ctrlm_db.handle, sql_global.str().c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      if(err_msg) {
         sqlite3_free(err_msg);
      }
      return;
   }

   // Create the ble controllers table
   stringstream sql_controllers;
   sql_controllers << "CREATE TABLE IF NOT EXISTS " << table_name_controller_list << "(key TEXT PRIMARY KEY, value TEXT);";
   rc = sqlite3_exec(g_ctrlm_db.handle, sql_controllers.str().c_str(), NULL, NULL, &err_msg);
   if(rc != SQLITE_OK) {
      XLOGD_INFO("SQL error: errmsg <%s> rc <%d> <%s>", (err_msg ? err_msg : ""), rc, ctrlm_db_errmsg(rc));
      sqlite3_free(err_msg);
      return;
   }
}

void ctrlm_db_ble_controller_create(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   ctrlm_db_queue_msg_controller_t *msg = (ctrlm_db_queue_msg_controller_t *)g_malloc(sizeof(ctrlm_db_queue_msg_controller_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   msg->header.type   = CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_CREATE;
   msg->network_id    = network_id;
   msg->controller_id = controller_id;

   ctrlm_db_queue_msg_push((gpointer)msg);
}

void ctrlm_db_ble_controller_destroy(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {

   ctrlm_db_queue_msg_controller_t *msg = (ctrlm_db_queue_msg_controller_t *)g_malloc(sizeof(ctrlm_db_queue_msg_controller_t));
   if(msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   msg->header.type   = CTRLM_DB_QUEUE_MSG_TYPE_CONTROLLER_DESTROY;
   msg->network_id    = network_id;
   msg->controller_id = controller_id;

   ctrlm_db_queue_msg_push((gpointer)msg);
}

void ctrlm_db_ble_global_table_name(ctrlm_network_id_t network_id, char *table) {
   errno_t safec_rc = sprintf_s(table, CONTROLLER_TABLE_NAME_MAX_LEN, CTRLM_DB_TABLE_NET_BLE_GLOBAL, network_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
}

void ctrlm_db_ble_controller_list_table_name(ctrlm_network_id_t network_id, char *table) {
   errno_t safec_rc = sprintf_s(table, CONTROLLER_TABLE_NAME_MAX_LEN, CTRLM_DB_TABLE_NET_BLE_CONTROLLER_LIST, network_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
}

void ctrlm_db_ble_controller_entry_table_name(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, char *table) {
   errno_t safec_rc = sprintf_s(table, CONTROLLER_TABLE_NAME_MAX_LEN, CTRLM_DB_TABLE_NET_BLE_CONTROLLER_ENTRY, network_id, controller_id);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
}

void ctrlm_db_ble_read_ota_failure_type_z_count(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 &value) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ble_controller_entry_table_name(network_id, controller_id, table);

   sqlite_uint64 data;
   if (0 > ctrlm_db_read_uint64(table, "ota_failure_type_z_cnt", &data)) {
      XLOGD_WARN("Failed to load OTA failure type Z count from db");
   } else {
      value = (guint8) data;
   }
}

void ctrlm_db_ble_write_ota_failure_type_z_count(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint8 value) {
   char table[CONTROLLER_TABLE_NAME_MAX_LEN];
   ctrlm_db_ble_controller_entry_table_name(network_id, controller_id, table);

   ctrlm_db_write_uint64(table, "ota_failure_type_z_cnt", (guint64) value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Voice Database
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ctrlm_db_voice_valid() {
   return(g_ctrlm_db.voice_is_valid);
}

void ctrlm_db_delete_legacy_entries(void) {
   #define VOICE_KEY_QTY (5)

   const char *voice_keys[VOICE_KEY_QTY] = { "sat_enable", "url_ptt", "url_ff", "url_mic_tap", "init_blob" };

   bool key_found = false;

   for(uint32_t index = 0; index < VOICE_KEY_QTY; index++) {
      const char *key = voice_keys[index];
      if(ctrlm_db_key_exists(CTRLM_DB_TABLE_VOICE, key)) {
         key_found = true;
         XLOGD_INFO("found key <%s>", key);

         // Delete the entry
         int rc = ctrlm_db_delete(CTRLM_DB_TABLE_VOICE, key);
         if(rc != 0) {
            XLOGD_INFO("error deleting key <%s>", key);
         }
      }
   }

   if(ctrlm_db_key_exists(CTRLM_DB_TABLE_VOICE, "query_string_ptt_count")) {
      key_found = true;
      XLOGD_INFO("found key <query_string_ptt_count>");

      const char *key_pattern = "query_string_ptt_%";
      int rc = ctrlm_db_delete(CTRLM_DB_TABLE_VOICE, key_pattern, true);
      if(rc != 0) {
         XLOGD_INFO("error deleting key pattern <%s>", key_pattern);
      }
   }
   if(key_found) { // At least one of the entries existed.  Vacuum the database to remove all remnants of the deleted entries.
      XLOGD_INFO("vacuum the database");
      if(!ctrlm_db_vacuum()) {
         XLOGD_ERROR("unable to vacuum the database");
      }
   }
}

void ctrlm_db_voice_read_guide_language(std::string &lang) {
   guchar *data = NULL;
   guint32 length = 0;
   ctrlm_db_read_blob(CTRLM_DB_TABLE_VOICE, "guide_language", &data, &length);
   if(NULL != data) {
      lang.assign((char *)data, length);
      ctrlm_db_free(data);
   } else {
      XLOGD_WARN("Failed to load guide_language from voice db");
   }
}

void ctrlm_db_voice_write_guide_language(std::string lang) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_VOICE, "guide_language", (const guchar*) lang.c_str(), lang.length());
}

void ctrlm_db_voice_read_aspect_ratio(std::string &ratio) {
   guchar *data = NULL;
   guint32 length = 0;
   ctrlm_db_read_blob(CTRLM_DB_TABLE_VOICE, "aspect_ratio", &data, &length);
   if(NULL != data) {
      ratio.assign((char *)data, length);
      ctrlm_db_free(data);
   } else {
      XLOGD_WARN("Failed to load aspect_ratio from voice db");
   }
}

void ctrlm_db_voice_write_aspect_ratio(std::string ratio) {
   ctrlm_db_write_blob(CTRLM_DB_TABLE_VOICE, "aspect_ratio", (const guchar*) ratio.c_str(), ratio.length());
}

void ctrlm_db_voice_read_utterance_duration_min(unsigned long &stream_time_min) {
   sqlite_uint64 data;
   if (0 > ctrlm_db_read_uint64(CTRLM_DB_TABLE_VOICE, "utterance_duration_min", &data)) {
      XLOGD_WARN("Failed to load utterance_duration_min from db");
   } else {
      stream_time_min = (unsigned long)data;
   }
}

void ctrlm_db_voice_write_utterance_duration_min(unsigned long stream_time_min) {
   ctrlm_db_write_uint64(CTRLM_DB_TABLE_VOICE, "utterance_duration_min", stream_time_min);
}

void ctrlm_db_voice_read_device_status(int device, int *status) {
   stringstream db_entry;
   sqlite_uint64 data;

   if(status) {
      db_entry << "device_status_" << device;
      if (0 > ctrlm_db_read_uint64(CTRLM_DB_TABLE_VOICE, db_entry.str().c_str(), &data)) {
         XLOGD_WARN("Failed to load %s from db", db_entry.str().c_str());
      } else {
         *status = (int)data;
         XLOGD_INFO("Read %s with value %d", db_entry.str().c_str(), *status);
      }
   }
}

void ctrlm_db_voice_write_device_status(int device, int status) {
   stringstream db_entry;

   db_entry << "device_status_" << device;
   ctrlm_db_write_uint64(CTRLM_DB_TABLE_VOICE, db_entry.str().c_str(), status);
}

void ctrlm_db_voice_read_audio_ducking_beep_enable(bool &enable) {
   sqlite_uint64 data;
   if (0 > ctrlm_db_read_uint64(CTRLM_DB_TABLE_VOICE, "audio_ducking_beep_enable", &data)) {
      XLOGD_WARN("Failed to load audio_ducking_beep_enable from db");
   } else {
      enable = (data ? true : false);
   }
}

void ctrlm_db_voice_write_audio_ducking_beep_enable(bool enable) {
   ctrlm_db_write_uint64(CTRLM_DB_TABLE_VOICE, "audio_ducking_beep_enable", (guint64) (enable ? 1 : 0));
}

void ctrlm_db_voice_read_par_voice_status(bool &status) {
   sqlite_uint64 data;
   if (0 > ctrlm_db_read_uint64(CTRLM_DB_TABLE_VOICE, "par_voice_status", &data)) {
      XLOGD_WARN("Failed to load par_voice_status from db");
   } else {
      status = (data ? true : false);
   }
}

void ctrlm_db_voice_write_par_voice_status(bool status) {
   ctrlm_db_write_uint64(CTRLM_DB_TABLE_VOICE, "par_voice_status", (guint64) (status ? 1 : 0));
}

void ctrlm_db_attr_write(std::weak_ptr<ctrlm_db_attr_t> attr) {
   ctrlm_db_queue_msg_write_attr_t *msg = new (std::nothrow) ctrlm_db_queue_msg_write_attr_t();
   if (msg == NULL) {
      XLOGD_ERROR("Out of memory");
      return;
   }
   msg->header.type = CTRLM_DB_QUEUE_MSG_TYPE_WRITE_ATTR;
   msg->attr.swap(attr);

   ctrlm_db_queue_msg_push((gpointer)msg);
}

bool ctrlm_db_attr_read(ctrlm_db_attr_t *attr) {
   return(attr->read_db(g_ctrlm_db.handle));
}
