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
#ifndef __CTRLM_THUNDER_PLUGIN_FRONT_PANEL_H__
#define __CTRLM_THUNDER_PLUGIN_FRONT_PANEL_H__
#include "ctrlm_thunder_plugin.h"

namespace Thunder {
namespace FrontPanel {

/**
 * This class is used within ControlMgr to interact with the FrontPanel Thunder Plugin.
 * It replaces direct libds device::FrontPanelIndicator calls with Thunder plugin calls.
 *
 * libds → Thunder mapping:
 *   FrontPanelIndicator::setColor(0xFFFFFF)
 *   + FrontPanelIndicator::setBrightness() → setLED (ledIndicator="power_led", brightness, color, red, green, blue)
 *   FrontPanelIndicator::setState(true)    → powerLedOn  (index="power_led")
 *   FrontPanelIndicator::setState(false)   → powerLedOff (index="power_led")
 *   FrontPanelIndicator::setBrightness()   → setBrightness (index="power_led", brightness)
 * Note: toPersist semantics are not preserved by the FrontPanel Thunder plugin API.
 */
class ctrlm_thunder_plugin_front_panel_t : public Thunder::Plugin::ctrlm_thunder_plugin_t {
public:
    /**
     * Returns the singleton instance of the FrontPanel Thunder plugin.
     * @return The instance, or NULL on error.
     */
    static ctrlm_thunder_plugin_front_panel_t *getInstance();

    /**
     * FrontPanel Thunder Plugin Destructor
     */
    virtual ~ctrlm_thunder_plugin_front_panel_t();

    /**
     * Turns the Power LED on: sets colour (white/0xFFFFFF) and brightness via setLED,
     * then activates the LED via powerLedOn.
     * Calls Thunder: org.rdk.FrontPanel.setLED + org.rdk.FrontPanel.powerLedOn
     * Note: toPersist semantics are not preserved (not supported by the plugin API).
     * @param brightness  Level 0-100, default 100.
     * @return true on success.
     */
    bool power_led_on(int brightness = 100);

    /**
     * Turns the Power LED off.
     * Calls Thunder: org.rdk.FrontPanel.powerLedOff
     * @return true on success.
     */
    bool power_led_off();

    /**
     * Sets the brightness of the Power LED without changing its on/off state.
     * Calls Thunder: org.rdk.FrontPanel.setBrightness
     * Note: persist semantics are not preserved (not supported by the plugin API).
     * @param brightness  Level 0-100.
     * @return true on success.
     */
    bool set_brightness(int brightness);

protected:
    /**
     * FrontPanel Thunder Plugin Default Constructor
     */
    ctrlm_thunder_plugin_front_panel_t();

    /**
     * No events to register for this plugin.
     */
    virtual bool register_events() { return true; }

    /**
     * Nothing to do on initial activation.
     */
    virtual void on_initial_activation();
};

};
};
#endif
