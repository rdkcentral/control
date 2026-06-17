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

#include "ctrlm_tv_audio_cap.h"
#include "ctrlm_log.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#ifdef USE_ACM
#include "libIBus.h"
#include "libIARM.h"
#include <sys/socket.h>
#include <sys/un.h>
#endif

#define PIPE_READ  (0)
#define PIPE_WRITE (1)

static ctrlm_tv_audio_cap_t *_instance = NULL;

ctrlm_tv_audio_cap_t *ctrlm_tv_audio_cap_t::get_instance() {
   if(_instance == NULL) {
      _instance = new ctrlm_tv_audio_cap_t();
   }
   return(_instance);
}

void ctrlm_tv_audio_cap_t::destroy_instance() {
   if(_instance != NULL) {
      delete _instance;
      _instance = NULL;
   }
}

ctrlm_tv_audio_cap_t::ctrlm_tv_audio_cap_t()
   :
#ifdef USE_AC_RMF
   rmf_handle_(NULL),
   rmf_default_settings_(),
#endif
#ifdef USE_ACM
   acm_session_id_(-1),
   acm_session_started_(false),
   acm_sock_fd_(-1),
#endif
   device_qty_(0),
   capturing_(false),
   capture_thread_id_(0),
   capture_thread_running_(false)
{
   pipe_fds_[PIPE_READ]  = -1;
   pipe_fds_[PIPE_WRITE] = -1;
#ifdef USE_ACM
   memset(acm_sock_path_, 0, sizeof(acm_sock_path_));
#endif
   XLOGD_INFO("Constructor");
}

ctrlm_tv_audio_cap_t::~ctrlm_tv_audio_cap_t() {
   XLOGD_INFO("Destructor");
   stop();
}

void ctrlm_tv_audio_cap_t::update_device_qty(uint32_t device_qty) {
   std::lock_guard<std::mutex> lock(mutex_);
   uint32_t prev_qty = device_qty_;
   device_qty_ = device_qty;

   if(prev_qty == 0 && device_qty_ > 0) {
      XLOGD_INFO("Mid-field device connected, starting TV audio capture");
      if(!start()) {
         XLOGD_ERROR("Failed to start TV audio capture");
      }
   } else if(prev_qty > 0 && device_qty_ == 0) {
      XLOGD_INFO("No mid-field devices remaining, stopping TV audio capture");
      stop();
   }
}

int ctrlm_tv_audio_cap_t::audio_pipe_fd_get() const {
   std::lock_guard<std::mutex> lock(mutex_);
   return pipe_fds_[PIPE_READ];
}

bool ctrlm_tv_audio_cap_t::is_capturing() const {
   std::lock_guard<std::mutex> lock(mutex_);
   return capturing_;
}

bool ctrlm_tv_audio_cap_t::start() {
   if(capturing_) {
      XLOGD_WARN("TV audio capture already active");
      return true;
   }

   // Create the pipe for delivering audio to xr-voice-sdk
   if(pipe(pipe_fds_) < 0) {
      XLOGD_ERROR("Failed to create audio pipe: %s", strerror(errno));
      return false;
   }

   // Set the write end to non-blocking to avoid stalling audio capture
   int flags = fcntl(pipe_fds_[PIPE_WRITE], F_GETFL, 0);
   if(flags >= 0) {
      fcntl(pipe_fds_[PIPE_WRITE], F_SETFL, flags | O_NONBLOCK);
   }

   XLOGD_INFO("Audio pipe created: read fd <%d> write fd <%d>", pipe_fds_[PIPE_READ], pipe_fds_[PIPE_WRITE]);

   bool success = false;

#ifdef USE_AC_RMF
   success = rmf_open();
#endif

#ifdef USE_ACM
   success = acm_open();
#endif

   if(!success) {
      close(pipe_fds_[PIPE_READ]);
      close(pipe_fds_[PIPE_WRITE]);
      pipe_fds_[PIPE_READ]  = -1;
      pipe_fds_[PIPE_WRITE] = -1;
      return false;
   }

   capturing_ = true;
   XLOGD_INFO("TV audio capture started");
   return true;
}

void ctrlm_tv_audio_cap_t::stop() {
   if(!capturing_) {
      return;
   }

   // Signal the capture thread to stop and wait for it
   if(capture_thread_running_) {
      capture_thread_running_ = false;
      pthread_join(capture_thread_id_, NULL);
      capture_thread_id_ = 0;
   }

#ifdef USE_AC_RMF
   rmf_close();
#endif

#ifdef USE_ACM
   acm_close();
#endif

   if(pipe_fds_[PIPE_WRITE] >= 0) {
      close(pipe_fds_[PIPE_WRITE]);
      pipe_fds_[PIPE_WRITE] = -1;
   }
   if(pipe_fds_[PIPE_READ] >= 0) {
      close(pipe_fds_[PIPE_READ]);
      pipe_fds_[PIPE_READ] = -1;
   }

   capturing_ = false;
   XLOGD_INFO("TV audio capture stopped");
}

void *ctrlm_tv_audio_cap_t::capture_thread_func(void *context) {
   ctrlm_tv_audio_cap_t *self = static_cast<ctrlm_tv_audio_cap_t *>(context);
   if(self != NULL) {
      self->capture_thread_run();
   }
   return NULL;
}

void ctrlm_tv_audio_cap_t::capture_thread_run() {
   XLOGD_INFO("Capture thread started");

#ifdef USE_ACM
   // Connect to the ACM unix domain socket
   if(!acm_connect_socket()) {
      XLOGD_ERROR("Capture thread: failed to connect ACM socket, exiting");
      return;
   }

   // Allocate read buffer - 100ms of 16-bit stereo @ 16kHz = 6400 bytes
   const size_t buf_size = 6400;
   uint8_t *buf = new(std::nothrow) uint8_t[buf_size];
   if(buf == NULL) {
      XLOGD_ERROR("Capture thread: failed to allocate read buffer");
      acm_disconnect_socket();
      return;
   }

   while(capture_thread_running_) {
      ssize_t bytes_read = read(acm_sock_fd_, buf, buf_size);
      if(bytes_read > 0) {
         ssize_t bytes_written = write(pipe_fds_[PIPE_WRITE], buf, (size_t)bytes_read);
         if(bytes_written < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
               XLOGD_WARN("Audio pipe full, dropping frame");
            } else {
               XLOGD_ERROR("Capture thread: pipe write failed: %s", strerror(errno));
               break;
            }
         }
      } else if(bytes_read == 0) {
         XLOGD_WARN("ACM socket closed by peer");
         break;
      } else {
         if(errno == EINTR) {
            continue;
         }
         if(errno == EAGAIN || errno == EWOULDBLOCK) {
            // Non-blocking socket, no data available yet
            usleep(2000); // 2ms
            continue;
         }
         XLOGD_ERROR("Capture thread: socket read failed: %s", strerror(errno));
         break;
      }
   }

   delete[] buf;
   acm_disconnect_socket();
#endif

   XLOGD_INFO("Capture thread exiting");
}

// =============================================================================
// RMF Audio Capture Implementation
// =============================================================================
#ifdef USE_AC_RMF
bool ctrlm_tv_audio_cap_t::rmf_open() {
   rmf_Error ret = RMF_AudioCapture_Open(&rmf_handle_);
   if(ret != RMF_SUCCESS) {
      XLOGD_ERROR("RMF_AudioCapture_Open failed: %d", ret);
      return false;
   }

   ret = RMF_AudioCapture_GetDefaultSettings(&rmf_default_settings_);
   if(ret != RMF_SUCCESS) {
      XLOGD_ERROR("RMF_AudioCapture_GetDefaultSettings failed: %d", ret);
      RMF_AudioCapture_Close(rmf_handle_);
      rmf_handle_ = NULL;
      return false;
   }

   XLOGD_INFO("RMF defaults: fifoSize <%zu> threshold <%zu>",
              rmf_default_settings_.fifoSize, rmf_default_settings_.threshold);

   // Configure capture settings
   RMF_AudioCapture_Settings settings = rmf_default_settings_;
   settings.cbBufferReady     = rmf_buffer_ready_cb;
   settings.cbBufferReadyParm = this;
   settings.cbStatusChange    = NULL;
   settings.cbStatusParm      = NULL;
   settings.format            = racFormat_e16BitStereo;
   settings.samplingFreq      = racFreq_e16000;

   ret = RMF_AudioCapture_Start(rmf_handle_, &settings);
   if(ret != RMF_SUCCESS) {
      XLOGD_ERROR("RMF_AudioCapture_Start failed: %d", ret);
      RMF_AudioCapture_Close(rmf_handle_);
      rmf_handle_ = NULL;
      return false;
   }

   XLOGD_INFO("RMF audio capture opened and started");
   return true;
}

void ctrlm_tv_audio_cap_t::rmf_close() {
   if(rmf_handle_ != NULL) {
      RMF_AudioCapture_Stop(rmf_handle_);

      // After Stop returns, no new callbacks will be generated.
      // Lock pipe_write_mutex_ to wait for any in-flight callback to finish,
      // then close the write end so no further writes are possible.
      {
         std::lock_guard<std::mutex> lock(pipe_write_mutex_);
         if(pipe_fds_[PIPE_WRITE] >= 0) {
            close(pipe_fds_[PIPE_WRITE]);
            pipe_fds_[PIPE_WRITE] = -1;
         }
      }

      RMF_AudioCapture_Close(rmf_handle_);
      rmf_handle_ = NULL;
      XLOGD_INFO("RMF audio capture closed");
   }
}

rmf_Error ctrlm_tv_audio_cap_t::rmf_buffer_ready_cb(void *context, void *buf, unsigned int size) {
   ctrlm_tv_audio_cap_t *self = static_cast<ctrlm_tv_audio_cap_t *>(context);
   if(self == NULL) {
      return RMF_ERROR;
   }

   std::lock_guard<std::mutex> lock(self->pipe_write_mutex_);
   if(self->pipe_fds_[PIPE_WRITE] < 0) {
      return RMF_SUCCESS;
   }

   const uint8_t *data = static_cast<const uint8_t *>(buf);
   unsigned int remaining = size;
   while(remaining > 0) {
      ssize_t bytes_written = write(self->pipe_fds_[PIPE_WRITE], data, remaining);
      if(bytes_written > 0) {
         data      += bytes_written;
         remaining -= (unsigned int)bytes_written;
      } else if(bytes_written < 0) {
         if(errno == EINTR) {
            continue;
         }
         if(errno == EAGAIN || errno == EWOULDBLOCK) {
            XLOGD_WARN("Audio pipe full, dropping %u bytes", remaining);
            break;
         }
         XLOGD_ERROR("RMF callback: pipe write failed: %s", strerror(errno));
         return RMF_ERROR;
      }
   }

   return RMF_SUCCESS;
}
#endif

// =============================================================================
// Audio Capture Manager (ACM) Implementation
// =============================================================================
#ifdef USE_ACM
bool ctrlm_tv_audio_cap_t::acm_open() {
   IARM_Result_t result;

   // Open ACM session with REALTIME_SOCKET output type
   audiocapturemgr::iarmbus_acm_arg_t open_param;
   memset(&open_param, 0, sizeof(open_param));
   open_param.details.arg_open.source      = 0; // primary audio source
   open_param.details.arg_open.output_type = audiocapturemgr::REALTIME_SOCKET;

   result = IARM_Bus_Call(IARMBUS_AUDIOCAPTUREMGR_NAME,
                          IARMBUS_AUDIOCAPTUREMGR_OPEN,
                          &open_param, sizeof(open_param));
   if(result != IARM_RESULT_SUCCESS || open_param.result != audiocapturemgr::ACM_RESULT_SUCCESS) {
      XLOGD_ERROR("ACM open failed: IARM result %d, ACM result %d", result, open_param.result);
      return false;
   }

   acm_session_id_ = open_param.session_id;
   XLOGD_INFO("ACM session opened: id <%d>", acm_session_id_);

   // Set audio properties
   audiocapturemgr::iarmbus_acm_arg_t props_param;
   memset(&props_param, 0, sizeof(props_param));
   props_param.session_id = acm_session_id_;
   props_param.details.arg_audio_properties.format             = audiocapturemgr::acmFormate16BitStereo;
   props_param.details.arg_audio_properties.sampling_frequency = audiocapturemgr::acmFreqe16000;

   result = IARM_Bus_Call(IARMBUS_AUDIOCAPTUREMGR_NAME,
                          IARMBUS_AUDIOCAPTUREMGR_SET_AUDIO_PROPERTIES,
                          &props_param, sizeof(props_param));
   if(result != IARM_RESULT_SUCCESS || props_param.result != audiocapturemgr::ACM_RESULT_SUCCESS) {
      XLOGD_ERROR("ACM set audio properties failed: IARM result %d, ACM result %d", result, props_param.result);
      acm_close();
      return false;
   }

   // Get output properties to retrieve the unix domain socket path
   audiocapturemgr::iarmbus_acm_arg_t output_param;
   memset(&output_param, 0, sizeof(output_param));
   output_param.session_id = acm_session_id_;

   result = IARM_Bus_Call(IARMBUS_AUDIOCAPTUREMGR_NAME,
                          IARMBUS_AUDIOCAPTUREMGR_GET_OUTPUT_PROPS,
                          &output_param, sizeof(output_param));
   if(result != IARM_RESULT_SUCCESS || output_param.result != audiocapturemgr::ACM_RESULT_SUCCESS) {
      XLOGD_ERROR("ACM get output props failed: IARM result %d, ACM result %d", result, output_param.result);
      acm_close();
      return false;
   }

   memset(acm_sock_path_, 0, sizeof(acm_sock_path_));
   strncpy(acm_sock_path_, output_param.details.arg_output_props.output.file_path, sizeof(acm_sock_path_) - 1);
   XLOGD_INFO("ACM socket path: %s", acm_sock_path_);

   // Start the ACM session
   audiocapturemgr::iarmbus_acm_arg_t start_param;
   memset(&start_param, 0, sizeof(start_param));
   start_param.session_id = acm_session_id_;

   result = IARM_Bus_Call(IARMBUS_AUDIOCAPTUREMGR_NAME,
                          IARMBUS_AUDIOCAPTUREMGR_START,
                          &start_param, sizeof(start_param));
   if(result != IARM_RESULT_SUCCESS || start_param.result != audiocapturemgr::ACM_RESULT_SUCCESS) {
      XLOGD_ERROR("ACM start failed: IARM result %d, ACM result %d", result, start_param.result);
      acm_close();
      return false;
   }

   acm_session_started_ = true;

   // Start the capture thread to read from the ACM socket
   capture_thread_running_ = true;
   if(pthread_create(&capture_thread_id_, NULL, capture_thread_func, this) != 0) {
      XLOGD_ERROR("ACM capture thread creation failed: %s", strerror(errno));
      capture_thread_running_ = false;
      acm_close();
      return false;
   }

   XLOGD_INFO("ACM audio capture opened and started");
   return true;
}

void ctrlm_tv_audio_cap_t::acm_close() {
   IARM_Result_t result;

   if(acm_session_started_) {
      audiocapturemgr::iarmbus_acm_arg_t stop_param;
      memset(&stop_param, 0, sizeof(stop_param));
      stop_param.session_id = acm_session_id_;

      result = IARM_Bus_Call(IARMBUS_AUDIOCAPTUREMGR_NAME,
                             IARMBUS_AUDIOCAPTUREMGR_STOP,
                             &stop_param, sizeof(stop_param));
      if(result != IARM_RESULT_SUCCESS) {
         XLOGD_WARN("ACM stop failed: IARM result %d", result);
      }
      acm_session_started_ = false;
   }

   if(acm_session_id_ >= 0) {
      audiocapturemgr::iarmbus_acm_arg_t close_param;
      memset(&close_param, 0, sizeof(close_param));
      close_param.session_id = acm_session_id_;

      result = IARM_Bus_Call(IARMBUS_AUDIOCAPTUREMGR_NAME,
                             IARMBUS_AUDIOCAPTUREMGR_CLOSE,
                             &close_param, sizeof(close_param));
      if(result != IARM_RESULT_SUCCESS) {
         XLOGD_WARN("ACM close failed: IARM result %d", result);
      }
      acm_session_id_ = -1;
   }

   memset(acm_sock_path_, 0, sizeof(acm_sock_path_));
   XLOGD_INFO("ACM session closed");
}

bool ctrlm_tv_audio_cap_t::acm_connect_socket() {
   if(!strlen(acm_sock_path_)) {
      XLOGD_ERROR("ACM socket path is empty");
      return false;
   }

   struct sockaddr_un addr;
   memset(&addr, 0, sizeof(addr));
   addr.sun_family = AF_UNIX;
   strncpy(addr.sun_path, acm_sock_path_, sizeof(addr.sun_path) - 1);

   acm_sock_fd_ = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
   if(acm_sock_fd_ < 0) {
      XLOGD_ERROR("ACM socket creation failed: %s", strerror(errno));
      return false;
   }

   if(connect(acm_sock_fd_, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
      XLOGD_ERROR("ACM socket connect failed: %s", strerror(errno));
      close(acm_sock_fd_);
      acm_sock_fd_ = -1;
      return false;
   }

   XLOGD_INFO("ACM socket connected: fd <%d> path <%s>", acm_sock_fd_, acm_sock_path_);
   return true;
}

void ctrlm_tv_audio_cap_t::acm_disconnect_socket() {
   if(acm_sock_fd_ >= 0) {
      // Flush remaining data from the socket before closing
      uint8_t flush_buf[4096];
      unsigned int flush_count = 8;
      while(flush_count-- > 0) {
         ssize_t n = read(acm_sock_fd_, flush_buf, sizeof(flush_buf));
         if(n <= 0) {
            break;
         }
      }

      close(acm_sock_fd_);
      acm_sock_fd_ = -1;
      XLOGD_INFO("ACM socket disconnected");
   }
}
#endif
