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

#ifndef __CTRLM_TV_AUDIO_CAP_H__
#define __CTRLM_TV_AUDIO_CAP_H__

#include <stdint.h>
#include <pthread.h>
#include <mutex>

#ifdef USE_AC_RMF
#include "rmfAudioCapture.h"
#endif

#ifdef USE_ACM
#include "audiocapturemgr_iarm.h"
#endif

/**
 * @brief ControlMgr TV Audio Capture Class
 *
 * This class is a singleton that interfaces with the rmf audio capture
 * and/or audio capture manager libraries. It starts TV audio processing
 * when mid-field voice devices are connected and stops when none remain.
 * Captured audio frames are written to a pipe file descriptor for
 * consumption by the xr-voice-sdk.
 */
class ctrlm_tv_audio_cap_t {
public:
   /**
    * This function is used to get the TV Audio Capture instance, as it is a Singleton.
    * @return The instance of the TV Audio Capture, or NULL on error.
    */
   static ctrlm_tv_audio_cap_t *get_instance();
   /**
    * This function is used to destroy the sole instance of the TV Audio Capture object.
    */
   static void destroy_instance();

   virtual ~ctrlm_tv_audio_cap_t();

   /**
    * Updates the count of connected mid-field voice devices.
    * Starts audio capture when count transitions from 0 to > 0.
    * Stops audio capture when count transitions to 0.
    * @param device_qty The number of connected mid-field voice devices.
    */
   void update_device_qty(uint32_t device_qty);

   /**
    * Returns the read end of the audio pipe for the xr-voice-sdk to consume.
    * @return The read file descriptor, or -1 if not capturing.
    */
   int audio_pipe_fd_get() const;

   /**
    * Returns whether TV audio capture is currently active.
    * @return true if capturing, false otherwise.
    */
   bool is_capturing() const;

private:
   /**
    * Default Constructor (Private due to it being a Singleton)
    */
   ctrlm_tv_audio_cap_t();

   bool start();
   void stop();

   static void *capture_thread_func(void *context);
   void         capture_thread_run();

#ifdef USE_AC_RMF
   bool rmf_open();
   void rmf_close();
   static rmf_Error rmf_buffer_ready_cb(void *context, void *buf, unsigned int size);
   RMF_AudioCaptureHandle      rmf_handle_;
   RMF_AudioCapture_Settings   rmf_default_settings_;
   std::mutex                  pipe_write_mutex_;
#endif

#ifdef USE_ACM
   bool acm_open();
   void acm_close();
   bool acm_connect_socket();
   void acm_disconnect_socket();
   audiocapturemgr::session_id_t acm_session_id_;
   bool                          acm_session_started_;
   char                          acm_sock_path_[MAX_OUTPUT_PATH_LEN];
   int                           acm_sock_fd_;
#endif

   mutable std::mutex mutex_;
   int                pipe_fds_[2];
   uint32_t           device_qty_;
   bool               capturing_;
   pthread_t          capture_thread_id_;
   bool               capture_thread_running_;
};

#endif
