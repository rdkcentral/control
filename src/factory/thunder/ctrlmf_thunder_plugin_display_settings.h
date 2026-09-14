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
#ifndef __CTRLMF_THUNDER_PLUGIN_DISPLAY_SETTINGS_H__
#define __CTRLMF_THUNDER_PLUGIN_DISPLAY_SETTINGS_H__
#include "ctrlmf_thunder_plugin.h"

namespace Thunder {
namespace DisplaySettings {

/**
 * This class is used within Control Factory to interact with the DisplaySettings Thunder Plugin.
 */
class ctrlmf_thunder_plugin_display_settings_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * This function is used to get the Thunder Display Settings instance, as it is a Singleton.
     * @return The instance of the Thunder Display Settings, or NULL on error.
     */
    static ctrlmf_thunder_plugin_display_settings_t *getInstance();

    /**
     * Display Settings Thunder Plugin Destructor
     */
    virtual ~ctrlmf_thunder_plugin_display_settings_t();

    /**
     * Calls DisplaySettings.setAudioDucking on the SPEAKER0 audio port.
     * @param action  true to start ducking, false to stop
     * @param type    true for relative ducking, false for absolute
     * @param level   Volume level 0-100
     * @return true on success
     */
    bool set_audio_ducking(bool action, bool type, unsigned char level);

protected:
    /**
     * DisplaySettings Thunder Plugin Default Constructor
     */
    ctrlmf_thunder_plugin_display_settings_t();
};
};
};
#endif
