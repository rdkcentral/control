#ifndef __CTRLM_THUNDER_POWERMANAGER_H__
#define __CTRLM_THUNDER_POWERMANAGER_H__

#include "ctrlm_powermanager.h"
#include "ctrlm_thunder_plugin_powermanager.h"

class ctrlm_thunder_powermanager_t : public ctrlm_powermanager_t {
public:
   ctrlm_thunder_powermanager_t();
   virtual ~ctrlm_thunder_powermanager_t();

   bool is_ready();
   ctrlm_power_state_t get_power_state();
   bool get_networked_standby_mode();
   bool get_wakeup_reason_voice();

private:
   Thunder::PowerManager::ctrlm_thunder_plugin_powermanager_t *plugin;
};

#endif
