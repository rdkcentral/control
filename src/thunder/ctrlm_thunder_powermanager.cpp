#include "ctrlm_thunder_powermanager.h"
#include "ctrlm.h"
#include "ctrlm_log.h"

ctrlm_thunder_powermanager_t *ctrlm_thunder_powermanager_create() {
   return(new ctrlm_thunder_powermanager_t());
}

ctrlm_thunder_powermanager_t::ctrlm_thunder_powermanager_t() {
   XLOGD_INFO("Thunder PowerManager Implementation");
   this->plugin = Thunder::PowerManager::ctrlm_thunder_plugin_powermanager_t::get_instance();
}

ctrlm_thunder_powermanager_t::~ctrlm_thunder_powermanager_t() {
}

bool ctrlm_thunder_powermanager_t::is_ready() {
   bool ret = false;
   if(this->plugin->is_activated()) {
      ret = true;
   } else {
      XLOGD_WARN("Plugin is not activated");
   }
   return(ret);
}

void ctrlm_thunder_powermanager_t::get_power_state(ctrlm_power_state_t &power_state) {
   this->plugin->get_power_state(power_state);
}

void ctrlm_thunder_powermanager_t::get_networked_standby_mode(bool networked_standby_mode) {
   this->plugin->get_networked_standby_mode(networked_standby_mode);
}

void ctrlm_thunder_powermanager_t::get_wakeup_reason_voice(bool wakeup_reason_voice) {
   this->plugin->get_wakeup_reason_voice(wakeup_reason_voice);
}


/*
void ctrlm_thunder_powermanager_t::activation_handler(Thunder::plugin_state_t state, void *data) {
   if(state == Thunder::PLUGIN_ACTIVATED) {
//TODO SOMETHING      ctrlm_main_auth_start_poll();
   }
}
*/
