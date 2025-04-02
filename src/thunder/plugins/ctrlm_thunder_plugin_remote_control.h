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
#ifndef __CTRLM_THUNDER_PLUGIN_REMOTE_CONTROL_H__
#define __CTRLM_THUNDER_PLUGIN_REMOTE_CONTROL_H__
#include "ctrlm_thunder_plugin.h"

namespace Thunder {
namespace RemoteControl {

/**
 * This class is used within ControlMgr to interact with the Remote Control Thunder Plugin. ControlMgr CURRENTLY only cares about the activation / deactivation of this plugin, 
 * so the implementation is quite basic.
 */
class ctrlm_thunder_plugin_remote_control_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * This function is used to get the Thunder Remote Control instance, as it is a Singleton.
     * @return The instance of the Thunder Remote Control, or NULL on error.
     */
    static ctrlm_thunder_plugin_remote_control_t *getInstance();

    /**
     * Remote Control Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_remote_control_t();

protected:
    /**
     * Remote Control Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_remote_control_t();
};
};
};

#endif
