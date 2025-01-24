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
#include <string>
#include <map>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "libIBus.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_config_default.h"
#include "ctrlm_utils.h"
#include "ctrlm_rcu.h"
#include "rf4ce/ctrlm_rf4ce_network.h"
#include "ctrlm_database.h"
#include "ctrlm_device_update.h"
#include "ctrlm_rfc.h"

using namespace std;

#define DEVICE_UPDATE_IMAGE_CHUNK_SIZE      (2048)
#define DEVICE_UPDATE_IMAGE_CHUNK_THRESHOLD (DEVICE_UPDATE_IMAGE_CHUNK_SIZE - 128) // Don't read next chunk until we're sure the controller rxd all the data from the chunk that will be replaced (RF4CE packet cannot be > 128 bytes so subtract this value)

#define RF4CE_SIMULTANEOUS_SESSION_QTY (2)

#if 1
#define DEVICE_UPDATE_MUTEX_LOCK()   g_rec_mutex_lock(&g_ctrlm_device_update.mutex_recursive)
#define DEVICE_UPDATE_MUTEX_UNLOCK() g_rec_mutex_unlock(&g_ctrlm_device_update.mutex_recursive)
#else
#define DEVICE_UPDATE_MUTEX_LOCK()   XLOGD_INFO("LOCK");  g_rec_mutex_lock(&g_ctrlm_device_update.mutex_recursive)
#define DEVICE_UPDATE_MUTEX_UNLOCK() XLOGD_INFO("UNLOCK");g_rec_mutex_unlock(&g_ctrlm_device_update.mutex_recursive)
//#define DEVICE_UPDATE_MUTEX_LOCK()
//#define DEVICE_UPDATE_MUTEX_UNLOCK()
#endif

#define DEVICE_UPDATE_SOFTWARE_VERSION_LENGTH   20

typedef enum {
   // Network based messages
   DEVICE_UPDATE_QUEUE_MSG_TYPE_IMAGE_STAGE     = 0,
   DEVICE_UPDATE_QUEUE_MSG_TYPE_IMAGE_READ      = 1,
   DEVICE_UPDATE_QUEUE_MSG_TYPE_IMAGE_UNSTAGE   = 2,
   DEVICE_UPDATE_QUEUE_MSG_TYPE_TERMINATE       = 3,
   DEVICE_UPDATE_QUEUE_MSG_TYPE_PROCESS_XCONF   = 4,
   CTRLM_DEVICE_UPDATE_QUEUE_MSG_TYPE_TICKLE    = CTRLM_MAIN_QUEUE_MSG_TYPE_TICKLE
} device_update_queue_msg_type_t;

typedef struct {
   device_update_queue_msg_type_t type;
} device_update_queue_msg_header_t;

typedef struct {
   device_update_queue_msg_header_t   header;
   char                               update_file_name[CTRLM_DEVICE_UPDATE_PATH_LENGTH];
} device_update_queue_msg_process_update_t;

typedef struct {
   device_update_queue_msg_header_t   header;
   guint16                            image_id;
   ctrlm_controller_id_t              controller_id;
   guint32                            session_count;
   guint32                            rf4ce_session_count;
   guint32                            reader_count;
} device_update_queue_msg_stage_t;

typedef struct {
   device_update_queue_msg_header_t   header;
   guint16                            image_id;
   ctrlm_controller_id_t              controller_id;
   gboolean                           use_offset;
   guint32                            offset;
} device_update_queue_msg_read_t;

typedef struct {
   device_update_queue_msg_header_t   header;
   guint16                            image_id;
   ctrlm_controller_id_t              controller_id;
   guint32                            session_count;
   guint32                            rf4ce_session_count;
   guint32                            reader_count;
} device_update_queue_msg_unstage_t;

typedef struct {
   ctrlm_network_id_t    network_id;
   ctrlm_controller_id_t controller_id;
   guint16               image_id;
} device_update_timeout_session_params_t;

typedef struct {
   string                                  device_name;
   ctrlm_rf4ce_device_update_image_type_t  image_type;
   string                                  file_path_archive;
   string                                  file_name_archive;
   string                                  file_name_image;
   ctrlm_rf4ce_controller_type_t           controller_type;
   guint16                                 id;
   version_software_t                      version_software;
   version_software_t                      version_bootloader_min;
   version_hardware_t                      version_hardware_min;
   guint32                                 size;
   guint32                                 crc;
   gboolean                                force_update;
   ctrlm_rf4ce_device_update_audio_theme_t audio_theme;
   guint32                                 reader_count;
   gboolean                                type_z;
} ctrlm_device_update_rf4ce_image_info_t;

typedef struct {
   ctrlm_device_update_session_id_t        session_id;
   ctrlm_device_update_image_id_t          image_id;
   string                                  device_name;
   version_software_t                      version_software;
   version_software_t                      version_bootloader;
   version_hardware_t                      version_hardware;
   gboolean                                interactive_download;
   gboolean                                interactive_load;
   gboolean                                background_download;
   gboolean                                download_initiated;
   gboolean                                download_in_progress;
   gboolean                                load_waiting;
   gboolean                                load_initiated;
   ctrlm_timestamp_t                       timestamp_to_load;
   guint32                                 time_after_inactive;
   guchar                                  percent_increment;
   guchar                                  percent_next;
   guint                                   timeout_source_id;
   device_update_timeout_session_params_t *timeout_params;
   guint32                                 timeout_value;
   FILE *                                  fd;
   char *                                  chunk_even;
   char *                                  chunk_odd;
   guint32                                 offset_even;
   guint32                                 offset_odd;
   guint32                                 bytes_read_file;
   guint32                                 bytes_read_controller;
   gboolean                                resume_pending;
} ctrlm_device_update_rf4ce_session_t;

typedef struct {
   bool     interactive;
   bool     background;
   bool     load_immediately;
   guint8   percent_increment;
   guint32  request_timeout;
} ctrlm_device_update_prefs_download_t;

typedef struct {
   bool     interactive;
   guint32  time_after_inactive;
   guint8   before_hour;
} ctrlm_device_update_prefs_load_t;

typedef struct {
   string                               server_update_path;
   string                               temp_file_path;
   vector<string>                       update_dirs;
   guint32                              check_poll_time_image;
   guint32                              check_poll_time_download;
   guint32                              check_poll_time_load;
   ctrlm_device_update_prefs_download_t download;
   ctrlm_device_update_prefs_load_t     load;
   device_update_check_locations_t      update_locations_valid;
   string                               xconf_json_file_path;
} ctrlm_device_update_prefs_t;

typedef struct {
   GThread *                                       main_thread;
   GRecMutex                                       mutex_recursive;
   sem_t                                           semaphore;
   GAsyncQueue *                                   queue;
   gboolean                                        running;

   ctrlm_device_update_prefs_t                     prefs;
   vector<ctrlm_device_update_rf4ce_image_info_t> *rf4ce_images;
   ctrlm_device_update_session_id_t                session_id;
   guint32                                         session_count;
   guint32                                         rf4ce_session_active_count;
   guint32                                         rf4ce_session_count;
   map<ctrlm_controller_id_t, ctrlm_device_update_rf4ce_session_t> rf4ce_sessions;
#ifdef XR15_704
   gboolean                                        xr15_crash_update;
#endif
   vector<rf4ce_device_update_session_resume_info_t> *sessions;
} ctrlm_device_update_t;

static ctrlm_device_update_t g_ctrlm_device_update;

static gpointer ctrlm_device_update_thread(gpointer param);
static void ctrlm_device_update_queue_msg_destroy(gpointer msg);

//static gboolean ctrlm_device_update_is_rf4ce_session_in_progress(void);
//static gboolean ctrlm_device_update_on_this_network(ctrlm_network_id_t network_id);
//static gboolean ctrlm_device_update_on_this_controller(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id);
static gboolean ctrlm_device_update_timeout_session(gpointer user_data);
static guint    ctrlm_device_update_timeout_session_create(guint timeout, gpointer param);
static void     ctrlm_device_update_timeout_session_update(guint *timeout_source_id, gint timeout, gpointer param);
static void     ctrlm_device_update_timeout_session_destroy(guint *timeout_source_id);
static gboolean ctrlm_device_update_load_config(json_t *json_obj_device_update);
static void     ctrlm_device_update_process_device_dir(const std::string &update_path, const std::string &device_name);
static void     ctrlm_device_update_process_device_file(const std::string &file_path_archive, const std::string &device_name, guint16 *image_id);
static gboolean ctrlm_device_update_image_info_get(const std::string &filename, ctrlm_device_update_rf4ce_image_info_t *image_info);
static gboolean ctrlm_device_update_rf4ce_is_software_version_not_equal(version_software_t current, version_software_t proposed);
static void     ctrlm_device_update_device_get_from_session(ctrlm_device_update_rf4ce_session_t *session, ctrlm_device_update_device_t *device);
static void     ctrlm_device_update_rfc_values_retrieved(const ctrlm_rfc_attr_t& attr);

static void     ctrlm_device_update_tmp_dir_make();
static void     ctrlm_device_update_tmp_dir_remove();
static void     ctrlm_device_update_rf4ce_tmp_dir_make();
static void     ctrlm_device_update_rf4ce_tmp_dir_remove();
static void     ctrlm_device_update_rf4ce_archive_extract(string file_path_archive, string file_name_archive);
static void     ctrlm_device_update_rf4ce_archive_remove(const std::string &file_name_archive);
static gboolean ctrlm_device_update_rf4ce_image_stage(ctrlm_controller_id_t controller_id, guint16 image_id, guint32 session_count, guint32 rf4ce_session_count, guint32 reader_count);
static gboolean ctrlm_device_update_rf4ce_image_unstage(ctrlm_controller_id_t controller_id, guint16 image_id, guint32 session_count, guint32 rf4ce_session_count, guint32 reader_count);
static gboolean ctrlm_device_update_rf4ce_image_read_next_chunk(ctrlm_controller_id_t controller_id, guint16 image_id, guint32 *offset);
static void     ctrlm_device_update_rf4ce_download_complete(ctrlm_device_update_rf4ce_session_t *session_info);
static void     ctrlm_device_update_process_dirs(void);
static void     ctrlm_device_update_rf4ce_session_resume_check(vector<rf4ce_device_update_session_resume_info_t> *sessions, bool process_local_files);

static ctrlm_device_update_session_id_t ctrlm_device_update_new_session_id();

void ctrlm_device_update_init(json_t *json_obj_device_update) {
   XLOGD_INFO("");

   // Initialize state
   g_ctrlm_device_update.running                          = false;
   g_ctrlm_device_update.rf4ce_images                     = new vector<ctrlm_device_update_rf4ce_image_info_t>();

   ctrlm_db_device_update_session_id_read(&g_ctrlm_device_update.session_id);
   // Increment by max simultaneous sessions in case of reboot before data written
   g_ctrlm_device_update.session_id                      += RF4CE_SIMULTANEOUS_SESSION_QTY;

   g_ctrlm_device_update.session_count                    = 0;
   g_ctrlm_device_update.rf4ce_session_active_count       = 0;
   g_ctrlm_device_update.rf4ce_session_count              = 0;

   g_ctrlm_device_update.prefs.server_update_path         = JSON_STR_VALUE_DEVICE_UPDATE_DIR_ROOT;
   g_ctrlm_device_update.prefs.temp_file_path             = JSON_STR_VALUE_DEVICE_UPDATE_DIR_TEMP;

   g_ctrlm_device_update.prefs.update_locations_valid     = (device_update_check_locations_t)JSON_INT_VALUE_DEVICE_UPDATE_CHECK_LOCATIONS;
   g_ctrlm_device_update.prefs.xconf_json_file_path       =   JSON_STR_VALUE_DEVICE_UPDATE_XCONF_JSON_FILE_PATH;

   g_ctrlm_device_update.prefs.check_poll_time_image      = JSON_INT_VALUE_DEVICE_UPDATE_CHECK_POLL_TIME_IMAGE;
   g_ctrlm_device_update.prefs.check_poll_time_download   = JSON_INT_VALUE_DEVICE_UPDATE_CHECK_POLL_TIME_DOWNLOAD;
   g_ctrlm_device_update.prefs.check_poll_time_load       = JSON_INT_VALUE_DEVICE_UPDATE_CHECK_POLL_TIME_LOAD;
   g_ctrlm_device_update.prefs.download.interactive       = JSON_BOOL_VALUE_DEVICE_UPDATE_INTERACTIVE_DOWNLOAD;
   g_ctrlm_device_update.prefs.download.background        = JSON_BOOL_VALUE_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_BACKGROUND_DOWNLOAD;
   g_ctrlm_device_update.prefs.download.load_immediately  = JSON_BOOL_VALUE_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_LOAD_IMMEDIATELY;
   g_ctrlm_device_update.prefs.download.percent_increment = JSON_INT_VALUE_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_PERCENT_INCREMENT;
   #ifdef CTRLM_NETWORK_RF4CE
   g_ctrlm_device_update.prefs.download.request_timeout   = JSON_INT_VALUE_NETWORK_RF4CE_DATA_REQUEST_TIMEOUT;
   #endif
   g_ctrlm_device_update.prefs.load.interactive           = JSON_BOOL_VALUE_DEVICE_UPDATE_INTERACTIVE_LOAD;
   g_ctrlm_device_update.prefs.load.time_after_inactive   = JSON_INT_VALUE_DEVICE_UPDATE_NON_INTERACTIVE_LOAD_TIME_AFTER_INACTIVE;
   g_ctrlm_device_update.prefs.load.before_hour           = JSON_INT_VALUE_DEVICE_UPDATE_NON_INTERACTIVE_LOAD_LOAD_BEFORE_HOUR;
   g_ctrlm_device_update.prefs.update_dirs.push_back(JSON_ARRAY_VAL_STR_DEVICE_UPDATE_DEVICE_UPDATE_DIRS_0);
   g_ctrlm_device_update.prefs.update_dirs.push_back(JSON_ARRAY_VAL_STR_DEVICE_UPDATE_DEVICE_UPDATE_DIRS_1);
   g_ctrlm_device_update.prefs.update_dirs.push_back(JSON_ARRAY_VAL_STR_DEVICE_UPDATE_DEVICE_UPDATE_DIRS_2);
   g_ctrlm_device_update.prefs.update_dirs.push_back(JSON_ARRAY_VAL_STR_DEVICE_UPDATE_DEVICE_UPDATE_DIRS_3);
   g_ctrlm_device_update.prefs.update_dirs.push_back(JSON_ARRAY_VAL_STR_DEVICE_UPDATE_DEVICE_UPDATE_DIRS_4);
   g_ctrlm_device_update.prefs.update_dirs.push_back(JSON_ARRAY_VAL_STR_DEVICE_UPDATE_DEVICE_UPDATE_DIRS_5);
  
#ifdef XR15_704
   g_ctrlm_device_update.xr15_crash_update                = false;
#endif
   g_ctrlm_device_update.sessions = NULL;

   // Create an asynchronous queue to receive incoming messages from the networks
   g_ctrlm_device_update.queue = g_async_queue_new_full(ctrlm_device_update_queue_msg_destroy);

   // Initialize semaphore and mutex
   g_rec_mutex_init(&g_ctrlm_device_update.mutex_recursive);
   sem_init(&g_ctrlm_device_update.semaphore, 0, 0);

   XLOGD_INFO("Waiting for device update thread initialization...");
   g_ctrlm_device_update.main_thread = g_thread_new("ctrlm_device_update", ctrlm_device_update_thread, NULL);

   // Block until initialization is complete or a timeout occurs
   sem_wait(&g_ctrlm_device_update.semaphore);

   ctrlm_device_update_load_config(json_obj_device_update);

   if(g_ctrlm_device_update.prefs.update_locations_valid   ==DEVICE_UPDATE_CHECK_FILESYSTEM ||
         g_ctrlm_device_update.prefs.update_locations_valid==DEVICE_UPDATE_CHECK_BOTH  )
   {
      ctrlm_device_update_process_dirs();
   }

   // Delete temp dir again
   ctrlm_device_update_tmp_dir_remove();

   g_ctrlm_device_update.running = true;

   ctrlm_device_update_init_iarm();

   // set up rfc handler
   ctrlm_rfc_t *rfc = ctrlm_rfc_t::get_instance();
   if(rfc) {
      rfc->add_changed_listener(ctrlm_rfc_t::attrs::DEVICE_UPDATE, &ctrlm_device_update_rfc_values_retrieved);
   }

}

device_update_check_locations_t ctrlm_device_update_check_locations_get()
{
   return g_ctrlm_device_update.prefs.update_locations_valid;
}

const char * ctrlm_device_update_get_xconf_path(){
   return g_ctrlm_device_update.prefs.xconf_json_file_path.c_str();
}

void ctrlm_device_update_terminate(void) {
   if(g_ctrlm_device_update.main_thread != NULL) {
      device_update_queue_msg_header_t *msg = (device_update_queue_msg_header_t *)g_malloc(sizeof(device_update_queue_msg_header_t));
      if(msg == NULL) {
         XLOGD_ERROR("Out of memory");
      } else {
         struct timespec end_time;
         clock_gettime(CLOCK_REALTIME, &end_time);
         end_time.tv_sec += 5;
         msg->type = DEVICE_UPDATE_QUEUE_MSG_TYPE_TERMINATE;

         XLOGD_INFO("Waiting for device update thread termination...");
         ctrlm_device_update_queue_msg_push((gpointer)msg);

         // Block until termination is acknowledged or a timeout occurs
         int acknowledged = sem_timedwait(&g_ctrlm_device_update.semaphore, &end_time); 

         if(acknowledged==-1) { // no response received
            XLOGD_INFO("Do NOT wait for thread to exit");
         } else {
            // Wait for thread to exit
            XLOGD_INFO("Waiting for thread to exit");
            g_thread_join(g_ctrlm_device_update.main_thread);
            XLOGD_INFO("thread exited.");
         }
      }
   }
   sem_destroy(&g_ctrlm_device_update.semaphore);
   if(g_ctrlm_device_update.sessions != NULL) {
      delete g_ctrlm_device_update.sessions;
      g_ctrlm_device_update.sessions = NULL;
   }
}

// Add a message to the control manager's processing queue
void ctrlm_device_update_queue_msg_push(gpointer msg) {
   g_async_queue_push(g_ctrlm_device_update.queue, msg);
}

void ctrlm_device_update_queue_msg_destroy(gpointer msg) {
   if(msg) {
      XLOGD_DEBUG("Free %p", msg);
      g_free(msg);
   }
}

gboolean ctrlm_device_update_is_image_id_valid(ctrlm_device_update_image_id_t image_id) {
   if(image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
      return FALSE;
   }
   return TRUE;
}

gboolean ctrlm_device_update_load_config(json_t *json_obj_device_update) {
   XLOGD_INFO("");

   DEVICE_UPDATE_MUTEX_LOCK();
   if(json_obj_device_update == NULL || !json_is_object(json_obj_device_update)) {
      XLOGD_INFO("use default configuration");
   } else {
      json_t *json_obj = json_object_get(json_obj_device_update, JSON_STR_NAME_DEVICE_UPDATE_DIR_ROOT);
      const char *text = "Server Update Path";
      if(json_obj != NULL && json_is_string(json_obj)) {
         XLOGD_INFO("%-28s - PRESENT <%s>", text, json_string_value(json_obj));
         g_ctrlm_device_update.prefs.server_update_path = json_string_value(json_obj);
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }
      json_obj = json_object_get(json_obj_device_update, JSON_STR_NAME_DEVICE_UPDATE_DIR_TEMP);
      text     = "Temp File Path";
      if(json_obj != NULL && json_is_string(json_obj)) {
         XLOGD_INFO("%-28s - PRESENT <%s>", text, json_string_value(json_obj));
         g_ctrlm_device_update.prefs.temp_file_path = json_string_value(json_obj);
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }


      json_obj = json_object_get(json_obj_device_update, JSON_INT_NAME_DEVICE_UPDATE_CHECK_LOCATIONS);
      text     = "update locations";
      if(json_obj != NULL && json_is_integer(json_obj)) {
         json_int_t update_locations = json_integer_value(json_obj);
         XLOGD_INFO("%-28s - PRESENT <%lld>", text, update_locations);
         g_ctrlm_device_update.prefs.update_locations_valid = (device_update_check_locations_t)update_locations;
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }

      json_obj = json_object_get(json_obj_device_update, JSON_STR_NAME_DEVICE_UPDATE_XCONF_JSON_FILE_PATH);
      text     = "Xconf File Path";
      if(json_obj != NULL && json_is_string(json_obj)) {
         XLOGD_INFO("%-28s - PRESENT <%s>", text, json_string_value(json_obj));
         g_ctrlm_device_update.prefs.xconf_json_file_path = json_string_value(json_obj);
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }



      json_obj = json_object_get(json_obj_device_update, JSON_INT_NAME_DEVICE_UPDATE_CHECK_POLL_TIME_IMAGE);
      text     = "Check Poll Time - Image";
      if(json_obj != NULL && json_is_integer(json_obj)) {
         json_int_t poll_time = json_integer_value(json_obj);
         XLOGD_INFO("%-28s - PRESENT <%lld>", text, poll_time);
         if(poll_time <= 0 || poll_time > 24 * 60 * 60) {
            XLOGD_INFO("%-28s - OUT OF RANGE %lld", text, poll_time);
         } else {
            g_ctrlm_device_update.prefs.check_poll_time_image = poll_time;
         }
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }
      json_obj = json_object_get(json_obj_device_update, JSON_INT_NAME_DEVICE_UPDATE_CHECK_POLL_TIME_DOWNLOAD);
      text     = "Check Poll Time - Download";
      if(json_obj != NULL && json_is_integer(json_obj)) {
         json_int_t poll_time = json_integer_value(json_obj);
         XLOGD_INFO("%-28s - PRESENT <%lld>", text, poll_time);
         if(poll_time <= 0 || poll_time > 30 * 60) {
            XLOGD_INFO("%-28s - OUT OF RANGE %lld", text, poll_time);
         } else {
            g_ctrlm_device_update.prefs.check_poll_time_download = poll_time;
         }
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }
      json_obj = json_object_get(json_obj_device_update, JSON_INT_NAME_DEVICE_UPDATE_CHECK_POLL_TIME_LOAD);
      text     = "Check Poll Time - Load";
      if(json_obj != NULL && json_is_integer(json_obj)) {
         json_int_t poll_time = json_integer_value(json_obj);
         XLOGD_INFO("%-28s - PRESENT <%lld>", text, poll_time);
         if(poll_time <= 0 || poll_time > 30 * 60) {
            XLOGD_INFO("%-28s - OUT OF RANGE %lld", text, poll_time);
         } else {
            g_ctrlm_device_update.prefs.check_poll_time_load = poll_time;
         }
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }
      json_obj = json_object_get(json_obj_device_update, JSON_BOOL_NAME_DEVICE_UPDATE_INTERACTIVE_DOWNLOAD);
      text     = "Download - Interactive";
      if(json_obj != NULL && json_is_boolean(json_obj)) {
         XLOGD_INFO("%-28s - PRESENT <%s>", text, json_is_true(json_obj) ? "true" : "false");
         if(json_is_true(json_obj)) {
            g_ctrlm_device_update.prefs.download.interactive = true;
         } else {
            g_ctrlm_device_update.prefs.download.interactive = false;
         }
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }
      json_obj = json_object_get(json_obj_device_update, JSON_BOOL_NAME_DEVICE_UPDATE_INTERACTIVE_LOAD);
      text     = "Load - Interactive";
      if(json_obj != NULL && json_is_boolean(json_obj)) {
         XLOGD_INFO("%-28s - PRESENT <%s>", text, json_is_true(json_obj) ? "true" : "false");
         if(json_is_true(json_obj)) {
            g_ctrlm_device_update.prefs.load.interactive = true;
         } else {
            g_ctrlm_device_update.prefs.load.interactive = false;
         }
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }

      #ifdef CTRLM_NETWORK_RF4CE
      json_obj = json_object_get(json_obj_device_update, JSON_INT_NAME_NETWORK_RF4CE_DATA_REQUEST_TIMEOUT);
      text     = "Data request timeout";
      if(json_obj != NULL && json_is_integer(json_obj)) {
         json_int_t req_timeout = json_integer_value(json_obj);
         XLOGD_INFO("%-28s - PRESENT <%lld>", text, req_timeout);
         g_ctrlm_device_update.prefs.download.request_timeout = req_timeout;
      } else {
         XLOGD_INFO("%-28s - ABSENT", text);
      }
      #endif

      json_t *json_obj_non_interactive = json_object_get(json_obj_device_update, JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE);
      if(json_obj_non_interactive == NULL || !json_is_object(json_obj_non_interactive)) {
         XLOGD_INFO("%s object not found", JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE);
      } else {
         json_t *json_obj_download = json_object_get(json_obj_non_interactive, JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD);
         if(json_obj_download == NULL || !json_is_object(json_obj_download)) {
            XLOGD_INFO("%s object not found", JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD);
         } else {
            json_obj = json_object_get(json_obj_download, JSON_BOOL_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_BACKGROUND_DOWNLOAD);
            text     = "Download - Background";
            if(json_obj != NULL && json_is_boolean(json_obj)) {
               XLOGD_INFO("%-28s - PRESENT <%s>", text, json_is_true(json_obj) ? "true" : "false");
               if(json_is_true(json_obj)) {
                  g_ctrlm_device_update.prefs.download.background = true;
               } else {
                  g_ctrlm_device_update.prefs.download.background = false;
               }
            } else {
               XLOGD_INFO("%-28s - ABSENT", text);
            }
            json_obj = json_object_get(json_obj_download, JSON_BOOL_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_LOAD_IMMEDIATELY);
            text     = "Download - Load Immediately";
            if(json_obj != NULL && json_is_boolean(json_obj)) {
               XLOGD_INFO("%-28s - PRESENT <%s>", text, json_is_true(json_obj) ? "true" : "false");
               if(json_is_true(json_obj)) {
                  g_ctrlm_device_update.prefs.download.load_immediately = true;
               } else {
                  g_ctrlm_device_update.prefs.download.load_immediately = false;
               }
            } else {
               XLOGD_INFO("%-28s - ABSENT", text);
            }
            json_obj = json_object_get(json_obj_download, JSON_INT_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_PERCENT_INCREMENT);
            text     = "Download - Percent Inc";
            if(json_obj != NULL && json_is_integer(json_obj)) {
               json_int_t increment = json_integer_value(json_obj);
               XLOGD_INFO("%-28s - PRESENT <%lld>", text, increment);
               if(increment <= 0 || increment > 50) {
                  XLOGD_INFO("%-28s - OUT OF RANGE %lld", text, increment);
               } else {
                  g_ctrlm_device_update.prefs.download.percent_increment = increment;
               }
            } else {
               XLOGD_INFO("%-28s - ABSENT", text);
            }
         }
         json_t *json_obj_load = json_object_get(json_obj_non_interactive, JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE_LOAD);
         if(json_obj_load == NULL || !json_is_object(json_obj_load)) {
            XLOGD_INFO("%s object not found", JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE_LOAD);
         } else {
            json_obj = json_object_get(json_obj_load, JSON_INT_NAME_DEVICE_UPDATE_NON_INTERACTIVE_LOAD_TIME_AFTER_INACTIVE);
            text     = "Load - Time After Inactive";
            if(json_obj != NULL && json_is_integer(json_obj)) {
               json_int_t time = json_integer_value(json_obj);
               XLOGD_INFO("%-28s - PRESENT <%lld>", text, time);
               if(time < 0) {
                  XLOGD_INFO("%-28s - OUT OF RANGE %lld", text, time);
               } else {
                  g_ctrlm_device_update.prefs.load.time_after_inactive = time;
               }
            } else {
               XLOGD_INFO("%-28s - ABSENT", text);
            }

            json_obj = json_object_get(json_obj_load, JSON_INT_NAME_DEVICE_UPDATE_NON_INTERACTIVE_LOAD_LOAD_BEFORE_HOUR);
            text     = "Load - Before Hour";
            if(json_obj != NULL && json_is_integer(json_obj)) {
               json_int_t hour = json_integer_value(json_obj);
               XLOGD_INFO("%-28s - PRESENT <%lld>", text, hour);
               if(hour < 2 || hour >= 24) {
                  XLOGD_INFO("%-28s - OUT OF RANGE %lld", text, hour);
               } else {
                  g_ctrlm_device_update.prefs.load.before_hour = hour;
               }
            } else {
               XLOGD_INFO("%-28s - ABSENT", text);
            }
         }
      }

      json_obj = json_object_get(json_obj_device_update, JSON_ARRAY_NAME_DEVICE_UPDATE_DEVICE_UPDATE_DIRS);
      text     = "Device Update Dirs";
      if(json_obj == NULL || !json_is_array(json_obj)) {
         XLOGD_ERROR("%-28s - ABSENT!", text);
      } else {
         size_t  index;
         size_t array_size = json_array_size(json_obj);
         if(array_size > 0) {
            g_ctrlm_device_update.prefs.update_dirs.clear();
         }

         for(index = 0; index < array_size; index++) {
            json_t *dir_name = json_array_get(json_obj, index);
            if(dir_name == NULL || !json_is_string(dir_name)) {
               XLOGD_WARN("Ignoring invalid directory entry!");
            } else {
               g_ctrlm_device_update.prefs.update_dirs.push_back(json_string_value(dir_name));
               XLOGD_INFO("%-28s - PRESENT <%s>", text, json_string_value(dir_name));
            }
         }
      }
   }

   DEVICE_UPDATE_MUTEX_UNLOCK();

   XLOGD_INFO("Server Update Path           <%s>",    g_ctrlm_device_update.prefs.server_update_path.c_str());
   XLOGD_INFO("Temp File Path               <%s>",    g_ctrlm_device_update.prefs.temp_file_path.c_str());
   XLOGD_INFO("Check Poll Time - Image      %d secs", g_ctrlm_device_update.prefs.check_poll_time_image);
   XLOGD_INFO("Check Poll Time - Download   %d secs", g_ctrlm_device_update.prefs.check_poll_time_download);
   XLOGD_INFO("Check Poll Time - Load       %d secs", g_ctrlm_device_update.prefs.check_poll_time_load);
   XLOGD_INFO("Download - Interactive       <%s>",    g_ctrlm_device_update.prefs.download.interactive ? "true" : "false");
   XLOGD_INFO("Download - Background        <%s>",    g_ctrlm_device_update.prefs.download.background ? "true" : "false");
   XLOGD_INFO("Download - Load Immediately  <%s>",    g_ctrlm_device_update.prefs.download.load_immediately ? "true" : "false");
   XLOGD_INFO("Download - Percent Increment %d",      g_ctrlm_device_update.prefs.download.percent_increment);
   XLOGD_INFO("Load - Interactive           <%s>",    g_ctrlm_device_update.prefs.load.interactive ? "true" : "false");
   XLOGD_INFO("Load - Time After Inactive   %d",      g_ctrlm_device_update.prefs.load.time_after_inactive);
   XLOGD_INFO("Load - Before Hour           %d",      g_ctrlm_device_update.prefs.load.before_hour);
   XLOGD_INFO("Xconf file path              %s",      g_ctrlm_device_update.prefs.xconf_json_file_path.c_str());
   XLOGD_INFO("Xconf location checks        %d",      g_ctrlm_device_update.prefs.update_locations_valid);
   for(vector<string>::iterator it = g_ctrlm_device_update.prefs.update_dirs.begin(); it != g_ctrlm_device_update.prefs.update_dirs.end(); it++) {
      XLOGD_INFO("Device Update Dirs           <%s>", it->c_str());
   }

   return true;
}

#define NONINTERACTIVE_DOWNLOAD_PATH JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE JSON_PATH_SEPERATOR JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD JSON_PATH_SEPERATOR
#define NONINTERACTIVE_LOAD_PATH JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE JSON_PATH_SEPERATOR JSON_OBJ_NAME_DEVICE_UPDATE_NON_INTERACTIVE_LOAD JSON_PATH_SEPERATOR
void ctrlm_device_update_rfc_values_retrieved(const ctrlm_rfc_attr_t& attr) {
   if(attr.get_rfc_value(JSON_STR_NAME_DEVICE_UPDATE_DIR_ROOT, g_ctrlm_device_update.prefs.server_update_path)){
      if(attr.get_rfc_value(JSON_INT_NAME_DEVICE_UPDATE_CHECK_LOCATIONS, g_ctrlm_device_update.prefs.update_locations_valid)) {
         if(g_ctrlm_device_update.prefs.update_locations_valid == DEVICE_UPDATE_CHECK_FILESYSTEM ||
            g_ctrlm_device_update.prefs.update_locations_valid == DEVICE_UPDATE_CHECK_BOTH) {
               // TODO check location
         }
      }
   }
   attr.get_rfc_value(JSON_STR_NAME_DEVICE_UPDATE_DIR_TEMP, g_ctrlm_device_update.prefs.temp_file_path);
   if(attr.get_rfc_value(JSON_STR_NAME_DEVICE_UPDATE_XCONF_JSON_FILE_PATH, g_ctrlm_device_update.prefs.xconf_json_file_path)) {
      // TODO export file again
   }
   attr.get_rfc_value(JSON_INT_NAME_DEVICE_UPDATE_CHECK_POLL_TIME_IMAGE, g_ctrlm_device_update.prefs.check_poll_time_image, 1, 24*60*60);
   attr.get_rfc_value(JSON_INT_NAME_DEVICE_UPDATE_CHECK_POLL_TIME_DOWNLOAD, g_ctrlm_device_update.prefs.check_poll_time_download, 1, 30*60);
   attr.get_rfc_value(JSON_INT_NAME_DEVICE_UPDATE_CHECK_POLL_TIME_LOAD, g_ctrlm_device_update.prefs.check_poll_time_load, 1, 30*60);
   attr.get_rfc_value(JSON_BOOL_NAME_DEVICE_UPDATE_INTERACTIVE_DOWNLOAD, g_ctrlm_device_update.prefs.download.interactive);
   attr.get_rfc_value(JSON_BOOL_NAME_DEVICE_UPDATE_INTERACTIVE_LOAD, g_ctrlm_device_update.prefs.load.interactive);
   #ifdef CTRLM_NETWORK_RF4CE
   attr.get_rfc_value(JSON_INT_NAME_NETWORK_RF4CE_DATA_REQUEST_TIMEOUT, g_ctrlm_device_update.prefs.download.request_timeout, 0);
   #endif

   attr.get_rfc_value(NONINTERACTIVE_DOWNLOAD_PATH JSON_BOOL_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_BACKGROUND_DOWNLOAD, g_ctrlm_device_update.prefs.download.background);
   attr.get_rfc_value(NONINTERACTIVE_DOWNLOAD_PATH JSON_BOOL_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_LOAD_IMMEDIATELY, g_ctrlm_device_update.prefs.download.load_immediately);
   attr.get_rfc_value(NONINTERACTIVE_DOWNLOAD_PATH JSON_INT_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_PERCENT_INCREMENT, g_ctrlm_device_update.prefs.download.percent_increment, 1, 50);
   attr.get_rfc_value(NONINTERACTIVE_DOWNLOAD_PATH JSON_BOOL_NAME_DEVICE_UPDATE_NON_INTERACTIVE_DOWNLOAD_BACKGROUND_DOWNLOAD, g_ctrlm_device_update.prefs.download.background);

   attr.get_rfc_value(NONINTERACTIVE_LOAD_PATH JSON_INT_NAME_DEVICE_UPDATE_NON_INTERACTIVE_LOAD_TIME_AFTER_INACTIVE, g_ctrlm_device_update.prefs.load.time_after_inactive, 0);
   attr.get_rfc_value(NONINTERACTIVE_LOAD_PATH JSON_INT_NAME_DEVICE_UPDATE_NON_INTERACTIVE_LOAD_LOAD_BEFORE_HOUR, g_ctrlm_device_update.prefs.load.before_hour, 2, 23);

   attr.get_rfc_value(JSON_ARRAY_NAME_DEVICE_UPDATE_DEVICE_UPDATE_DIRS, g_ctrlm_device_update.prefs.update_dirs);


   //    json_obj = json_object_get(json_obj_device_update, JSON_ARRAY_NAME_DEVICE_UPDATE_DEVICE_UPDATE_DIRS);
   //    text     = "Device Update Dirs";
   //    if(json_obj == NULL || !json_is_array(json_obj)) {
   //       XLOGD_ERROR("%-28s - ABSENT!", text);
   //    } else {
   //       size_t  index;
   //       size_t array_size = json_array_size(json_obj);
   //       if(array_size > 0) {
   //          g_ctrlm_device_update.prefs.update_dirs.clear();
   //       }

   //       for(index = 0; index < array_size; index++) {
   //          json_t *dir_name = json_array_get(json_obj, index);
   //          if(dir_name == NULL || !json_is_string(dir_name)) {
   //             XLOGD_WARN("Ignoring invalid directory entry!");
   //          } else {
   //             g_ctrlm_device_update.prefs.update_dirs.push_back(json_string_value(dir_name));
   //             XLOGD_INFO("%-28s - PRESENT <%s>", text, json_string_value(dir_name));
   //          }
   //       }
   //    }
   // }
}

void ctrlm_device_update_process_dirs(void) {
   // Delete temp dir and create it again so we are always starting fresh
   ctrlm_device_update_tmp_dir_remove();
   ctrlm_device_update_tmp_dir_make();

   for(vector<string>::iterator it = g_ctrlm_device_update.prefs.update_dirs.begin(); it != g_ctrlm_device_update.prefs.update_dirs.end(); it++) {
      string update_path = g_ctrlm_device_update.prefs.server_update_path + *it;

      if(update_path.length() <= 0 || !g_file_test(update_path.c_str(), G_FILE_TEST_IS_DIR)) {
         XLOGD_ERROR("Dir not found <%s>", update_path.c_str());
      } else {
         XLOGD_INFO("Processing dir <%s>", update_path.c_str());
         ctrlm_device_update_process_device_dir(update_path, *it);
      }
   }
}

void ctrlm_device_update_process_device_dir(const std::string &update_path, const std::string &device_name) {
   GDir *  gdir  = NULL;
   GError *error = NULL;
   XLOGD_INFO("<%s> <%s>", device_name.c_str(), update_path.c_str());

   gdir = g_dir_open(update_path.c_str(), 0, &error);
   if(gdir == NULL) {
      if(error) {
         XLOGD_ERROR("Dir Open error code %d <%s>", error->code, error->message ? error->message : "");
         g_error_free(error);
      } else {
         XLOGD_ERROR("Dir Open error");
      }
      return;
   }

   // Create the temp dir
   ctrlm_device_update_rf4ce_tmp_dir_make();

   // loop through dir listing and get normal files
   const gchar *dir_entry;

   while((dir_entry = g_dir_read_name(gdir)) != NULL) {
      string filename   = dir_entry;
      string path_entry = update_path + "/" + filename;
      XLOGD_INFO("Entry <%s>", dir_entry);

      if(!g_file_test(path_entry.c_str(), G_FILE_TEST_IS_REGULAR)) { // Only process regular files
         XLOGD_INFO("Entry <%s> is not a regular file", dir_entry);
      } else {
         // check for it being either a tgz or tar.gz file
         size_t idx = filename.rfind('.');
         if(idx != string::npos) {
            string ext = filename.substr(idx + 1);
            if(ext == "gz") {
               idx = filename.rfind('.', idx);
               if(idx == string::npos) {
                  continue;
               }
               string ext = filename.substr(idx + 1);
               if(ext == "tar.gz") {
                  ctrlm_device_update_process_device_file(path_entry, device_name, NULL);
               }
            } else if(ext == "tgz") {
               ctrlm_device_update_process_device_file(path_entry, device_name, NULL);
            }
         }
      }
   }
   g_dir_close(gdir);

   // resize the images vector
   g_ctrlm_device_update.rf4ce_images->resize(g_ctrlm_device_update.rf4ce_images->size());

   // Clean up the temp dir
   ctrlm_device_update_rf4ce_tmp_dir_remove();
}

void ctrlm_device_update_process_device_file(const std::string &file_path_archive, const std::string &device_name, guint16 *image_id) {
   GDir *  gdir  = NULL;
   GError *error = NULL;
   XLOGD_INFO("<%s> <%s>", device_name.c_str(), file_path_archive.c_str());

   size_t idx = file_path_archive.rfind('/');
   string file_name_archive = file_path_archive.substr(idx + 1);

   // lets make sure this file exists before we try to process it.
   if(ctrlm_file_exists(file_path_archive.c_str())==false){
      XLOGD_ERROR("incoming file does not exist ");
      return;
   }

   ctrlm_device_update_rf4ce_archive_extract(file_path_archive, file_name_archive);

   string dir = g_ctrlm_device_update.prefs.temp_file_path + "rf4ce/" + file_name_archive + "/";

   gdir = g_dir_open(dir.c_str(), 0, &error);
   if(gdir == NULL) {
      if(error) {
         XLOGD_ERROR("Dir open error code %d <%s>", error->code, error->message ? error->message : "");
         g_error_free(error);
      } else {
         XLOGD_ERROR("Dir open error");
      }
      ctrlm_device_update_rf4ce_archive_remove(file_name_archive);
      return;
   }

   // loop through dir listing to find the xml file
   const gchar *dir_entry;

   while((dir_entry = g_dir_read_name(gdir)) != NULL) {
      string filename = dir_entry;
      //XLOGD_INFO("dumMgr:found file:%s", filename.c_str());

      size_t idx = filename.rfind('.');
      if(idx == string::npos) {
         continue;
      }
      string ext = filename.substr(idx + 1);
      //XLOGD_INFO("extension:%s", ext.c_str());

      if(ext == "xml") {
         ctrlm_device_update_rf4ce_image_info_t image_info;
         image_info.file_path_archive  = file_path_archive;
         image_info.file_name_archive  = file_name_archive;

         string file_name_xml = g_ctrlm_device_update.prefs.temp_file_path + "rf4ce/" + file_name_archive + "/" + filename;

         if(!ctrlm_device_update_image_info_get(file_name_xml, &image_info)) {
            XLOGD_INFO("unable to get image info <%s>", dir_entry);
         } else {
            ctrlm_rf4ce_controller_type_t controller_type;
            if(image_info.device_name == "XR11-20") {
               controller_type = RF4CE_CONTROLLER_TYPE_XR11;
            } else if(image_info.device_name == "XR15-10") {
               controller_type = RF4CE_CONTROLLER_TYPE_XR15;
            } else if(image_info.device_name == "XR15-20" || image_info.device_name == "XR15-20Z") {
               controller_type = RF4CE_CONTROLLER_TYPE_XR15V2;
            } else if(image_info.device_name == "XR16-10" || image_info.device_name == "XR16-10Z") {
               controller_type = RF4CE_CONTROLLER_TYPE_XR16;
            } else if(image_info.device_name == "XR19-10") {
               controller_type = RF4CE_CONTROLLER_TYPE_XR19;
            } else if(image_info.device_name == "XRA-10") {
               controller_type = RF4CE_CONTROLLER_TYPE_XRA;
            }  else {
               XLOGD_ERROR("Unsupported device <%s>", device_name.c_str());
               break;
            }
            image_info.controller_type = controller_type;
            image_info.reader_count    = 0;

            //XLOGD_INFO("sending announce event");
            //IARM_Result_t retval = IARM_Bus_BroadcastEvent(IARM_BUS_DEVICE_UPDATE_NAME, (IARM_EventId_t) IARM_BUS_DEVICE_UPDATE_EVENT_ANNOUNCE, (void *) &myData, sizeof(_IARM_Bus_DeviceUpdate_Announce_t));
            //if(retval == IARM_RESULT_SUCCESS) {
            //   //XLOGD_INFO("Announce Event sent successfully");
            //} else {
            //   XLOGD_INFO("Announce Event problem, %i", retval);
            //}

            XLOGD_INFO("Storing image info");

            if(image_id == NULL) {
               image_info.id = g_ctrlm_device_update.rf4ce_images->size();
               g_ctrlm_device_update.rf4ce_images->insert(g_ctrlm_device_update.rf4ce_images->end(), image_info);
            } else { // use specified image id
               image_info.id = *image_id;

               if(image_info.id >= g_ctrlm_device_update.rf4ce_images->size()) {
                  g_ctrlm_device_update.rf4ce_images->resize(image_info.id + 1);
               }

               g_ctrlm_device_update.rf4ce_images->at(image_info.id) = image_info;
            }

            // HACK FOR XR15-704
#ifdef XR15_704
            version_software_t version_bug = {XR15_DEVICE_UPDATE_BUG_FIRMWARE_MAJOR, XR15_DEVICE_UPDATE_BUG_FIRMWARE_MINOR, XR15_DEVICE_UPDATE_BUG_FIRMWARE_REVISION, XR15_DEVICE_UPDATE_BUG_FIRMWARE_PATCH};
            if(RF4CE_CONTROLLER_TYPE_XR15 == controller_type && ctrlm_device_update_rf4ce_is_software_version_min_met(image_info.version_software, version_bug)) {
               XLOGD_INFO("XR15v1 image >= 2.0.0.0 available, enabling crash code for XR15v1s running < 2.0.0.0");
               g_ctrlm_device_update.xr15_crash_update = true;
            }
#endif
            // HACK FOR XR15-704
#ifdef CTRLM_NETWORK_RF4CE
            // Firmware Notify message
            errno_t safec_rc = -1;
            ctrlm_main_queue_msg_notify_firmware_t *msg = (ctrlm_main_queue_msg_notify_firmware_t *)g_malloc(sizeof(ctrlm_main_queue_msg_notify_firmware_t));
            if(NULL == msg) {
               XLOGD_ERROR("Out of memory");
               g_assert(0);
            }
            else {
               msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_NOTIFY_FIRMWARE;
               msg->image_type        = image_info.image_type;
               msg->controller_type   = controller_type;
               msg->force_update      = image_info.force_update;
               msg->type_z            = image_info.type_z;
               safec_rc = memcpy_s(&msg->version_software, sizeof(msg->version_software), &image_info.version_software, sizeof(version_software_t));
               ERR_CHK(safec_rc);
               safec_rc = memcpy_s(&msg->version_bootloader_min, sizeof(msg->version_bootloader_min), &image_info.version_bootloader_min, sizeof(version_software_t));
               ERR_CHK(safec_rc);
               safec_rc = memcpy_s(&msg->version_hardware_min, sizeof(msg->version_hardware_min), &image_info.version_hardware_min, sizeof(version_hardware_t));
               ERR_CHK(safec_rc);
               ctrlm_main_queue_msg_push(msg);
            }  //CID:113223 - Forward null
#endif
         }
      }
   }

   ctrlm_device_update_rf4ce_archive_remove(file_name_archive);
   g_dir_close(gdir);
}

gboolean ctrlm_device_update_image_info_get(const std::string &filename, ctrlm_device_update_rf4ce_image_info_t *image_info) {
   gchar *contents = NULL;
   string xml;

   if(image_info == NULL) {
      return false;
   }

   XLOGD_INFO("File <%s>", filename.c_str());

   if(!g_file_test(filename.c_str(), G_FILE_TEST_EXISTS) || !g_file_get_contents(filename.c_str(), &contents, NULL, NULL)) {
      XLOGD_INFO("unable to get file contents <%s>", filename.c_str());
      return false;
   }
   xml = contents;
   g_free(contents);

   string version_string = ctrlm_xml_tag_text_get(xml, "image:softwareVersion");
   if(version_string.length() == 0) {
      XLOGD_ERROR("Missing Software Version");
      return false;
   }
   unsigned int ver[4];
      if(4 != sscanf(version_string.c_str(), "%u.%u.%u.%u", &ver[0], &ver[1], &ver[2], &ver[3])) {
         XLOGD_ERROR("Error converting software version");
         return false;
      } else if(ver[0] > 255 || ver[1] > 255 || ver[2] > 255 || ver[3] > 255) {
         XLOGD_ERROR("Error software version out of range");
         return false;
      }
      image_info->version_software.major    = (guchar)ver[0];
      image_info->version_software.minor    = (guchar)ver[1];
      image_info->version_software.revision = (guchar)ver[2];
      image_info->version_software.patch    = (guchar)ver[3];

   version_string = ctrlm_xml_tag_text_get(xml, "image:bootLoaderVersionMin");
   if(version_string.length() == 0) {
      XLOGD_ERROR("Missing Bootloader Version Min");
      return false;
   }

   if(4 != sscanf(version_string.c_str(), "%u.%u.%u.%u", &ver[0], &ver[1], &ver[2], &ver[3])) {
      XLOGD_ERROR("Error converting bootloader version min");
      return false;
   } else if(ver[0] > 255 || ver[1] > 255 || ver[2] > 255 || ver[3] > 255) {
      XLOGD_ERROR("Error Bootloader version min out of range");
      return false;
   }
   image_info->version_bootloader_min.major    = (guchar)ver[0];
   image_info->version_bootloader_min.minor    = (guchar)ver[1];
   image_info->version_bootloader_min.revision = (guchar)ver[2];
   image_info->version_bootloader_min.patch    = (guchar)ver[3];

   version_string = ctrlm_xml_tag_text_get(xml, "image:hardwareManufacturer");
   if(version_string.length() == 0) {
      XLOGD_ERROR("Missing Hardware Manufacturer");
      return false;
   }

   if(1 != sscanf(version_string.c_str(), "%u", &ver[0])) {
      XLOGD_ERROR("Error converting hardware manufacturer");
      return false;
   } else if(ver[0] > 15) {
      XLOGD_ERROR("Error Hardware manufacturer out of range");
      return false;
   }
   image_info->version_hardware_min.manufacturer       = (guchar)ver[0];

   version_string = ctrlm_xml_tag_text_get(xml, "image:hardwareVersionMin");
   if(version_string.length() == 0) {
      XLOGD_ERROR("Missing Hardware Version Min");
      return false;
   }

   if(2 != sscanf(version_string.c_str(), "%u.%u", &ver[0], &ver[1])) {
      XLOGD_ERROR("Error converting hardware version min");
      return false;
   } else if(ver[0] > 15 || ver[1] > 255) {
      XLOGD_ERROR("Error hardware version min out of range");
      return false;
   }
   image_info->version_hardware_min.model       = (guchar)ver[0];
   image_info->version_hardware_min.hw_revision = (guchar)ver[1];
   image_info->version_hardware_min.lot_code    = 0;

   string type = ctrlm_xml_tag_text_get(xml, "image:type");
   if(type.length() == 0) {
      XLOGD_ERROR("Missing Type");
      return false;
   }
   image_info->image_type = (ctrlm_rf4ce_device_update_image_type_t)atol(type.c_str());
   if(image_info->image_type != RF4CE_DEVICE_UPDATE_IMAGE_TYPE_FIRMWARE && image_info->image_type != RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_1
      && image_info->image_type != RF4CE_DEVICE_UPDATE_IMAGE_TYPE_DSP && image_info->image_type != RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_2
      && image_info->image_type != RF4CE_DEVICE_UPDATE_IMAGE_TYPE_KEYWORD_MODEL) {
      XLOGD_ERROR("Unsupported Type %u", image_info->image_type);
      return false;
   }
   if(image_info->image_type == RF4CE_DEVICE_UPDATE_IMAGE_TYPE_FIRMWARE) {
      image_info->audio_theme = RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID;
   } else {
      string audio_theme = ctrlm_xml_tag_text_get(xml, "image:audio_theme");
      if(audio_theme.length() == 0) {
         image_info->audio_theme = RF4CE_DEVICE_UPDATE_AUDIO_THEME_DEFAULT;
      } else {
         image_info->audio_theme = (ctrlm_rf4ce_device_update_audio_theme_t)atol(audio_theme.c_str());
         if(image_info->audio_theme > RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID) {
            image_info->audio_theme = RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID;
         }
      }
   }

   image_info->device_name  = ctrlm_xml_tag_text_get(xml, "image:productName");
   if(image_info->device_name.length() == 0) {
      XLOGD_ERROR("Missing Device Name");
      return false;
   }

   char type_z = image_info->device_name.back();
   image_info->type_z = (type_z == 'Z') ? true : false;

   image_info->file_name_image  = ctrlm_xml_tag_text_get(xml, "image:fileName");
   if(image_info->file_name_image.length() == 0) {
      XLOGD_ERROR("Missing File Name");
      return false;
   }

   //TODO Check to make sure image file exists and CRC matches

   string size  = ctrlm_xml_tag_text_get(xml, "image:size");
   if(size.length() == 0) {
      XLOGD_ERROR("Missing Size");
      return false;
   }
   image_info->size = atol(size.c_str());

   string crc  = ctrlm_xml_tag_text_get(xml, "image:CRC");
   if(crc.length() == 0) {
      XLOGD_ERROR("Missing CRC");
      return false;
   }
   image_info->crc = strtoul(crc.c_str(), NULL, 16);

   string force_update = ctrlm_xml_tag_text_get(xml, "image:force_update");
   if(force_update.length() == 0) {
      XLOGD_INFO("Missing force update flag");
      image_info->force_update = false;
   } else if(force_update == "1"){
      image_info->force_update = true;
   } else {
      XLOGD_ERROR("Invalid force update flag <%s>", force_update.c_str());
      image_info->force_update = false;
   }
#ifdef CTRLM_NETWORK_RF4CE
   XLOGD_INFO("device <%s> type <%s> size %u crc 0x%08X", image_info->device_name.c_str(), ctrlm_rf4ce_device_update_image_type_str(image_info->image_type), image_info->size, image_info->crc);
   XLOGD_INFO("sw version <%u.%u.%u.%u> force update <%s> image file name <%s> audio theme <%s> type z <%s>", image_info->version_software.major, image_info->version_software.minor, image_info->version_software.revision, image_info->version_software.patch, image_info->force_update ? "YES" : "NO", image_info->file_name_image.c_str(), ctrlm_rf4ce_device_update_audio_theme_str(image_info->audio_theme), image_info->type_z ? "YES" : "NO");
   XLOGD_INFO("hw version min <%u.%u.%u>", image_info->version_hardware_min.manufacturer, image_info->version_hardware_min.model, image_info->version_hardware_min.hw_revision);
   XLOGD_INFO("bldr version min <%u.%u.%u.%u>", image_info->version_bootloader_min.major, image_info->version_bootloader_min.minor, image_info->version_bootloader_min.revision, image_info->version_bootloader_min.patch);
#endif
   return true;
}

void ctrlm_device_update_tmp_dir_make() {
   XLOGD_INFO("<%s>", g_ctrlm_device_update.prefs.temp_file_path.c_str());
   errno = 0;
   if(0 != mkdir(g_ctrlm_device_update.prefs.temp_file_path.c_str(), S_IRWXU | S_IRWXG)){
      int errsv = errno;
      XLOGD_ERROR("Failed to mkdir, mkdir error (%s)", strerror(errsv));
      return;
   }
}

void ctrlm_device_update_tmp_dir_remove() {
   XLOGD_INFO("<%s>", g_ctrlm_device_update.prefs.temp_file_path.c_str());
   if( true != ctrlm_utils_rm_rf(g_ctrlm_device_update.prefs.temp_file_path) ) {
      XLOGD_WARN("Failed to remove \"%s\" due to ctrlm_utils_rm_rf call error", g_ctrlm_device_update.prefs.temp_file_path.c_str());
   }
}

void ctrlm_device_update_rf4ce_tmp_dir_make() {
   string dir = g_ctrlm_device_update.prefs.temp_file_path + "rf4ce/";
   XLOGD_INFO("<%s>", dir.c_str());
   errno = 0;
   if(0 != mkdir(dir.c_str(), S_IRWXU | S_IRWXG)){
      int errsv = errno;
      XLOGD_ERROR("Failed to mkdir, mkdir error (%s)", strerror(errsv));
      return;
   }
}

void ctrlm_device_update_rf4ce_tmp_dir_remove() {
   string dir = g_ctrlm_device_update.prefs.temp_file_path + "rf4ce/";
   XLOGD_INFO("<%s>", dir.c_str());
   if( true != ctrlm_utils_rm_rf(dir) ) {
      XLOGD_WARN("Failed to remove \"%s\" due to ctrlm_utils_rm_rf call error", dir.c_str());
   }
}

gboolean ctrlm_device_update_rf4ce_check_dir_exists(string path){
   struct stat st;
   XLOGD_INFO("test for dir <%s>", path.c_str());

   if(stat(path.c_str(),&st) == 0){
       if((st.st_mode & S_IFDIR) != 0){
           return true;
       }
   }
   XLOGD_INFO("dir not found <%s>", path.c_str());
   return false;
}

void ctrlm_device_update_rf4ce_archive_extract(string file_path_archive, string file_name_archive) {
   //TODO since we are using system commands and not checking exit codes we should probably at least check
   //     to see if the file exists and return failure if not.  It would be better to check tar exit code
   //     to verify the file exists and untar'd correctly
   string dir = g_ctrlm_device_update.prefs.temp_file_path + "rf4ce/" + file_name_archive + "/";
   // if we get here we need the tmp directories, so check for and create them if they do not exist;
   if(ctrlm_device_update_rf4ce_check_dir_exists(g_ctrlm_device_update.prefs.temp_file_path + "rf4ce/")==false){
      ctrlm_device_update_tmp_dir_make();
      ctrlm_device_update_rf4ce_tmp_dir_make();
   }
   XLOGD_INFO("<%s> <%s>", file_name_archive.c_str(), dir.c_str());
   errno = 0;
   if(0 != mkdir(dir.c_str(), S_IRWXU | S_IRWXG)){
      int errsv = errno;
      XLOGD_ERROR("Failed to mkdir, mkdir error (%s)", strerror(errsv));
      return;
   }

   if( true != ctrlm_tar_archive_extract(file_path_archive, dir) ) {
      XLOGD_WARN("Failed to extract the archive due to ctrlm_tar_archive_extract call error");
      return;
   }
}

void ctrlm_device_update_rf4ce_archive_remove(const std::string &file_name_archive) {
   std::string dir = g_ctrlm_device_update.prefs.temp_file_path + "rf4ce/" + file_name_archive + "/";
   XLOGD_INFO("<%s> <%s>", file_name_archive.c_str(), dir.c_str());
   if( true != ctrlm_utils_rm_rf(dir) ) {
      XLOGD_WARN("Failed to remove \"%s\" due to ctrlm_utils_rm_rf call error", dir.c_str());
   }
}

guint32 ctrlm_device_update_request_timeout_get(void) {
   return g_ctrlm_device_update.prefs.download.request_timeout;
}

gboolean ctrlm_device_update_rf4ce_is_image_available(ctrlm_rf4ce_device_update_image_type_t image_type, ctrlm_rf4ce_controller_type_t type, version_hardware_t version_hardware, version_software_t version_bootloader, version_software_t version_software, ctrlm_rf4ce_device_update_audio_theme_t audio_theme, rf4ce_device_update_image_info_t *image_info, bool type_z) {
   gboolean           found   = false;
   rf4ce_device_update_image_info_t upgrade;
   errno_t safec_rc = -1;
   safec_rc = memset_s(&upgrade, sizeof(upgrade), 0, sizeof(upgrade));
   ERR_CHK(safec_rc);
   upgrade.version = version_software;

#ifdef CTRLM_NETWORK_RF4CE
   XLOGD_INFO("Controller type <%s> Current  image type <%s> theme <%s> hw ver <%u.%u.%u.%u> bldr ver <%u.%u.%u.%u> sw ver <%u.%u.%u.%u> type Z <%s>", ctrlm_rf4ce_controller_type_str(type), ctrlm_rf4ce_device_update_image_type_str(image_type), ctrlm_rf4ce_device_update_audio_theme_str(audio_theme), version_hardware.manufacturer, version_hardware.model, version_hardware.hw_revision, version_hardware.lot_code, version_bootloader.major, version_bootloader.minor, version_bootloader.revision, version_bootloader.patch, version_software.major, version_software.minor, version_software.revision, version_software.patch, type_z ? "YES":"NO");
#endif

   DEVICE_UPDATE_MUTEX_LOCK();

   if((image_type == RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_1) && (audio_theme < RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID)) {
      // User has set an audio theme.  Find the image and set a force update
      for(vector<ctrlm_device_update_rf4ce_image_info_t>::iterator it = g_ctrlm_device_update.rf4ce_images->begin(); it < g_ctrlm_device_update.rf4ce_images->end(); it++) {
#ifdef CTRLM_NETWORK_RF4CE
         XLOGD_INFO("Checking for audio theme image type <%s> controller_type <%s>  hw ver min <%u.%u.%u.%u> bldr ver min <%u.%u.%u.%u> sw ver <%u.%u.%u.%u>", ctrlm_rf4ce_device_update_image_type_str(it->image_type), ctrlm_rf4ce_controller_type_str(it->controller_type), it->version_hardware_min.manufacturer, it->version_hardware_min.model, it->version_hardware_min.hw_revision, it->version_hardware_min.lot_code,
               it->version_bootloader_min.major, it->version_bootloader_min.minor, it->version_bootloader_min.revision, it->version_bootloader_min.patch, it->version_software.major, it->version_software.minor, it->version_software.revision, it->version_software.patch);
#endif
         if(it->image_type == image_type && it->controller_type == type && ctrlm_device_update_rf4ce_is_hardware_version_min_met(version_hardware, it->version_hardware_min)
                                                                        && ctrlm_device_update_rf4ce_is_software_version_min_met(version_bootloader, it->version_bootloader_min)) {
            // Image is for this device.  Check for force update or newer image
            if(it->audio_theme == audio_theme) {
               upgrade.id           = it->id;
               upgrade.version      = it->version_software;
               upgrade.size         = it->size;
               upgrade.crc          = it->crc;
               upgrade.force_update = true; // Themes are always force updates
               upgrade.do_not_load  = g_ctrlm_device_update.prefs.download.load_immediately ? false : true;
               upgrade.type_z       = it->type_z;
               safec_rc = strncpy_s(upgrade.file_name, sizeof(upgrade.file_name), it->file_path_archive.c_str(), sizeof(upgrade.file_name) - 1);
               ERR_CHK(safec_rc);
               found = true;
#ifdef CTRLM_NETWORK_RF4CE
               XLOGD_INFO("Found AUDIO THEME <%s> sw version <%u.%u.%u.%u> id %u size %u crc 0x%08X", ctrlm_rf4ce_device_update_audio_theme_str(audio_theme), it->version_software.major, it->version_software.minor, it->version_software.revision, it->version_software.patch, it->id, it->size, it->crc);
#endif
               break;
            }
         }
      }
      // Continue to search for higher version below like normal
   }

   // Check for a newer version of image for this controller type
   for(vector<ctrlm_device_update_rf4ce_image_info_t>::iterator it = g_ctrlm_device_update.rf4ce_images->begin(); it < g_ctrlm_device_update.rf4ce_images->end(); it++) {
#ifdef CTRLM_NETWORK_RF4CE
      XLOGD_INFO("Checking image type <%s> theme <%s> controller_type <%s>  hw ver min <%u.%u.%u.%u> bldr ver min <%u.%u.%u.%u> sw ver <%u.%u.%u.%u> force <%s> type z <%s>", ctrlm_rf4ce_device_update_image_type_str(it->image_type), ctrlm_rf4ce_device_update_audio_theme_str(it->audio_theme), ctrlm_rf4ce_controller_type_str(it->controller_type), it->version_hardware_min.manufacturer, it->version_hardware_min.model, it->version_hardware_min.hw_revision, it->version_hardware_min.lot_code, it->version_bootloader_min.major, it->version_bootloader_min.minor, it->version_bootloader_min.revision, it->version_bootloader_min.patch, it->version_software.major, it->version_software.minor, it->version_software.revision, it->version_software.patch, it->force_update ? "YES" : "NO", it->type_z ? "YES" : "NO");
#endif
      if((it->image_type == image_type) && (it->controller_type == type) && ((it->audio_theme == RF4CE_DEVICE_UPDATE_AUDIO_THEME_DEFAULT) || (it->audio_theme >= RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID))
                                                                         && ctrlm_device_update_rf4ce_is_hardware_version_min_met(version_hardware, it->version_hardware_min)
                                                                         && ctrlm_device_update_rf4ce_is_software_version_min_met(version_bootloader, it->version_bootloader_min)
                                                                         && (it->type_z == type_z)) {
         // Image is for this device.  Check for force update or newer image
         XLOGD_INFO("Image is compatible with the hardware");

         // Force update version is compared against actual remote software version in version_software variable instead of the highest upgrade file found so far in upgrade.version
         if(it->force_update && ctrlm_device_update_rf4ce_is_software_version_not_equal(version_software, it->version_software)) {
            // Now check to make sure we haven't already found a higher version than the remote software version
            if(!ctrlm_device_update_rf4ce_is_software_version_higher(version_software, upgrade.version)) {
               upgrade.id           = it->id;
               upgrade.version      = it->version_software;
               upgrade.size         = it->size;
               upgrade.crc          = it->crc;
               upgrade.force_update = true;
               upgrade.do_not_load  = g_ctrlm_device_update.prefs.download.load_immediately ? false : true;
               upgrade.type_z       = it->type_z;
               safec_rc = strncpy_s(upgrade.file_name, sizeof(upgrade.file_name), it->file_path_archive.c_str(), sizeof(upgrade.file_name) - 1);
               ERR_CHK(safec_rc);
               found = true;
               XLOGD_INFO("Found FORCE UPDATE sw version <%u.%u.%u.%u> id %u size %u crc 0x%08X", it->version_software.major, it->version_software.minor, it->version_software.revision, it->version_software.patch, it->id, it->size, it->crc);
            } else {
               XLOGD_INFO("Ignoring FORCE UPDATE sw version <%u.%u.%u.%u> id %u size %u crc 0x%08X", it->version_software.major, it->version_software.minor, it->version_software.revision, it->version_software.patch, it->id, it->size, it->crc);
            }
         } else if(ctrlm_device_update_rf4ce_is_software_version_higher(version_software, it->version_software)) {
            // Now check to make sure the version is higher than any previous versions found
            if(ctrlm_device_update_rf4ce_is_software_version_higher(upgrade.version, it->version_software)) {
               upgrade.id           = it->id;
               upgrade.version      = it->version_software;
               upgrade.size         = it->size;
               upgrade.crc          = it->crc;
               upgrade.force_update = false;
               upgrade.do_not_load  = g_ctrlm_device_update.prefs.download.load_immediately ? false : true;
               upgrade.type_z       = it->type_z;
               safec_rc = strncpy_s(upgrade.file_name, sizeof(upgrade.file_name), it->file_path_archive.c_str(), sizeof(upgrade.file_name) - 1);
               ERR_CHK(safec_rc);
               found = true;
               XLOGD_INFO("Found sw version <%u.%u.%u.%u> id %u size %u crc 0x%08X", it->version_software.major, it->version_software.minor, it->version_software.revision, it->version_software.patch, it->id, it->size, it->crc);
            } else {
               XLOGD_INFO("Ignoring NORMAL UPDATE sw version <%u.%u.%u.%u> id %u size %u crc 0x%08X", it->version_software.major, it->version_software.minor, it->version_software.revision, it->version_software.patch, it->id, it->size, it->crc);
            }
         }
         // The highest software version will always win regardless of the mix of force update and non-force update images
      }
   }
   upgrade.file_name[CTRLM_DEVICE_UPDATE_PATH_LENGTH - 1] = '\0';

   DEVICE_UPDATE_MUTEX_UNLOCK();

   if(!found) {
      switch(image_type) {
         case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_FIRMWARE: {
            XLOGD_INFO("No firmware image found.");
            break;
         }
         case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_1: {
            XLOGD_INFO("No audio data image found.");
            break;
         }
         default: {
            XLOGD_INFO("No other image found.");
         }
      }
      return(false);
   }
   
   // Fill in the image information
   if(image_info) {
      *image_info = upgrade;
   }
   
   return(true);
}

gboolean ctrlm_device_update_rf4ce_is_software_version_not_equal(version_software_t current, version_software_t proposed) {
   guint32 num_current, num_proposed;
   num_current  = (current.major  << 24) | (current.minor  << 16) | (current.revision  << 8) | current.patch;
   num_proposed = (proposed.major << 24) | (proposed.minor << 16) | (proposed.revision << 8) | proposed.patch;

   if(num_proposed != num_current) {
      return true;
   }
   return false;
}

gboolean ctrlm_device_update_rf4ce_is_software_version_higher(version_software_t current, version_software_t proposed) {
   guint32 num_current, num_proposed;
   num_current  = (current.major  << 24) | (current.minor  << 16) | (current.revision  << 8) | current.patch;
   num_proposed = (proposed.major << 24) | (proposed.minor << 16) | (proposed.revision << 8) | proposed.patch;

   if(num_proposed > num_current) {
      return true;
   }
   return false;
}

gboolean ctrlm_device_update_rf4ce_is_software_version_min_met(version_software_t current, version_software_t minimum) {
   guint32 num_current, num_minimum;
   num_current = (current.major << 24) | (current.minor << 16) | (current.revision << 8) | current.patch;
   num_minimum = (minimum.major << 24) | (minimum.minor << 16) | (minimum.revision << 8) | minimum.patch;

   if(num_current >= num_minimum) {
      return true;
   }
   return false;
}

gboolean ctrlm_device_update_rf4ce_is_hardware_version_min_met(version_hardware_t current, version_hardware_t minimum) {
   // Compare manufacturer
   if(current.manufacturer != minimum.manufacturer) {
      return(false);
   }
   // Compare model
   if(current.model < minimum.model) {
      return(false);
   }
   // Compare hw revision
   if(current.hw_revision < minimum.hw_revision) {
      return(false);
   }
   return(true);
}

gboolean ctrlm_device_update_process_xconf_update(string fileLocation) {
   //we need to process the information in the xml part of the tar just like we would for a tar built in file system.
   device_update_queue_msg_process_update_t *data = (device_update_queue_msg_process_update_t *)g_malloc(sizeof(device_update_queue_msg_process_update_t));
   if(data == NULL) {
      XLOGD_ERROR("Out of memory");
      return(false);
   }

   data->header.type = DEVICE_UPDATE_QUEUE_MSG_TYPE_PROCESS_XCONF;
   errno_t safec_rc = strncpy_s(data->update_file_name, sizeof(data->update_file_name), fileLocation.c_str(), CTRLM_DEVICE_UPDATE_PATH_LENGTH - 1);
   ERR_CHK(safec_rc);
   data->update_file_name[CTRLM_DEVICE_UPDATE_PATH_LENGTH - 1] = '\0';

   ctrlm_device_update_queue_msg_push((gpointer) data);

   return(true);
}

gboolean ctrlm_device_update_rf4ce_image_stage(ctrlm_controller_id_t controller_id, guint16 image_id, guint32 session_count, guint32 rf4ce_session_count, guint32 reader_count) {

   if(image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
      XLOGD_WARN("Image not found %u", image_id);
      return(false);
   }

   device_update_queue_msg_stage_t *stage = (device_update_queue_msg_stage_t *)g_malloc(sizeof(device_update_queue_msg_stage_t));
   if(stage == NULL) {
      XLOGD_ERROR("Out of memory");
      return(false);
   }

   stage->header.type         = DEVICE_UPDATE_QUEUE_MSG_TYPE_IMAGE_STAGE;
   stage->image_id            = image_id;
   stage->controller_id       = controller_id;
   stage->session_count       = session_count;
   stage->rf4ce_session_count = rf4ce_session_count;
   stage->reader_count        = reader_count;

   ctrlm_device_update_queue_msg_push((gpointer) stage);

   return(true);
}

gboolean ctrlm_device_update_rf4ce_image_unstage(ctrlm_controller_id_t controller_id, guint16 image_id, guint32 session_count, guint32 rf4ce_session_count, guint32 reader_count) {

   if(image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
      XLOGD_WARN("Image not found %u", image_id);
      return(false);
   }

   device_update_queue_msg_unstage_t *unstage = (device_update_queue_msg_unstage_t *)g_malloc(sizeof(device_update_queue_msg_unstage_t));
   if(unstage == NULL) {
      XLOGD_ERROR("Out of memory");
      return(false);
   }

   unstage->header.type         = DEVICE_UPDATE_QUEUE_MSG_TYPE_IMAGE_UNSTAGE;
   unstage->image_id            = image_id;
   unstage->controller_id       = controller_id;
   unstage->session_count       = session_count;
   unstage->rf4ce_session_count = rf4ce_session_count;
   unstage->reader_count        = reader_count;

   ctrlm_device_update_queue_msg_push((gpointer)unstage);

   return(true);
}

gboolean ctrlm_device_update_rf4ce_image_read_next_chunk(ctrlm_controller_id_t controller_id, guint16 image_id, guint32 *offset) {

   if(image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
      XLOGD_WARN("Image not found %u", image_id);
      return(false);
   }

   device_update_queue_msg_read_t *read = (device_update_queue_msg_read_t *)g_malloc(sizeof(device_update_queue_msg_read_t));
   if(read == NULL) {
      XLOGD_ERROR("Out of memory");
      return(false);
   }

   read->header.type   = DEVICE_UPDATE_QUEUE_MSG_TYPE_IMAGE_READ;
   read->image_id      = image_id;
   read->controller_id = controller_id;
   if(offset == NULL) {
      read->use_offset = false;
      read->offset     = 0;
   } else {
      read->use_offset = true;
      read->offset     = *offset;
   }

   ctrlm_device_update_queue_msg_push((gpointer)read);

   return(true);
}


gpointer ctrlm_device_update_thread(gpointer param) {
   bool running = true;
   XLOGD_INFO("Started");

   // Unblock the caller that launched this thread
   sem_post(&g_ctrlm_device_update.semaphore);

   XLOGD_INFO("Enter main loop");
   do {
      gpointer msg = g_async_queue_pop(g_ctrlm_device_update.queue);
      if(msg == NULL) {
         XLOGD_ERROR("NULL message received");
         continue;
      }

      device_update_queue_msg_header_t *header = (device_update_queue_msg_header_t *)msg;
      switch(header->type) {
         case DEVICE_UPDATE_QUEUE_MSG_TYPE_TERMINATE: {
            XLOGD_INFO("DEVICE_UPDATE_QUEUE_MSG_TYPE_TERMINATE");
            sem_post(&g_ctrlm_device_update.semaphore);
            running = false;
            break;
         }
         case CTRLM_DEVICE_UPDATE_QUEUE_MSG_TYPE_TICKLE: {
            XLOGD_DEBUG("TICKLE");
            ctrlm_thread_monitor_msg_t *thread_monitor_msg = (ctrlm_thread_monitor_msg_t *) msg;
            *thread_monitor_msg->response = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
            break;
         }
         case DEVICE_UPDATE_QUEUE_MSG_TYPE_PROCESS_XCONF: {
            device_update_queue_msg_process_update_t* xconf_msg = (device_update_queue_msg_process_update_t *)msg;
            XLOGD_INFO("DEVICE_UPDATE_QUEUE_MSG_TYPE_PROCESS_XCONF file %s",xconf_msg->update_file_name);

            bool network_managing_upgrade = false;

            ctrlm_main_queue_msg_network_fw_upgrade_t msg;
            errno_t safec_rc = -1;
            safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
            ERR_CHK(safec_rc);
            safec_rc = strncpy_s(msg.temp_dir_path, sizeof(msg.temp_dir_path), g_ctrlm_device_update.prefs.temp_file_path.c_str(), sizeof(msg.temp_dir_path) - 1);
            ERR_CHK(safec_rc);
            msg.temp_dir_path[CTRLM_DEVICE_UPDATE_PATH_LENGTH - 1] = '\0';
            safec_rc = strncpy_s(msg.archive_file_path, sizeof(msg.archive_file_path), xconf_msg->update_file_name, sizeof(msg.archive_file_path) - 1);
            ERR_CHK(safec_rc);
            msg.archive_file_path[CTRLM_DEVICE_UPDATE_PATH_LENGTH - 1] = '\0';
            msg.network_managing_upgrade = &network_managing_upgrade;

            ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_network_managed_upgrade, &msg, sizeof(msg), NULL, CTRLM_MAIN_NETWORK_ID_ALL, true);

            if (!network_managing_upgrade) {
               ctrlm_device_update_process_device_file(xconf_msg->update_file_name, "", NULL);
               ctrlm_device_update_rf4ce_session_resume_check(g_ctrlm_device_update.sessions, false);
            }
            break;
         }
         case DEVICE_UPDATE_QUEUE_MSG_TYPE_IMAGE_STAGE: {
            device_update_queue_msg_stage_t *stage = (device_update_queue_msg_stage_t *)msg;
            XLOGD_INFO("LOAD image id %u", stage->image_id);

            ctrlm_device_update_rf4ce_image_info_t *image_info = &(*g_ctrlm_device_update.rf4ce_images)[stage->image_id];
            ctrlm_device_update_rf4ce_session_t    *session_info;

            if(g_ctrlm_device_update.rf4ce_sessions.count(stage->controller_id) == 0) {
               XLOGD_ERROR("STAGE session not found %u", stage->controller_id);
               break;
            }
            session_info = &g_ctrlm_device_update.rf4ce_sessions[stage->controller_id];

            if(stage->session_count == 1) {
               // Create the temp directory
               ctrlm_device_update_tmp_dir_make();
            }
            if(stage->rf4ce_session_count == 1) {
               // Create the RF4CE temp directory
               ctrlm_device_update_rf4ce_tmp_dir_make();
            }
            if(stage->reader_count == 1) { // Image is NOT already extracted
               // Extract files from archive
               ctrlm_device_update_rf4ce_archive_extract(image_info->file_path_archive, image_info->file_name_archive);
            }

            // Open the file with read access
            string file_path_image = g_ctrlm_device_update.prefs.temp_file_path + "rf4ce/" + image_info->file_name_archive + "/" + image_info->file_name_image;

            XLOGD_INFO("Opening image file <%s>", file_path_image.c_str());

            session_info->fd = fopen(file_path_image.c_str(), "r");
            if(session_info->fd == NULL) {
               XLOGD_TELEMETRY("Unable to open image file <%s>", file_path_image.c_str());
               break;
            }

            // Allocate two chunk buffers
            char *chunks = (char *) g_malloc(DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2);
            if(chunks == NULL) {
               XLOGD_INFO("Out of memory");
            } else {
               size_t size = fread(chunks, 1, DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2, session_info->fd);

               if(size != DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2) {
                  XLOGD_INFO("Unable to read first two chunks %u", size);
                  g_free(chunks);
               } else {
                  DEVICE_UPDATE_MUTEX_LOCK();

                  session_info->chunk_even      = chunks;
                  session_info->chunk_odd       = chunks + DEVICE_UPDATE_IMAGE_CHUNK_SIZE;
                  session_info->offset_even     = 0;
                  session_info->offset_odd      = DEVICE_UPDATE_IMAGE_CHUNK_SIZE;
                  session_info->bytes_read_file = size;

                  DEVICE_UPDATE_MUTEX_UNLOCK();
               }
            }
            break;
         }
         case DEVICE_UPDATE_QUEUE_MSG_TYPE_IMAGE_READ: {
            device_update_queue_msg_read_t *read = (device_update_queue_msg_read_t *)msg;

            ctrlm_device_update_rf4ce_image_info_t *image_info = &(*g_ctrlm_device_update.rf4ce_images)[read->image_id];
            ctrlm_device_update_rf4ce_session_t    *session_info;

            if(g_ctrlm_device_update.rf4ce_sessions.count(read->controller_id) == 0) {
               XLOGD_ERROR("READ session not found %u", read->controller_id);
               break;
            }
            session_info = &g_ctrlm_device_update.rf4ce_sessions[read->controller_id];

            if(read->use_offset) { // Controller has resumed download from a different point in the file
               XLOGD_INFO("RESET Image id %u Offset %u", read->image_id, read->offset);

               guint32 offset = read->offset & ~(DEVICE_UPDATE_IMAGE_CHUNK_SIZE - 1);

               int rc = fseek(session_info->fd, offset, SEEK_SET);
               if(rc != 0) {
                  int errsv = errno;
                  XLOGD_ERROR("%u fseek failed  <%s>", read->controller_id, strerror(errsv));
                  break;
               }

               size_t size = fread(session_info->chunk_even, 1, DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2, session_info->fd);

               if(size != DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2) {
                  XLOGD_INFO("Unable to read two chunks %u", size);
               } else {
                  DEVICE_UPDATE_MUTEX_LOCK();

                  session_info->offset_even     = offset;
                  session_info->offset_odd      = offset + DEVICE_UPDATE_IMAGE_CHUNK_SIZE;
                  session_info->bytes_read_file = offset + (DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2);
                  session_info->resume_pending  = false;

                  DEVICE_UPDATE_MUTEX_UNLOCK();
               }
            } else {
               guint32 chunk_size = DEVICE_UPDATE_IMAGE_CHUNK_SIZE;

               XLOGD_INFO("READ Image id %u Offset %u", read->image_id, session_info->bytes_read_file);

               // This section does not use a mutex to avoid blocking the controller image read which is time sensitive.

               if(session_info->offset_even > session_info->offset_odd) {
                  if(session_info->offset_odd + DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 3 >= image_info->size) {
                     XLOGD_INFO("End of file reached %u", image_info->size);
                     chunk_size = image_info->size - (session_info->offset_odd + DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2);
                  }
                  size_t size = fread(session_info->chunk_odd, 1, chunk_size, session_info->fd);
                  if(size == chunk_size) {
                     session_info->offset_odd      += DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2;
                     session_info->bytes_read_file += size;
                  } else {
                     XLOGD_INFO("Unable to read next chunk. req %u got %u", chunk_size, size);
                  }
               } else {
                  if(session_info->offset_even + DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 3 >= image_info->size) {
                     XLOGD_INFO("End of file reached %u", image_info->size);
                     chunk_size = image_info->size - (session_info->offset_even + DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2);
                  }
                  size_t size = fread(session_info->chunk_even, 1, chunk_size, session_info->fd);
                  if(size == chunk_size) {
                     session_info->offset_even     += DEVICE_UPDATE_IMAGE_CHUNK_SIZE * 2;
                     session_info->bytes_read_file += size;
                  } else {
                     XLOGD_INFO("Unable to read next chunk. req %u got %u", chunk_size, size);
                  }
               }
            }
            break;
         }
         case DEVICE_UPDATE_QUEUE_MSG_TYPE_IMAGE_UNSTAGE: {
            device_update_queue_msg_unstage_t *unstage = (device_update_queue_msg_unstage_t *)msg;
            XLOGD_INFO("UNLOAD image id %u", unstage->image_id);
            ctrlm_device_update_rf4ce_image_info_t *image_info = &(*g_ctrlm_device_update.rf4ce_images)[unstage->image_id];
            ctrlm_device_update_rf4ce_session_t    *session_info;

            if(g_ctrlm_device_update.rf4ce_sessions.count(unstage->controller_id) == 0) {
               XLOGD_ERROR("UNSTAGE session not found %u", unstage->controller_id);
            }

            DEVICE_UPDATE_MUTEX_LOCK();

            session_info = &g_ctrlm_device_update.rf4ce_sessions[unstage->controller_id];

            if(session_info->fd != NULL) {
               fclose(session_info->fd);
               session_info->fd = NULL;
            }

            if(session_info->chunk_even != NULL) {
               g_free(session_info->chunk_even);
            }
            session_info->chunk_even  = NULL;
            session_info->chunk_odd   = NULL;
            session_info->offset_even = 0;
            session_info->offset_odd  = 0;

            // Remove timeout source
            ctrlm_device_update_timeout_session_destroy(&g_ctrlm_device_update.rf4ce_sessions[unstage->controller_id].timeout_source_id);

            // Delete the session mapping
            if(NULL != g_ctrlm_device_update.rf4ce_sessions[unstage->controller_id].timeout_params) {
               g_free(g_ctrlm_device_update.rf4ce_sessions[unstage->controller_id].timeout_params);
            }
            g_ctrlm_device_update.rf4ce_sessions.erase(unstage->controller_id);

            DEVICE_UPDATE_MUTEX_UNLOCK();
            if(unstage->reader_count == 0) { // No more readers are using this image
               // Delete the extracted archive
               ctrlm_device_update_rf4ce_archive_remove(image_info->file_name_archive);
            }
            if(unstage->rf4ce_session_count == 0) {
               // Clean up the temp directory
               ctrlm_device_update_rf4ce_tmp_dir_remove();
            }
            if(unstage->session_count == 0) {
               // Clean up the temp directory
               ctrlm_device_update_tmp_dir_remove();
            }
            break;
         }
         default: {
            break;
         }
      }
      g_free(msg);
   } while(running);
   return(NULL);
}


guint16 ctrlm_device_update_rf4ce_image_data_read(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint16 image_id, guint32 offset, guint16 length, guchar *data) {
   guint16 qty_read = 0;
   errno_t safec_rc = -1;

   // is image id valid?
   if(image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
      XLOGD_ERROR("Controller id %u Image not found %u", controller_id, image_id);
      return(qty_read);
   }
   ctrlm_device_update_rf4ce_image_info_t *image_info = &(*g_ctrlm_device_update.rf4ce_images)[image_id];
   ctrlm_device_update_rf4ce_session_t    *session_info;

   DEVICE_UPDATE_MUTEX_LOCK();

   if(g_ctrlm_device_update.rf4ce_sessions.count(controller_id) == 0) {
      XLOGD_ERROR("Controller id %u: session not found", controller_id);
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return(qty_read);
   }
   session_info = &g_ctrlm_device_update.rf4ce_sessions[controller_id];

   // are offset and length valid?
   if(offset > image_info->size || offset + length > image_info->size) {
      XLOGD_ERROR("Controller id %u: Attempt to read past EOF. offset %u length %u size %u", controller_id, offset, length, image_info->size);
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return(qty_read);
   }
   
   // Kick the session timeout
   ctrlm_device_update_timeout_session_update(&session_info->timeout_source_id, g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_value, session_info->timeout_params);

   if(session_info->chunk_even == NULL) {
      XLOGD_INFO("Controller id %u: : No data yet", controller_id);
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return(qty_read);
   }
   XLOGD_INFO("Controller id %u: Image id %u Offset %u Length %u", controller_id, image_id, offset, length);

   // Read the data
   if(offset >= session_info->offset_even && (offset + length) <= (session_info->offset_even + DEVICE_UPDATE_IMAGE_CHUNK_SIZE)) { // Data is in even chunk
      safec_rc = memcpy_s(data, length, &session_info->chunk_even[offset - session_info->offset_even], length);
      ERR_CHK(safec_rc);
      qty_read = length;
   } else if(offset >= session_info->offset_odd && (offset + length) <= (session_info->offset_odd + DEVICE_UPDATE_IMAGE_CHUNK_SIZE)) { // Data is in odd chunk
      safec_rc = memcpy_s(data, length, &session_info->chunk_odd[offset - session_info->offset_odd], length);
      ERR_CHK(safec_rc);
      qty_read = length;
   } else if(session_info->offset_odd  > session_info->offset_even && offset < session_info->offset_odd && (offset + length) > session_info->offset_even) { // Data crosses from even to odd
      safec_rc = memcpy_s(data, length, &session_info->chunk_even[offset - session_info->offset_even], length);
      ERR_CHK(safec_rc);
      qty_read = length;
   } else if(session_info->offset_even > session_info->offset_odd  && offset < session_info->offset_even  && (offset + length) > session_info->offset_odd)  { // Data crosses from odd to even
      guint16 length_odd, length_even;
      length_odd  = session_info->offset_even - offset;
      length_even = length - length_odd;
      //XLOGD_INFO("Odd to even crossing. length odd %u even %u", length_odd, length_even);
      safec_rc = memcpy_s(data, length, &session_info->chunk_odd[offset - session_info->offset_odd], length_odd);
      ERR_CHK(safec_rc);
      safec_rc = memcpy_s(&data[length_odd], length, &session_info->chunk_even[0], length_even);
      ERR_CHK(safec_rc);
      qty_read = length;
   } else {
      XLOGD_INFO("Controller id %u: Data not found. offset odd <%u> even <%u> chunk size <%u> resume pending <%s>", controller_id, session_info->offset_odd, session_info->offset_even, DEVICE_UPDATE_IMAGE_CHUNK_SIZE, session_info->resume_pending ? "YES" : "NO");

      // Controller is resuming download
      if(!session_info->resume_pending) {
         ctrlm_device_update_rf4ce_image_read_next_chunk(controller_id, image_id, &offset);
         session_info->bytes_read_controller = offset;
         session_info->resume_pending        = true;
      }
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return(0);
   }
   
   // Request to read the next chunk?
   if((qty_read == length) && ((offset + length + DEVICE_UPDATE_IMAGE_CHUNK_THRESHOLD) > session_info->bytes_read_file) && (session_info->bytes_read_file < image_info->size)) {
      ctrlm_device_update_rf4ce_image_read_next_chunk(controller_id, image_id, NULL);
   }

   session_info->bytes_read_controller += qty_read;
   guchar percent_complete = (session_info->bytes_read_controller * 100) / image_info->size;
   if(percent_complete >= session_info->percent_next) {
      ctrlm_device_update_iarm_event_download_status(session_info->session_id, percent_complete);
      session_info->percent_next +=  session_info->percent_increment;
   } else if(session_info->bytes_read_controller >= image_info->size) {
      ctrlm_device_update_iarm_event_download_status(session_info->session_id, 100);
   }

  
   if((100 == percent_complete || session_info->bytes_read_controller >= image_info->size) && false == session_info->load_waiting) {
      session_info->load_waiting = true;
      if(g_ctrlm_device_update.rf4ce_session_active_count > 0) {
         g_ctrlm_device_update.rf4ce_session_active_count--;
      }
      XLOGD_INFO("Data Download Complete, RF4CE Active Download Session Count %u", g_ctrlm_device_update.rf4ce_session_active_count);
   }

   DEVICE_UPDATE_MUTEX_UNLOCK();

   //ctrlm_print_data_hex(__FUNCTION__, data, qty_read, 32);

   // Return the number of bytes read
   return(qty_read);
}

//gboolean ctrlm_device_update_is_rf4ce_session_in_progress(void) {
//   if(g_ctrlm_device_update.rf4ce_session_count) {
//      return(true);
//   }
//   return(false);
//}

//gboolean ctrlm_device_update_on_this_network(ctrlm_network_id_t network_id) {
   //XLOGD_DEBUG("0x%X 0x%X", g_ctrlm_validation.network_id, network_id);
   //return(g_ctrlm_validation.network_id == network_id);
//   return(false);
//}

//gboolean ctrlm_device_update_on_this_controller(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id) {
   //XLOGD_DEBUG("0x%X 0x%X", g_ctrlm_validation.controller_id, controller_id);
   //return(g_ctrlm_validation.controller_id == controller_id);
//   return(false);
//}

gboolean ctrlm_device_update_rf4ce_begin(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, version_hardware_t version_hardware, version_software_t version_bootloader, version_software_t version_software, guint16 image_id, guint32 timeout, gboolean manual_poll, rf4ce_device_update_begin_info_t *begin_info, ctrlm_device_update_session_id_t *session_id_in, ctrlm_device_update_session_id_t *session_id_out, gboolean session_resume) {
   XLOGD_INFO("(%u, %u) image id %u", network_id, controller_id, image_id);

   if(image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
      XLOGD_ERROR("Invalid Image id (%u)", image_id);
      return(false);
   }
   if(begin_info == NULL) {
      XLOGD_ERROR("Invalid parameters");
      return(false);
   }

   // In failure scenarios below, use the default poll time
   begin_info->when = RF4CE_DEVICE_UPDATE_IMAGE_CHECK_POLL_TIME;
   begin_info->time = g_ctrlm_device_update.prefs.check_poll_time_image;
   ctrlm_device_update_rf4ce_image_info_t *image_info = &(*g_ctrlm_device_update.rf4ce_images)[image_id];

   DEVICE_UPDATE_MUTEX_LOCK();
   // check if the CFIResponse failed to transmit on time
   if(g_ctrlm_device_update.rf4ce_sessions.count(controller_id) > 0           &&
      g_ctrlm_device_update.rf4ce_sessions[controller_id].image_id == image_id ) {
      XLOGD_WARN("Controller %d already has a session active for image %d", controller_id, image_id);
      begin_info->background_download = g_ctrlm_device_update.rf4ce_sessions[controller_id].background_download;
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return(true);
   } else if(g_ctrlm_device_update.rf4ce_sessions.count(controller_id) == 0) {
      // Attempt to acquire a session
      if(g_ctrlm_device_update.rf4ce_session_active_count >= RF4CE_SIMULTANEOUS_SESSION_QTY) {
         XLOGD_INFO("Too many sessions are in progress (%u)", g_ctrlm_device_update.rf4ce_session_active_count);
         DEVICE_UPDATE_MUTEX_UNLOCK();
         return(false);
      }
      g_ctrlm_device_update.session_count++;
      g_ctrlm_device_update.rf4ce_session_active_count++;
      g_ctrlm_device_update.rf4ce_session_count++;
      image_info->reader_count++;

      XLOGD_INFO("RF4CE Active Download Session Count %u", g_ctrlm_device_update.rf4ce_session_active_count);

      // Create the mapping from controller id to session
      g_ctrlm_device_update.rf4ce_sessions[controller_id].session_id            = (session_id_in == NULL) ? ctrlm_device_update_new_session_id() : *session_id_in;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].image_id              = image_id;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].device_name           = image_info->device_name;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].version_software      = version_software;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].version_bootloader    = version_bootloader;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].version_hardware      = version_hardware;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].interactive_download  = g_ctrlm_device_update.prefs.download.interactive;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].interactive_load      = g_ctrlm_device_update.prefs.load.interactive;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].background_download   = g_ctrlm_device_update.prefs.download.background;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].percent_increment     = g_ctrlm_device_update.prefs.download.percent_increment;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].percent_next          = 0;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_source_id     = 0;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_params        = NULL;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_value         = (timeout + 999) / 1000;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].fd                    = NULL;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].chunk_even            = NULL;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].chunk_odd             = NULL;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].offset_even           = 0;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].offset_odd            = 0;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].bytes_read_file       = 0;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].bytes_read_controller = 0;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].download_initiated    = false;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].download_in_progress  = false;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].load_initiated        = false;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].load_waiting          = false;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].time_after_inactive   = 0;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].resume_pending        = false;
      ctrlm_timestamp_get(&g_ctrlm_device_update.rf4ce_sessions[controller_id].timestamp_to_load);

      if(session_id_out != NULL) {
         *session_id_out = g_ctrlm_device_update.rf4ce_sessions[controller_id].session_id;
      }
      // Broadcast "ready to download" event
      ctrlm_device_update_iarm_event_ready_to_download(g_ctrlm_device_update.rf4ce_sessions[controller_id].session_id);
   }
   if(g_ctrlm_device_update.prefs.download.interactive && !g_ctrlm_device_update.rf4ce_sessions[controller_id].download_initiated) { // interactive, note that the device is ready and wait for download initiate from update manager service
      begin_info->when = RF4CE_DEVICE_UPDATE_IMAGE_CHECK_POLL_TIME;
      begin_info->time = g_ctrlm_device_update.prefs.check_poll_time_download;

      XLOGD_INFO("Interactive.  Wait for download initiate.");

      image_info->reader_count--;
      g_ctrlm_device_update.rf4ce_session_count--;
      if(g_ctrlm_device_update.rf4ce_session_active_count > 0) {
         g_ctrlm_device_update.rf4ce_session_active_count--;
      }
      g_ctrlm_device_update.session_count--;
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return(false);
   }

   if(!ctrlm_device_update_rf4ce_image_stage(controller_id, image_id, g_ctrlm_device_update.session_count, g_ctrlm_device_update.rf4ce_session_count, image_info->reader_count)) {
      image_info->reader_count--;
      g_ctrlm_device_update.rf4ce_session_count--;
      if(g_ctrlm_device_update.rf4ce_session_active_count > 0) {
         g_ctrlm_device_update.rf4ce_session_active_count--;
      }
      g_ctrlm_device_update.session_count--;
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return(false);
   }

   if(!session_resume) { // Start session timeout
      g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_params = (device_update_timeout_session_params_t *)g_malloc(sizeof(device_update_timeout_session_params_t));
      if(NULL == g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_params) {
         image_info->reader_count--;
         g_ctrlm_device_update.rf4ce_session_count--;
         if(g_ctrlm_device_update.rf4ce_session_active_count > 0) {
            g_ctrlm_device_update.rf4ce_session_active_count--;
         }
         g_ctrlm_device_update.session_count--;
         DEVICE_UPDATE_MUTEX_UNLOCK();
         return(false);
      }
      g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_params->network_id     = network_id;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_params->controller_id  = controller_id;
      g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_params->image_id       = image_id;

      g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_source_id = ctrlm_device_update_timeout_session_create(g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_value, g_ctrlm_device_update.rf4ce_sessions[controller_id].timeout_params);
   }

   // Override the background download flag for manual polls originating from the remote
   if(manual_poll) {
      g_ctrlm_device_update.rf4ce_sessions[controller_id].background_download = false;
   }

   // Set download rate
   if(g_ctrlm_device_update.rf4ce_sessions[controller_id].background_download) {
      begin_info->background_download = true;
   } else {
      begin_info->background_download = false;
      ctrlm_voice_t *voice_obj = ctrlm_get_voice_obj();
      if(voice_obj) {
         voice_obj->voice_device_update_in_progress_set(true);
      }
   }

   g_ctrlm_device_update.rf4ce_sessions[controller_id].download_in_progress = true;
   DEVICE_UPDATE_MUTEX_UNLOCK();
   return(true);
}

void ctrlm_device_update_rf4ce_end(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint16 image_id, ctrlm_device_update_iarm_load_result_t result) {
   if(image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
      XLOGD_INFO("Invalid Image id (%u)", image_id);
      return;
   }

   DEVICE_UPDATE_MUTEX_LOCK();

   if(g_ctrlm_device_update.rf4ce_sessions.count(controller_id) == 0) {
      XLOGD_ERROR("session not found %u", controller_id);
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return;
   }

   ctrlm_device_update_rf4ce_image_info_t *image_info   = &(*g_ctrlm_device_update.rf4ce_images)[image_id];
   ctrlm_device_update_rf4ce_session_t    *session_info = &g_ctrlm_device_update.rf4ce_sessions[controller_id];

   ctrlm_device_update_iarm_event_load_end(session_info->session_id, result);

   ctrlm_device_update_rf4ce_download_complete(session_info);

   // Decrement active count if download was still in progress
   if(false == session_info->load_waiting) {
       if(g_ctrlm_device_update.rf4ce_session_active_count > 0) {
          g_ctrlm_device_update.rf4ce_session_active_count--;
       }
       XLOGD_INFO("RF4CE Active Download Session Count %u", g_ctrlm_device_update.rf4ce_session_active_count);
   }

   image_info->reader_count--;
   g_ctrlm_device_update.rf4ce_session_count--;
   g_ctrlm_device_update.session_count--;

   ctrlm_device_update_rf4ce_image_unstage(controller_id, image_id, g_ctrlm_device_update.session_count, g_ctrlm_device_update.rf4ce_session_count, image_info->reader_count);
   DEVICE_UPDATE_MUTEX_UNLOCK();
}

void ctrlm_device_update_rf4ce_download_complete(ctrlm_device_update_rf4ce_session_t *session_info) {
   if(session_info->download_in_progress) {
      session_info->download_in_progress = false;

      if(!session_info->background_download) { // Foreground download.  Clear device update in progress if no other foreground sessions are in progress
         bool foreground_download_in_progress = false;

         // Set back to the default in case this was changed by a manual poll
         session_info->background_download = g_ctrlm_device_update.prefs.download.background;

         for(map<ctrlm_controller_id_t, ctrlm_device_update_rf4ce_session_t>::iterator it = g_ctrlm_device_update.rf4ce_sessions.begin(); it != g_ctrlm_device_update.rf4ce_sessions.end(); it++) {
            if(!it->second.background_download && it->second.download_in_progress) {
               foreground_download_in_progress = true;
               break;
            }
         }

         if(!foreground_download_in_progress) {
            ctrlm_voice_t *voice_obj = ctrlm_get_voice_obj();
            if(voice_obj) {
               voice_obj->voice_device_update_in_progress_set(false);
            }
         }
      }
   }
}

void ctrlm_device_update_rf4ce_load_info(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint16 image_id, rf4ce_device_update_load_info_t *load_info) {
   XLOGD_INFO("(%u, %u)", network_id, controller_id);
   ctrlm_device_update_rf4ce_session_t    *session_info;
   if(load_info == NULL) {
      XLOGD_ERROR("invalid parameters");
      return;
   }

   DEVICE_UPDATE_MUTEX_LOCK();
   if(g_ctrlm_device_update.rf4ce_sessions.count(controller_id) == 0) {
      XLOGD_ERROR("session not found %u", controller_id);
      load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_WHEN_INACTIVE;
      load_info->time = g_ctrlm_device_update.prefs.load.time_after_inactive;
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return;
   }
   session_info = &g_ctrlm_device_update.rf4ce_sessions[controller_id];

   ctrlm_device_update_rf4ce_download_complete(session_info);

   load_info->time = 0;
   time_t now_gmt = time(NULL);
   tm *now_local = localtime(&now_gmt);

   if(g_ctrlm_device_update.prefs.load.interactive) {
      if(!session_info->load_initiated) {
         load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_POLL_AGAIN;
         load_info->time = g_ctrlm_device_update.prefs.check_poll_time_load;  // In a minute...
         XLOGD_INFO("<%s> Load Image Interactive. Not initiated. Wait %d seconds", session_info->device_name.c_str(), load_info->time);
      } else if(ctrlm_timestamp_since_ns(session_info->timestamp_to_load) == 0){ // Time has not been reached
         load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_POLL_AGAIN;
         load_info->time = (ctrlm_timestamp_until_ms(session_info->timestamp_to_load) + 999) / 1000;  // Wait the specified time...
         XLOGD_INFO("<%s> Load Image Interactive. Wait %d seconds", session_info->device_name.c_str(), load_info->time);
      } else if(session_info->time_after_inactive > 0){
         load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_WHEN_INACTIVE;
         load_info->time = session_info->time_after_inactive;
         XLOGD_INFO("<%s> Load Image Interactive. When inactive %d seconds", session_info->device_name.c_str(), load_info->time);
         ctrlm_device_update_iarm_event_load_begin(session_info->session_id);
      } else {
         load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_NOW;
         XLOGD_INFO("<%s> Load Image Interactive. Now.", session_info->device_name.c_str());
         ctrlm_device_update_iarm_event_load_begin(session_info->session_id);
      }
   } else if(now_local->tm_hour < g_ctrlm_device_update.prefs.load.before_hour) { // the time is between midnight and load_before_hour
      if(session_info->time_after_inactive > 0) {
         load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_WHEN_INACTIVE;
         load_info->time = session_info->time_after_inactive;
         XLOGD_INFO("<%s> Load Image when inactive %d seconds", session_info->device_name.c_str(), load_info->time);
      } else {
         load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_NOW;
         XLOGD_INFO("<%s> Load Image Now. Time is %02d:%02d Load before %02d:00", session_info->device_name.c_str(), now_local->tm_hour, now_local->tm_min, g_ctrlm_device_update.prefs.load.before_hour);
      }
      ctrlm_device_update_iarm_event_load_begin(session_info->session_id);
   //} else if(0) { // Wait for inactivity period
   //   load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_WHEN_INACTIVE;
   //   load_info->time = CTRLM_DEVICE_UPDATE_RF4CE_DEFAULT_LOAD_INACTIVE_TIME;
   //} else if(0) { // Poll again after next keypress (user is present)
   //   load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_POLL_KEYPRESS;
   //} else if(0) { // Cancel the load
   //   load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_CANCEL;
   } else { // Wait until the next day
      load_info->when = RF4CE_DEVICE_UPDATE_IMAGE_LOAD_POLL_AGAIN;
      load_info->time = (24 + (g_ctrlm_device_update.prefs.load.before_hour / 2) - now_local->tm_hour) * 60 * 60;  // In the first hour after midnight...
      XLOGD_INFO("Load Image Tomorrow. Time is %02d:%02d Load before %02d:00.  Wait %d hours", now_local->tm_hour, now_local->tm_min, g_ctrlm_device_update.prefs.load.before_hour, load_info->time / 3600);
   }

   // reset timeout value to expected update time
   ctrlm_device_update_timeout_session_update(&session_info->timeout_source_id, load_info->time + CTRLM_RF4CE_CONTROLLER_FIRMWARE_UPDATE_TIMEOUT, session_info->timeout_params);

   DEVICE_UPDATE_MUTEX_UNLOCK();
}


gboolean ctrlm_device_update_timeout_session(gpointer user_data) {
   XLOGD_INFO("Session timed out.");
   device_update_timeout_session_params_t *params = (device_update_timeout_session_params_t *)user_data;
   if((NULL == params)) {
      XLOGD_ERROR("NULL parameter");
      return(false);
   }

   DEVICE_UPDATE_MUTEX_LOCK();
   if(g_ctrlm_device_update.rf4ce_sessions.count(params->controller_id) == 0) {
      XLOGD_ERROR("session not found %u", params->controller_id);
      DEVICE_UPDATE_MUTEX_UNLOCK();
      g_ctrlm_device_update.rf4ce_sessions[params->controller_id].timeout_source_id = 0;
      return(false);
   }

   ctrlm_device_update_rf4ce_end(params->network_id, params->controller_id, params->image_id, CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_TIMEOUT);
   g_ctrlm_device_update.rf4ce_sessions[params->controller_id].timeout_source_id = 0;
   DEVICE_UPDATE_MUTEX_UNLOCK();
   return(false);
};

guint ctrlm_device_update_timeout_session_create(guint timeout, gpointer param) {
   return g_timeout_add_seconds(timeout, ctrlm_device_update_timeout_session, param);
}

void ctrlm_device_update_timeout_session_update(guint *timeout_source_id, gint timeout, gpointer param) {
   if(NULL != timeout_source_id && 0 != *timeout_source_id) {
      g_source_remove(*timeout_source_id);
      *timeout_source_id = ctrlm_device_update_timeout_session_create(timeout, param);
   }
}

void ctrlm_device_update_timeout_session_destroy(guint *timeout_source_id) {
   if(NULL != timeout_source_id && 0 != *timeout_source_id) {
      g_source_remove(*timeout_source_id);
      *timeout_source_id = 0;
   }
}

void ctrlm_device_update_rf4ce_notify_reboot(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, gboolean session_resume) {
   DEVICE_UPDATE_MUTEX_LOCK();
   if(g_ctrlm_device_update.rf4ce_sessions.count(controller_id) != 0) {
      if(session_resume) {
         XLOGD_INFO("session is in progress %u with resume support", controller_id);
      } else {
         XLOGD_ERROR("session is in progress %u", controller_id);
         ctrlm_device_update_rf4ce_end(network_id, controller_id, g_ctrlm_device_update.rf4ce_sessions[controller_id].image_id, CTRLM_DEVICE_UPDATE_IARM_LOAD_RESULT_ERROR_TIMEOUT);
      }
   }
   DEVICE_UPDATE_MUTEX_UNLOCK();
}

gboolean ctrlm_device_update_session_get_by_id(ctrlm_device_update_session_id_t session_id, ctrlm_device_update_iarm_call_session_t *session) {
   if(session == NULL) {
      return(false);
   }
   DEVICE_UPDATE_MUTEX_LOCK();
   // Locate the session
   for(map<ctrlm_controller_id_t, ctrlm_device_update_rf4ce_session_t>::iterator it = g_ctrlm_device_update.rf4ce_sessions.begin(); it != g_ctrlm_device_update.rf4ce_sessions.end(); it++) {
      if(it->second.session_id == session_id) {
         if(it->second.image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
            continue;
         }
         ctrlm_device_update_device_get_from_session(&it->second, &session->device);

         session->image_id             = it->second.image_id;
         session->interactive_download = it->second.interactive_download;
         session->interactive_load     = it->second.interactive_load;
         session->download_percent     = (it->second.bytes_read_controller * 100) / (*g_ctrlm_device_update.rf4ce_images)[it->second.image_id].size;
         session->load_complete        = false;
         session->error_code           = 0;
         DEVICE_UPDATE_MUTEX_UNLOCK();
         return(true);
      }
   }
   DEVICE_UPDATE_MUTEX_UNLOCK();
   return(false);
}

gboolean ctrlm_device_update_image_device_get_by_id(ctrlm_device_update_session_id_t session_id, ctrlm_device_update_image_id_t *image_id, ctrlm_device_update_device_t *device) {
   if(image_id == NULL || device == NULL) {
      return(false);
   }
   // Locate the session
   map<ctrlm_controller_id_t, ctrlm_device_update_rf4ce_session_t>::iterator it;
   for(it = g_ctrlm_device_update.rf4ce_sessions.begin(); it != g_ctrlm_device_update.rf4ce_sessions.end(); it++) {
      if(it->second.session_id == session_id) {
         if(it->second.image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
            continue;
         }
         *image_id = it->second.image_id;
         ctrlm_device_update_device_get_from_session(&it->second, device);
         return(true);
      }
   }
   return(false);
}

void ctrlm_device_update_device_get_from_session(ctrlm_device_update_rf4ce_session_t *session, ctrlm_device_update_device_t *device) {
   errno_t safec_rc = -1;
   safec_rc = strncpy_s(device->name, sizeof(device->name), session->device_name.c_str(), CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH - 1);
   ERR_CHK(safec_rc);
   safec_rc = sprintf_s(device->version_software,   CTRLM_DEVICE_UPDATE_VERSION_LENGTH, "%u.%u.%u.%u",  session->version_software.major,        session->version_software.minor,   session->version_software.revision,    session->version_software.patch);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
   safec_rc = sprintf_s(device->version_hardware,   CTRLM_DEVICE_UPDATE_VERSION_LENGTH, "%u.%u.%u.%u",  session->version_hardware.manufacturer, session->version_hardware.model,   session->version_hardware.hw_revision, session->version_hardware.lot_code);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
   safec_rc = sprintf_s(device->version_bootloader, CTRLM_DEVICE_UPDATE_VERSION_LENGTH, "%u.%u.%u.%u",  session->version_bootloader.major,      session->version_bootloader.minor, session->version_bootloader.revision,  session->version_bootloader.patch);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
   device->name[CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH - 1]           = '\0';
}

gboolean ctrlm_device_update_image_get_by_id(ctrlm_device_update_image_id_t image_id, ctrlm_device_update_image_t *image) {
   errno_t safec_rc = -1;
   DEVICE_UPDATE_MUTEX_LOCK();

   if(image_id >= g_ctrlm_device_update.rf4ce_images->size() || image == NULL) {
      DEVICE_UPDATE_MUTEX_UNLOCK();
      return(false);
   }
   ctrlm_device_update_rf4ce_image_info_t *image_info = &(*g_ctrlm_device_update.rf4ce_images)[image_id];

   switch(image_info->image_type) {
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_FIRMWARE:     image->image_type = CTRLM_DEVICE_UPDATE_IMAGE_TYPE_FIRMWARE;   break;
      case RF4CE_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA_1: image->image_type = CTRLM_DEVICE_UPDATE_IMAGE_TYPE_AUDIO_DATA; break;
      default:                                          image->image_type = CTRLM_DEVICE_UPDATE_IMAGE_TYPE_OTHER;      break;
   }
   image->force_update = image_info->force_update;
   image->image_size = image_info->size;
   safec_rc = strncpy_s(image->device_name,  sizeof(image->device_name),  image_info->device_name.c_str(), CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH - 1);
   ERR_CHK(safec_rc);
   safec_rc = strcpy_s(image->device_class, sizeof(image->device_class), "remotes");
   ERR_CHK(safec_rc);
   safec_rc = sprintf_s(image->image_version, CTRLM_DEVICE_UPDATE_VERSION_LENGTH, "%u.%u.%u.%u",  image_info->version_software.major, image_info->version_software.minor, image_info->version_software.revision, image_info->version_software.patch);
   if(safec_rc < EOK) {
      ERR_CHK(safec_rc);
   }
   safec_rc = strncpy_s(image->image_file_path, sizeof(image->image_file_path), image_info->file_path_archive.c_str(), CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH - 1);
   ERR_CHK(safec_rc);

   image->device_name[CTRLM_DEVICE_UPDATE_DEVICE_NAME_LENGTH - 1]  = '\0';

   image->image_file_path[CTRLM_DEVICE_UPDATE_PATH_LENGTH - 1]     = '\0';

   DEVICE_UPDATE_MUTEX_UNLOCK();

   return(true);
}

gboolean ctrlm_device_update_status_info_get(ctrlm_device_update_iarm_call_status_t *status) {
   if(status == NULL) {
      return(false);
   }
   guint32 session_qty = 0;

   DEVICE_UPDATE_MUTEX_LOCK();

   map<ctrlm_controller_id_t, ctrlm_device_update_rf4ce_session_t>::iterator it;
   for(it = g_ctrlm_device_update.rf4ce_sessions.begin(); it != g_ctrlm_device_update.rf4ce_sessions.end(); it++) {
      status->session_ids[session_qty] = it->second.session_id;
      session_qty++;
      if(session_qty >= CTRLM_DEVICE_UPDATE_MAX_SESSIONS) {
         break;
      }
   }

   status->session_qty                = session_qty;
   status->image_qty                  = g_ctrlm_device_update.rf4ce_images->size();
   if(status->image_qty >= CTRLM_DEVICE_UPDATE_MAX_IMAGES) {
      status->image_qty = CTRLM_DEVICE_UPDATE_MAX_IMAGES;
   }
   for(unsigned long index = 0; index < status->image_qty; index++) {
      status->image_ids[index] = index;
   }
   status->interactive_download = g_ctrlm_device_update.prefs.download.interactive;
   status->interactive_load     = g_ctrlm_device_update.prefs.load.interactive;
   status->percent_increment    = g_ctrlm_device_update.prefs.download.percent_increment;
   status->load_immediately     = g_ctrlm_device_update.prefs.download.load_immediately;
   status->running              = g_ctrlm_device_update.running;

   DEVICE_UPDATE_MUTEX_UNLOCK();
   return(true);
}

gboolean ctrlm_device_update_interactive_download_start(ctrlm_device_update_session_id_t session_id, gboolean background, guchar percent_increment, gboolean load_immediately) {
   if(!g_ctrlm_device_update.prefs.download.interactive) {
      XLOGD_WARN("Interactive Download is Disabled");
      return(false);
   }
   DEVICE_UPDATE_MUTEX_LOCK();
   // Locate the session
   map<ctrlm_controller_id_t, ctrlm_device_update_rf4ce_session_t>::iterator it;
   for(it = g_ctrlm_device_update.rf4ce_sessions.begin(); it != g_ctrlm_device_update.rf4ce_sessions.end(); it++) {
      if(it->second.session_id == session_id) {
         // Kick off the download
         it->second.download_initiated = true;
         if(percent_increment == 0) {
            it->second.percent_increment = 100;
            it->second.percent_next      = 0;
         } else if(percent_increment < 100) {
            it->second.percent_increment = percent_increment;
            it->second.percent_next      = 0;
         }

         if(background) {
            it->second.background_download = true;
         } else {
            it->second.background_download = false;
         }

         it->second.interactive_load = true;
         if(load_immediately) {
            it->second.load_initiated      = true;
            ctrlm_timestamp_get(&it->second.timestamp_to_load);
            it->second.time_after_inactive = 0;
         }

         DEVICE_UPDATE_MUTEX_UNLOCK();
         return(true);
      }
   }
   DEVICE_UPDATE_MUTEX_UNLOCK();
   return(false);
}

gboolean ctrlm_device_update_interactive_load_start(ctrlm_device_update_session_id_t session_id, ctrlm_device_update_iarm_load_type_t load_type, guint32 time_to_load, guint32 time_after_inactive) {
   if(!g_ctrlm_device_update.prefs.load.interactive) {
      XLOGD_WARN("Interactive Load is Disabled");
      return(false);
   }
   if(load_type >= CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_MAX) {
      XLOGD_ERROR("Invalid Load Type <%s>", ctrlm_device_update_iarm_load_type_str(load_type));
      return(false);
   }
   DEVICE_UPDATE_MUTEX_LOCK();
   // Locate the session
   map<ctrlm_controller_id_t, ctrlm_device_update_rf4ce_session_t>::iterator it;
   for(it = g_ctrlm_device_update.rf4ce_sessions.begin(); it != g_ctrlm_device_update.rf4ce_sessions.end(); it++) {
      if(it->second.session_id == session_id) {
         // Kick off the load
         it->second.load_initiated = true;
         switch(load_type) {
            case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_DEFAULT: {
               time_t now_gmt = time(NULL);
               tm *now_local = localtime(&now_gmt);
               ctrlm_timestamp_get(&it->second.timestamp_to_load);
               if(now_local->tm_hour < g_ctrlm_device_update.prefs.load.before_hour) {
                  XLOGD_INFO("Load Image Now. Time is %02d:%02d Load before %02d:00", now_local->tm_hour, now_local->tm_min, g_ctrlm_device_update.prefs.load.before_hour);
               } else {
                  time_to_load = (24 + (g_ctrlm_device_update.prefs.load.before_hour / 2) - now_local->tm_hour) * 60 * 60;  // In the first hour after midnight...
                  XLOGD_INFO("Load Image Tomorrow. Time is %02d:%02d Load before %02d:00.  Wait %d hours", now_local->tm_hour, now_local->tm_min, g_ctrlm_device_update.prefs.load.before_hour, time_to_load / 3600);
                  ctrlm_timestamp_add_secs(&it->second.timestamp_to_load, time_to_load);
               }
               break;
            }
            case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_NORMAL: {
               ctrlm_timestamp_get(&it->second.timestamp_to_load);
               ctrlm_timestamp_add_secs(&it->second.timestamp_to_load, time_to_load);
               it->second.time_after_inactive = time_after_inactive;
               break;
            }
            case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_POLL:
            case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_ABORT:
            case CTRLM_DEVICE_UPDATE_IARM_LOAD_TYPE_MAX: {
               // TODO
               break;
            }
         }
         DEVICE_UPDATE_MUTEX_UNLOCK();
         return(true);
      }
   }
   DEVICE_UPDATE_MUTEX_UNLOCK();
   return(false);
}

ctrlm_device_update_session_id_t ctrlm_device_update_new_session_id() {
   // Increment the global session ID
   g_ctrlm_device_update.session_id += 1;
 
   // Write new session ID to NVM
   ctrlm_db_device_update_session_id_write(g_ctrlm_device_update.session_id); 

   // Return new session ID
   return g_ctrlm_device_update.session_id;
}

gboolean ctrlm_device_update_is_controller_updating(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, bool ignore_load_waiting) {
   if(CTRLM_NETWORK_TYPE_RF4CE == ctrlm_network_type_get(network_id)) {
      if(g_ctrlm_device_update.rf4ce_sessions.count(controller_id) > 0) {
         if(ignore_load_waiting || !g_ctrlm_device_update.rf4ce_sessions[controller_id].load_waiting) {
            XLOGD_INFO("< %u, %u > Download in progress", network_id, controller_id);
            return true;
         }
      }
   }
      return false;
}


void ctrlm_device_update_check_for_update_file_delete(ctrlm_device_update_image_id_t image_id, ctrlm_controller_id_t controller_id, ctrlm_network_id_t network_id){
   if(image_id >= g_ctrlm_device_update.rf4ce_images->size()) {
      XLOGD_WARN("Controller id %u Image not found %u.  Ignoring.", controller_id, image_id);
      return;
   }
   // download is complete so we need to check whether the current update file is still needed
   // or if it should be deleted.  Send a message to main and ask if there are other controllers of this
   // type that need the upgrade.
   XLOGD_INFO("download is complete checking to delete update file for image_id %d",image_id);

   ctrlm_device_update_rf4ce_image_info_t *image_info   = &(*g_ctrlm_device_update.rf4ce_images)[image_id];
   XLOGD_INFO("checking delete file with  %d",image_info->controller_type);

   // Allocate a message and send it to Control Manager's queue
   ctrlm_main_queue_msg_update_file_check_t *msg = (ctrlm_main_queue_msg_update_file_check_t *) g_malloc(sizeof(ctrlm_main_queue_msg_update_file_check_t));

   if (NULL == msg) {
      XLOGD_FATAL("Out of memory");
      g_assert(0);
      return;
   }

   msg->header.type = CTRLM_MAIN_QUEUE_MSG_TYPE_CHECK_UPDATE_FILE_NEEDED;
   // I dont know if these values are gotten correctly so noting them
   // for review.
   msg->controller_id=controller_id;
   msg->header.network_id = network_id;
   XLOGD_INFO("checking delete controller type %d",image_info->controller_type);
   msg->controller_type = image_info->controller_type;
   XLOGD_INFO("checking delete image type %d",image_info->image_type);
   msg->update_type=image_info->image_type;
   XLOGD_INFO("checking delete file archive %s",image_info->file_path_archive.c_str());
   errno_t safec_rc = strncpy_s(msg->file_path_archive,sizeof(msg->file_path_archive), image_info->file_path_archive.c_str(),sizeof(msg->file_path_archive) - 1);
   ERR_CHK(safec_rc);
   msg->update_version=image_info->version_software;
   XLOGD_INFO("sending update file check msg for %s",image_info->file_path_archive.c_str());

   ctrlm_main_queue_msg_push(msg);

}

void ctrlm_device_update_timeout_update_activity(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, guint timeout_value) {
   if(ctrlm_device_update_is_controller_updating(network_id, controller_id, false)) {
      ctrlm_device_update_rf4ce_session_t *session = &g_ctrlm_device_update.rf4ce_sessions[controller_id];

      //If no timeout value was given then use session timeout value
      timeout_value = (timeout_value == CTRLM_DEVICE_UPDATE_USE_DEFAULT_TIMEOUT) ? (session->timeout_value) : (timeout_value);

      if(session->download_in_progress && !session->load_waiting) { // We want to make sure the remote is actively downloading
         XLOGD_DEBUG("Remote is currently updating, we will update timeout with %d seconds.", timeout_value);
         ctrlm_device_update_timeout_session_update(&session->timeout_source_id, timeout_value, session->timeout_params);
      }
   }
}

string ctrlm_device_update_get_software_version(guint16 image_id){
   char sw_version[DEVICE_UPDATE_SOFTWARE_VERSION_LENGTH] = {'\0'};
   if(image_id < g_ctrlm_device_update.rf4ce_images->size()){
      ctrlm_device_update_rf4ce_image_info_t *image_info = &(*g_ctrlm_device_update.rf4ce_images)[image_id];
      version_software_t version = image_info->version_software;
      errno_t safec_rc = sprintf_s(sw_version, DEVICE_UPDATE_SOFTWARE_VERSION_LENGTH, "<%u.%u.%u.%u>", version.major, version.minor, version.revision, version.patch);
      if(safec_rc < EOK) {
         ERR_CHK(safec_rc);
      }
   }

   return string(sw_version);
}

#ifdef XR15_704
gboolean ctrlm_device_update_xr15_crash_update_get() {
   return g_ctrlm_device_update.xr15_crash_update;
}
#endif

void ctrlm_device_update_rf4ce_session_resume(vector<rf4ce_device_update_session_resume_info_t> *sessions) {
   // Store sessions vector
   g_ctrlm_device_update.sessions = sessions;

   // Check for download sessions to continue. if ctrlmgr restarted process local file in filesystem since xconf won't call iarmbus api
   ctrlm_device_update_rf4ce_session_resume_check(g_ctrlm_device_update.sessions, !ctrlm_is_recently_booted());
}

void ctrlm_device_update_rf4ce_session_resume_check(vector<rf4ce_device_update_session_resume_info_t> *sessions, bool process_local_files) {
   if(sessions == NULL) {
      return;
   }

   for(vector<rf4ce_device_update_session_resume_info_t>::iterator it = sessions->begin(); it != sessions->end(); ) {
      gboolean ready_to_download = false;

      if(process_local_files) {
         XLOGD_INFO("process local image file <%s>", it->state.file_name);
         ctrlm_device_update_process_device_file(it->state.file_name,"", &it->state.image_id);
      }

      rf4ce_device_update_image_info_t image_info;
      gboolean image_available = ctrlm_device_update_rf4ce_is_image_available(it->state.image_type, it->type, it->version_hardware, it->version_bootloader, it->version_software, RF4CE_DEVICE_UPDATE_AUDIO_THEME_INVALID, &image_info, it->type_z);
      if(image_available) {
         rf4ce_device_update_begin_info_t begin_info;
         begin_info.when                = (rf4ce_device_update_image_check_when_t)it->state.when;
         begin_info.time                = it->state.time;
         begin_info.background_download = it->state.background_download;

         ready_to_download = ctrlm_device_update_rf4ce_begin(it->network_id, it->controller_id, it->version_hardware, it->version_bootloader, it->version_software, image_info.id, it->timeout, it->state.manual_poll, &begin_info, &it->state.session_id, NULL, true);
      }
      XLOGD_INFO("session id <%u> image id <%u> <%s> <%s>", it->state.session_id, it->state.image_id, image_available ? "AVAILABLE" : "NOT AVAILABLE", ready_to_download ? "READY TO DOWNLOAD" : "NOT READY TO DOWNLOAD");

      // Delete vector entry if download was started
      if(ready_to_download) {
         it = sessions->erase(it);
      } else {
         it++;
      }
   }

   // Free vector if there are no more entries
   if(g_ctrlm_device_update.sessions->size() == 0) {
      delete g_ctrlm_device_update.sessions;
      g_ctrlm_device_update.sessions = NULL;
   }
}
