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
#include "ctrlm_thunder_plugin_front_panel.h"
#include "ctrlm_log.h"
#include <WPEFramework/core/core.h>
#include <WPEFramework/websocket/websocket.h>
#include <WPEFramework/plugins/plugins.h>

using namespace Thunder;
using namespace FrontPanel;
using namespace WPEFramework;

ctrlm_thunder_plugin_front_panel_t::ctrlm_thunder_plugin_front_panel_t()
    : ctrlm_thunder_plugin_t("FrontPanel", "org.rdk.FrontPanel", 1) {
}

ctrlm_thunder_plugin_front_panel_t::~ctrlm_thunder_plugin_front_panel_t() {
}

ctrlm_thunder_plugin_front_panel_t *ctrlm_thunder_plugin_front_panel_t::getInstance() {
    static ctrlm_thunder_plugin_front_panel_t instance;
    return &instance;
}

void ctrlm_thunder_plugin_front_panel_t::on_initial_activation() {
    Thunder::Plugin::ctrlm_thunder_plugin_t::on_initial_activation();
}

bool ctrlm_thunder_plugin_front_panel_t::power_led_on(int brightness) {
    // Set LED colour (white = 0xFFFFFF) and brightness via setLED first
    JsonObject params_led, response_led;
    params_led["ledIndicator"] = "power_led";
    params_led["brightness"]   = brightness;
    params_led["color"]        = "white";
    params_led["red"]          = 255;
    params_led["green"]        = 255;
    params_led["blue"]         = 255;
    bool ret = this->call_plugin("setLED", (void *)&params_led, (void *)&response_led);
    if(!ret) {
        XLOGD_ERROR("FrontPanel setLED call failed");
    }

    // Activate the LED via powerLedOn
    JsonObject params_on, response_on;
    params_on["index"] = "power_led";
    if(!this->call_plugin("powerLedOn", (void *)&params_on, (void *)&response_on)) {
        XLOGD_ERROR("FrontPanel powerLedOn call failed");
        ret = false;
    }
    return ret;
}

bool ctrlm_thunder_plugin_front_panel_t::power_led_off() {
    JsonObject params, response;
    params["index"] = "power_led";
    if(!this->call_plugin("powerLedOff", (void *)&params, (void *)&response)) {
        XLOGD_ERROR("FrontPanel powerLedOff call failed");
        return false;
    }
    return true;
}

bool ctrlm_thunder_plugin_front_panel_t::set_brightness(int brightness) {
    JsonObject params, response;
    params["index"]      = "power_led";
    params["brightness"] = brightness;
    if(!this->call_plugin("setBrightness", (void *)&params, (void *)&response)) {
        XLOGD_ERROR("FrontPanel setBrightness call failed");
        return false;
    }
    return true;
}
