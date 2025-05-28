#include "ctrlm_thunder_powermanager.h"
#include "ctrlm.h"
#include "ctrlm_log.h"

static ctrlm_thunder_powermanager_t *instance = NULL;

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

ctrlm_thunder_powermanager_t* ctrlm_thunder_powermanager_t::get_instance() {

   if(instance == NULL) {
      instance = new ctrlm_thunder_powermanager_t();
   }

   return(instance);
}

void ctrlm_thunder_powermanager_t::destroy_instance() {

   if (instance != NULL) {
        delete instance;
        instance = NULL;
    }
}


ctrlm_power_state_t ctrlm_thunder_powermanager_t::get_power_state() {

   if(this->plugin == NULL) {
      XLOGD_WARN("plugin not yet available");
      return CTRLM_POWER_STATE_ON;
   } else {
      return this->plugin->get_power_state();
   }
}

#ifdef NETWORKED_STANDBY_MODE_ENABLED
bool ctrlm_thunder_powermanager_t::get_networked_standby_mode() {

   if(this->plugin == NULL) {
      XLOGD_ERROR("plugin not yet available");
      return false;
   } else {
      return this->plugin->get_networked_standby_mode(networked_standby_mode);
   }
}

bool ctrlm_thunder_powermanager_t::get_wakeup_reason_voice() {

   if(this->plugin == NULL) {
      XLOGD_WARN("plugin not yet available");
      return false;
   } else {
      return this->plugin->get_wakeup_reason_voice(wakeup_reason_voice);
   }
}
#endif
