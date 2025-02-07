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
#ifndef __CTRLMF_THUNDER_PLUGIN_CONTROL_MANAGER_H__
#define __CTRLMF_THUNDER_PLUGIN_CONTROL_MANAGER_H__

#include "ctrlmf_thunder_plugin.h"

typedef enum {
    CONTROL_MANAGER_EVENT_SESSION_BEGIN,
    CONTROL_MANAGER_EVENT_SESSION_END,
    CONTROL_MANAGER_EVENT_STREAM_BEGIN,
    CONTROL_MANAGER_EVENT_STREAM_END,
    CONTROL_MANAGER_EVENT_SERVER_MESSAGE,
    CONTROL_MANAGER_EVENT_KEYWORD_VERIFICATION,
    CONTROL_MANAGER_EVENT_INVALID
} control_manager_event_t;

typedef struct {
   int32_t     remote_id;
   const char *session_id;
   const char *device_type;
   bool        keyword_verification;
} control_manager_event_session_begin_t;

typedef struct {
   int32_t     remote_id;
   const char *session_id;
} control_manager_event_session_end_t;

typedef struct {
   int32_t     remote_id;
   const char *session_id;
} control_manager_event_stream_begin_t;

typedef struct {
   int32_t     remote_id;
   const char *session_id;
} control_manager_event_stream_end_t;

typedef struct {
   const char *session_id;
} control_manager_event_server_message_t;

typedef struct {
   int32_t     remote_id;
   const char *session_id;
} control_manager_event_keyword_verification_t;

namespace Thunder {
namespace ControlManager {

typedef void (*control_manager_event_handler_t)(control_manager_event_t event, void *user_data);

/**
 * This class is used within Control Factory to interact with the Control Manager Thunder Plugin.
 */
class ctrlm_thunder_plugin_control_manager_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * Control Manager Thunder Plugin Constructor
     */
    ctrlm_thunder_plugin_control_manager_t();

    /**
     * Control Manager Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_control_manager_t();

    bool configure_voice_hf(std::string &url_hf, bool enable);
    bool configure_voice_mic_tap(std::string &url_mic_tap, bool enable);

    bool send_voice_message(void);

    bool set_voice_init(void);

    bool voice_session_types(std::vector<std::string> &types);
    bool voice_session_request(std::string &type, std::string &transcription, std::string &audio_file);
    bool voice_session_terminate(std::string &session_id);

    bool status_voice_hf(std::string &url_hf, bool *url_hf_enabled);
    bool status_voice_mic_tap(std::string &url_mic_tap, bool *url_mic_tap_enabled);

    bool add_event_handler(control_manager_event_handler_t handler, void *user_data = NULL);
    void remove_event_handler(control_manager_event_handler_t handler);

    void on_session_begin(int32_t remote_id, std::string session_id, std::string device_type, bool keyword_verification);
    void on_session_end(int32_t remote_id, std::string session_id);
    void on_stream_begin(int32_t remote_id, std::string session_id);
    void on_stream_end(int32_t remote_id, std::string session_id);
    void on_server_message(std::string session_id);
    void on_keyword_verification(int32_t remote_id, std::string session_id);

protected:
    /**
     * This function is called when registering for Thunder events.
     * @return True if the events were registered for successfully otherwise False.
     */
    virtual bool register_events();

private:

    std::vector<std::pair<control_manager_event_handler_t, void *> > event_callbacks;

    bool        registered_events;
};

};
};

#endif
