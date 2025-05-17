#include "ctrlm_thunder_powermanager.h"
#include "ctrlm.h"
#include "ctrlm_log.h"

static ctrlm_thunder_powermanager_t *instance = NULL;

ctrlm_thunder_powermanager_t *ctrlm_thunder_powermanager_create() {
   if(instance == NULL) {
      instance = new ctrlm_thunder_powermanager_t();
   }
   return(instance);
}

ctrlm_thunder_powermanager_t::ctrlm_thunder_powermanager_t() {
   XLOGD_INFO("Thunder PowerManager Implementation");
   this->plugin = Thunder::PowerManager::ctrlm_thunder_plugin_powermanager_t::get_instance();
}

ctrlm_thunder_powermanager_t::~ctrlm_thunder_powermanager_t() {
}

bool ctrlm_thunder_powermanager_t::is_ready() {
   bool ret = false;

   if(this->plugin == NULL) {
      XLOGD_WARN("plugin not yet available");
   } else {
      if(this->plugin->is_activated()) {
         ret = true;
      } else {
         XLOGD_WARN("Plugin is not activated");
      }
   }
   return(ret);
}

void ctrlm_thunder_powermanager_t::get_power_state(ctrlm_power_state_t &power_state) {
   this->plugin->get_power_state(power_state);
}

#ifdef NETWORKED_STANDBY_MODE_ENABLED
void ctrlm_thunder_powermanager_t::get_networked_standby_mode(bool networked_standby_mode) {
   this->plugin->get_networked_standby_mode(networked_standby_mode);
}

void ctrlm_thunder_powermanager_t::get_wakeup_reason_voice(bool wakeup_reason_voice) {
   this->plugin->get_wakeup_reason_voice(wakeup_reason_voice);
}
#endif

