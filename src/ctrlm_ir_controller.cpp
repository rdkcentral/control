/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2014 RDK Management
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
// Defines

// Includes

#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_ir_controller.h"
#include "ctrlm_utils.h"
#include "ctrlm_database.h"
#include "ctrlm_config_types.h"
#include "ctrlm_config_default.h"
#include "ctrlm_network.h"

#include <fcntl.h>

#include <libevdev-1.0/libevdev/libevdev.h>

using namespace std;



static void* ctrlm_ir_key_monitor_thread(void *data);
static gboolean ctrlm_ir_retry_input_open(gpointer user_data);
static int ctrlm_ir_open_key_input_device(string name);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum {
    CTRLM_IR_KEY_QUEUE_MSG_TYPE_BUMP,
    CTRLM_IR_KEY_QUEUE_MSG_TYPE_THREAD_POLL,
    CTRLM_IR_KEY_QUEUE_MSG_TYPE_TERMINATE
} ctrlm_ir_key_queue_msg_type_t;

typedef struct {
   ctrlm_ir_key_queue_msg_type_t type;
} ctrlm_ir_key_queue_msg_header_t;

typedef struct {
   ctrlm_ir_key_queue_msg_header_t hdr;
   ctrlm_thread_monitor_response_t *response;
} ctrlm_ir_key_queue_msg_thread_poll_t;

#define CTRLM_IR_KEY_MSG_QUEUE_MSG_MAX         (10)
#define CTRLM_IR_KEY_MSG_QUEUE_MSG_SIZE_MAX    (sizeof(ctrlm_ir_key_queue_msg_thread_poll_t))

#define KEY_INPUT_DEVICE_BASE_DIR    "/dev/input/"
#define KEY_INPUT_DEVICE_BASE_FILE   "event"

static guint g_retry_input_open_timer_tag = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static ctrlm_ir_controller_t *_instance = NULL;

ctrlm_ir_controller_t* ctrlm_ir_controller_t::get_instance() {
    if (_instance == NULL) {
        _instance = new ctrlm_ir_controller_t();
    }
    return(_instance);
}

void ctrlm_ir_controller_t::destroy_instance() {
    if (_instance != NULL) {
        delete _instance;
        _instance = NULL;
    }
}

ctrlm_ir_controller_t::ctrlm_ir_controller_t()
   : input_device_name_(JSON_STR_VALUE_IR_INPUT_DEVICE_NAME)
   , last_key_time_(std::make_shared<ctrlm_uint64_db_attr_t>("Last Keypress Time", 0, "", "last_key_time"))
   , last_key_code_(std::make_shared<ctrlm_uint64_db_attr_t>("Last Keypress Code", CTRLM_KEY_CODE_INVALID, "", "last_key_code"))
   , last_key_time_flush_(0)
   , mask_key_codes_(true)
   , key_thread_msgq_(XR_MQ_INVALID)
   , scan_code_(0)
{

   key_thread_.running = false;

   read_config();

   if (input_device_name_.empty()) {
      XLOGD_WARN("IR input device name is empty, not starting key monitor thread...");
   } else {

      // Create an asynchronous queue to receive incoming messages
      if (false == ctrlm_utils_message_queue_open(&key_thread_msgq_, CTRLM_IR_KEY_MSG_QUEUE_MSG_MAX, CTRLM_IR_KEY_MSG_QUEUE_MSG_SIZE_MAX)) {
         XLOGD_ERROR("failed to create message queue to key monitor thread");
      } else {
         XLOGD_INFO("Starting key monitor thread...");
         key_thread_.name = "ctrlm_ir_key_mon";
         ctrlm_utils_thread_create(&key_thread_, ctrlm_ir_key_monitor_thread, this);
      }
   }
}

ctrlm_ir_controller_t::~ctrlm_ir_controller_t() {
   XLOGD_INFO("destructor");

   if (key_thread_.running) {
      ctrlm_ir_key_queue_msg_header_t msg;
      msg.type = CTRLM_IR_KEY_QUEUE_MSG_TYPE_TERMINATE;
      ctrlm_utils_queue_msg_push(key_thread_msgq_, (const char *)&msg, sizeof(msg));
      ctrlm_utils_thread_join(&key_thread_, 2);
   }
   ctrlm_timeout_destroy(&g_retry_input_open_timer_tag);
   ctrlm_utils_message_queue_close(&key_thread_msgq_);
}


bool ctrlm_ir_controller_t::read_config() {
   bool ret = false;
   ctrlm_config_string_t name("ir.input_device_name");
   if(name.get_config_value(this->input_device_name_)) {
      XLOGD_INFO("IR input device name from config file: <%s>", this->input_device_name_.c_str());
      ret = true;
   } else {
      XLOGD_WARN("Failed to read from config, using IR input device name default: <%s>", this->input_device_name_.c_str());
   }
   return(ret);
}

std::string ctrlm_ir_controller_t::name_get(void) {
   return "INFRARED_CONTROLLER";
}

std::string ctrlm_ir_controller_t::input_device_name_get(void) {
   return input_device_name_;
}

xr_mq_t ctrlm_ir_controller_t::key_thread_msgq_get() const {
   return  key_thread_msgq_;
}

void ctrlm_ir_controller_t::mask_key_codes_set(bool mask_key_codes) {
   mask_key_codes_ = mask_key_codes;
}
bool ctrlm_ir_controller_t::mask_key_codes_get() const {
   return(mask_key_codes_);
}

time_t ctrlm_ir_controller_t::last_key_time_get() {
   return (time_t)(last_key_time_->get_value());
}

void ctrlm_ir_controller_t::last_key_time_set(time_t val) {
   last_key_time_->set_value(val);
}

uint16_t ctrlm_ir_controller_t::last_key_code_get() {
   return (uint16_t)(last_key_code_->get_value());
}

void ctrlm_ir_controller_t::last_key_code_set(uint16_t val) {
   last_key_code_->set_value(val);
}

void ctrlm_ir_controller_t::last_key_time_update() {
   last_key_time_->set_value((uint64_t)time(NULL));

   if(this->last_key_time_get() > last_key_time_flush_) {
      last_key_time_flush_ = this->last_key_time_get() + LAST_KEY_DATABASE_FLUSH_INTERVAL;
      ctrlm_db_attr_write(last_key_time_);
      ctrlm_db_attr_write(last_key_code_);
   }
}

void ctrlm_ir_controller_t::process_event_key(uint16_t key_code) {
   last_key_code_->set_value((uint64_t)key_code);
   last_key_time_update();

   if (key_code == KEY_BLUETOOTH || key_code == KEY_CONNECT) {
      const uint8_t commandCode = (scan_code_ >> 0) & 0xff;
      XLOGD_INFO("received IR key for BLE pairing (scan code 0x%06x : pairCode=%hhu)", scan_code_, commandCode);

      ctrlm_iarm_call_StartPairWithCode_params_t params;
      params.key_code = key_code;
      params.pair_code = commandCode;

      ctrlm_main_queue_msg_pair_with_code_t msg;
      errno_t safec_rc = memset_s(&msg, sizeof(msg), 0, sizeof(msg));
      ERR_CHK(safec_rc);
      msg.params = &params;

      ctrlm_main_queue_handler_push(CTRLM_HANDLER_NETWORK, (ctrlm_msg_handler_network_t)&ctrlm_obj_network_t::req_process_pair_with_code, &msg, sizeof(msg), NULL, CTRLM_MAIN_NETWORK_ID_ALL, true);
   }
}

void ctrlm_ir_controller_t::db_table_set(const std::string &table) {
   last_key_time_->set_table(table);
   last_key_code_->set_table(table);
}

void ctrlm_ir_controller_t::db_load() {
   ctrlm_db_attr_read(last_key_time_.get());
   ctrlm_db_attr_read(last_key_code_.get());
}

void ctrlm_ir_controller_t::db_store() {
   ctrlm_db_attr_write(last_key_time_);
   ctrlm_db_attr_write(last_key_code_);
}

void ctrlm_ir_controller_t::set_scan_code(int code) {
   scan_code_ = code;
}


void ctrlm_ir_controller_t::print_status() {
   XLOGD_WARN("------------------------------------------------------------");
   XLOGD_INFO("Controller                   : %s", name_get().c_str());
   XLOGD_INFO("");
   XLOGD_INFO("Last Key Code                : %u (%s key)", this->last_key_code_get(), ctrlm_linux_key_code_str(this->last_key_code_get(), false));
   XLOGD_INFO("Last Key Time                : %s", ctrlm_utils_time_as_string(this->last_key_time_get()).c_str());
   XLOGD_WARN("------------------------------------------------------------");
}

void ctrlm_ir_controller_t::thread_poll(void *data) {
   if(data == NULL) {
      XLOGD_ERROR("invalid params");
      return;
   }
   if (!key_thread_.running) {
      XLOGD_DEBUG("IR key thread is not running at all, returning success");
      ctrlm_thread_monitor_response_t *response = (ctrlm_thread_monitor_response_t *)data;
      *response = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
      return;
   }

   ctrlm_ir_key_queue_msg_thread_poll_t msg;
   msg.hdr.type = CTRLM_IR_KEY_QUEUE_MSG_TYPE_THREAD_POLL;
   msg.response = (ctrlm_thread_monitor_response_t*)data;
   ctrlm_utils_queue_msg_push(key_thread_msgq_, (const char *)&msg, sizeof(msg));
}

static int ctrlm_ir_open_key_input_device(string name) {
   int fd = -1;
   
   XLOGD_INFO("Searching for IR input device named <%s>", name.c_str());

   if (!name.empty()) {

      string keyInputBaseDir(KEY_INPUT_DEVICE_BASE_DIR);
      DIR *dir_p = opendir(keyInputBaseDir.c_str());
      if (NULL == dir_p) {
         int errsv = errno;
         XLOGD_ERROR("Failed to open key input device dir at path <%s>: error = <%d>, <%s>", 
               keyInputBaseDir.c_str(), errsv, strerror(errsv));
         return -1;
      }

      dirent *file_p;
      while ((file_p = readdir(dir_p)) != NULL) {
         if (strstr(file_p->d_name, KEY_INPUT_DEVICE_BASE_FILE) != NULL) {
            //this is one of the event devices, open it and see if it belongs to this MAC
            string keyInputFilename = keyInputBaseDir + file_p->d_name;
            int input_fd = open(keyInputFilename.c_str(), O_RDONLY|O_NONBLOCK);
            if (input_fd < 0) {
               int errsv = errno;
               XLOGD_WARN("Failed to open key input device at path <%s>: error = <%d>, <%s>", 
                     keyInputFilename.c_str(), errsv, strerror(errsv));
            } else {
               struct libevdev *evdev = NULL;   
               int rc = libevdev_new_from_fd(input_fd, &evdev);
               if (rc < 0) {
                  XLOGD_ERROR("Failed to init libevdev (%s)", strerror(-rc));   //on failure, rc is negative errno
               } else {
                  XLOGD_INFO("Input device <%s> name: <%s> ID: bus %#x vendor %#x product %#x, phys = <%s>, unique = <%s>",
                        keyInputFilename.c_str(),
                        libevdev_get_name(evdev),libevdev_get_id_bustype(evdev),libevdev_get_id_vendor(evdev),
                        libevdev_get_id_product(evdev),libevdev_get_phys(evdev),libevdev_get_uniq(evdev));
               
                  if (name.compare(libevdev_get_name(evdev)) == 0) {
                     libevdev_free(evdev);
                     evdev = NULL;
                     closedir(dir_p);
                     XLOGD_INFO("Successfully opened IR input device <%s> from NAME <%s>", keyInputFilename.c_str(), name.c_str());
                     return input_fd;
                  }
               }
               close(input_fd);
               if (NULL != evdev) {
                  libevdev_free(evdev);
                  evdev = NULL;
               }
            }
         }
      }
      closedir(dir_p);
   }
   return fd;
}

static gboolean ctrlm_ir_retry_input_open(gpointer user_data) {
   xr_mq_t* mq =  (xr_mq_t*) user_data;

   if (XR_MQ_INVALID != *mq) {
      ctrlm_ir_key_queue_msg_header_t msg;
      msg.type = CTRLM_IR_KEY_QUEUE_MSG_TYPE_BUMP;
      ctrlm_utils_queue_msg_push(*mq, (const char *)&msg, sizeof(msg));
   }
   g_retry_input_open_timer_tag = 0;
   return false;     //This is not a recurring timer, kill it by returning false
}

void* ctrlm_ir_key_monitor_thread(void *data) {
   XLOGD_INFO("Enter...");

   ctrlm_ir_controller_t *ir_controller = (ctrlm_ir_controller_t *)data;

   struct input_event event;
   errno_t safec_rc = -1;
   int input_device_fd = -1;
   int input_device_retry_cnt = 0;
   int input_device_retry_max = 10;
   
   fd_set rfds;
   int nfds = -1;
   bool running = true;
   char msg[CTRLM_IR_KEY_MSG_QUEUE_MSG_SIZE_MAX];
   xr_mq_t msgq = ir_controller->key_thread_msgq_get();

   string input_device_name = ir_controller->input_device_name_get();

   do {
      if (input_device_fd < 0) {
         if (0 > (input_device_fd = ctrlm_ir_open_key_input_device(input_device_name))) {
            // failed to open the key input device, maybe ctrlm started before the device was created.  Wait a bit and try again.
            if (input_device_retry_cnt < input_device_retry_max) {
               // retry a couple times in case the IR input device hasn't been created yet
               input_device_retry_cnt++;
               ctrlm_timeout_destroy(&g_retry_input_open_timer_tag);
               g_retry_input_open_timer_tag = ctrlm_timeout_create(5 * 1000, ctrlm_ir_retry_input_open, &msgq);
            } else {
               XLOGD_FATAL("Failed to find IR input device and reached max number of retries, remote pairing might not work!");
            }
         }
      }

      // Needs to be reinitialized before each call to select() because select() will modify these variables
      FD_ZERO(&rfds);
      FD_SET(msgq, &rfds);
      if (input_device_fd >= 0) {
         FD_SET(input_device_fd, &rfds);
      }
      nfds = msgq;
      nfds = MAX(nfds, input_device_fd);
      nfds++;

      int ret = select(nfds, &rfds, NULL, NULL, NULL);
      if (ret < 0) {
         int errsv = errno;
         XLOGD_DEBUG("select() failed: error = <%d>, <%s>", errsv, strerror(errsv));
         continue;
      }

      if(FD_ISSET(msgq, &rfds)) {
         ssize_t bytes_read = xr_mq_pop(msgq, msg, sizeof(msg));
         if(bytes_read <= 0) {
            XLOGD_ERROR("mq_receive failed, rc <%d>", bytes_read);
         } else {
            ctrlm_ir_key_queue_msg_header_t *hdr = (ctrlm_ir_key_queue_msg_header_t *) msg;
            switch(hdr->type) {
               case CTRLM_IR_KEY_QUEUE_MSG_TYPE_BUMP: {
                  XLOGD_DEBUG("message type CTRLM_IR_KEY_QUEUE_MSG_TYPE_BUMP");
                  break;
               }
               case CTRLM_IR_KEY_QUEUE_MSG_TYPE_THREAD_POLL: {
                  XLOGD_DEBUG("message type CTRLM_IR_KEY_QUEUE_MSG_TYPE_THREAD_POLL");
                  ctrlm_ir_key_queue_msg_thread_poll_t *thread_poll = (ctrlm_ir_key_queue_msg_thread_poll_t *) msg;
                  *thread_poll->response = CTRLM_THREAD_MONITOR_RESPONSE_ALIVE;
                  break;
               }
               case CTRLM_IR_KEY_QUEUE_MSG_TYPE_TERMINATE: {
                  XLOGD_DEBUG("message type CTRLM_IR_KEY_QUEUE_MSG_TYPE_TERMINATE");
                  running = false;
                  break;
               }
               default: {
                  XLOGD_DEBUG("Unknown message type %u", hdr->type);
                  break;
               }
            }
         }
      }

      if (input_device_fd >= 0 && FD_ISSET(input_device_fd, &rfds)) {
         safec_rc = memset_s ((void*) &event, sizeof(event), 0, sizeof(event));
         ERR_CHK(safec_rc);
         ret = read(input_device_fd, (void*)&event, sizeof(event));
         if (ret < 0) {
            int errsv = errno;

            if (errsv == ENODEV) {
                // Receiving error ENODEV 19 "No such device" means the input device is no longer valid,
                // so close fd and re-open
                XLOGD_ERROR("error = <%d>, <%s>, closing and reopening device...", errsv, strerror(errsv));
                input_device_retry_cnt = 0;
                if (input_device_fd >= 0) {
                    close(input_device_fd);
                    input_device_fd = -1;
                }
            }
         } else {

            switch (event.type) {
               case EV_SYN:
                  ir_controller->set_scan_code(0);
                  break;

               case EV_KEY:
                  if (event.code != 0 && event.value >= 0 && event.value < 3) {

                     ctrlm_key_status_t key_status = CTRLM_KEY_STATUS_INVALID;
                     switch (event.value) {
                        case 0: { key_status = CTRLM_KEY_STATUS_UP; break; }
                        case 1: { key_status = CTRLM_KEY_STATUS_DOWN; break; }
                        case 2: { key_status = CTRLM_KEY_STATUS_REPEAT; break; }
                        default: break;
                     }
                     XLOGD_TELEMETRY("%s - code = <%d> (%s key), status = <%s>", ir_controller->name_get().c_str(),
                           ir_controller->mask_key_codes_get() ? -1 : event.code, 
                           ctrlm_linux_key_code_str(event.code, ir_controller->mask_key_codes_get()), ctrlm_key_status_str(key_status));

                     ir_controller->process_event_key(event.code);
                  }

                  ir_controller->set_scan_code(0);
                  break;

               case EV_MSC:
                  if (event.code == MSC_SCAN) {
                     ir_controller->set_scan_code(event.value);
                  }
                  break;

               default:
                  break;
            }
         }
      }
   } while (running);
   
   if (running) {
      XLOGD_ERROR("key monitor thread broke out of loop without being told, an error occurred...");
   } else {
      XLOGD_INFO("thread told to exit...");
   }

   ctrlm_timeout_destroy(&g_retry_input_open_timer_tag);

   if (input_device_fd >= 0) {
      close(input_device_fd);
   }
   return NULL;
}

