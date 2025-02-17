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
#ifndef __CTRLMF_THUNDER_PLUGIN_SYSTEM_AUDIO_PLAYER_H__
#define __CTRLMF_THUNDER_PLUGIN_SYSTEM_AUDIO_PLAYER_H__
#include "ctrlmf_thunder_plugin.h"

#define SYSTEM_AUDIO_PLAYER_AUDIO_TYPE_PCM        "pcm"
#define SYSTEM_AUDIO_PLAYER_AUDIO_TYPE_MP3        "mp3"
#define SYSTEM_AUDIO_PLAYER_AUDIO_TYPE_WAV        "wav"

#define SYSTEM_AUDIO_PLAYER_SOURCE_TYPE_WEBSOCKET "websocket"
#define SYSTEM_AUDIO_PLAYER_SOURCE_TYPE_HTTP      "httpsrc"
#define SYSTEM_AUDIO_PLAYER_SOURCE_TYPE_FILE      "filesrc"
#define SYSTEM_AUDIO_PLAYER_SOURCE_TYPE_DATA      "data"

#define SYSTEM_AUDIO_PLAYER_PLAY_MODE_SYSTEM      "system"
#define SYSTEM_AUDIO_PLAYER_PLAY_MODE_APP         "app"

typedef enum {
    SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_STARTED,
    SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_FINISHED,
    SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_PAUSED,
    SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_RESUMED,
    SYSTEM_AUDIO_PLAYER_EVENT_NETWORK_ERROR,
    SYSTEM_AUDIO_PLAYER_EVENT_PLAYBACK_ERROR,
    SYSTEM_AUDIO_PLAYER_EVENT_NEED_DATA,
    SYSTEM_AUDIO_PLAYER_EVENT_INVALID
} system_audio_player_event_t;

namespace Thunder {
namespace SystemAudioPlayer {

typedef void (*system_audio_player_event_handler_t)(system_audio_player_event_t event, void *user_data);

/**
 * This class is used within ControlMgr to interact with the System Audio Player Thunder Plugin. This implementation exposes methods to call for playing audio,
 * along with some events that are important for the ControlMgr implementation.
 */
class ctrlm_thunder_plugin_system_audio_player_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * Authservice Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_system_audio_player_t();

    /**
     * Authservice Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_system_audio_player_t();

    /**
     * Function that opens the System Audio Player.
     * @param  audiotype  pcm, mp3, wav can be passed as parameter
     * @param  sourcetype websocket, httpsrc, filesrc, data can be passed as parameter
     * @param  playmode   system, app can be passed as parameter
     * @return True if opened successfully otherwise False
     */
    bool open(const char *audio_type, const char *source_type, const char *play_mode);

    /**
     * Function that closes the System Audio Player.
     * @return True if closed successfully otherwise False
     */
    bool close();

    /**
     * Function that plays the audio provided.
     * @param  url url source
     * @return True if closed successfully otherwise False
     */
    bool play(std::string url);

    /**
     * Function that pauses the audio playback.
     * @return True if paused successfully otherwise False
     */
    bool pause();

    /**
     * Function that resumes the playback from where its paused.
     * @return True if paused successfully otherwise False
     */
    bool resume();

    /**
     * Function that controls primary and audio player's volume.
     * @param  volume_primary 
     * @param  volume_player 
     * @return True on success otherwise False.
     */
    bool setMixerLevels(std::string volume_primary, std::string volume_player);

    /**
     * Function that configures PCM/MP3 audio source options.
     * @param  format 
     * @param  rate 
     * @param  channels 
     * @param  layout 
     * @return True on success otherwise False.
     */
    bool config(std::string format, std::string rate, std::string channels, std::string layout);

    /**
     * This function is used to register a handler for the System Audio Player Thunder Plugin events.
     * @param handler The pointer to the function to handle the event.
     * @param user_data A pointer to data to pass to the event handler. This data is NOT freed when the handler is removed.
     * @return True if the event handler was added, otherwise False.
     */
    bool add_event_handler(system_audio_player_event_handler_t handler, void *user_data = NULL);

    /**
     * This function is used to deregister a handler for the System Audio Player Thunder Plugin events.
     * @param handler The pointer to the function that handled the event.
     */
    void remove_event_handler(system_audio_player_event_handler_t handler);

public:
    /** 
     * This function is technically used internally but from static function. This is used to broadcast SAP events.
     */
    void on_sap_events(std::string str_id, std::string str_event);


protected:
    /**
     * This function is called when registering for Thunder events.
     * @return True if the events were registered for successfully otherwise False.
     */
    virtual bool register_events();

private:
    std::vector<std::pair<system_audio_player_event_handler_t, void *> > event_callbacks;
    
    bool        registered_events;
    std::string id_active;
};

const char *system_audio_player_event_str(system_audio_player_event_t event);

};
};

#endif
