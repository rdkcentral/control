#ifndef __CTRLM_THUNDER_POWERMANAGER_H__
<<<<<<< HEAD
#define __CTRLM_THUNDER_POWERMANAGER_H__
=======
#define __CTRLM_TUNDER_POWERMANAGER_H__
>>>>>>> 66311b27882fe408e6bd16e2fb839c102febd562

#include "ctrlm_thunder_plugin_powermanager.h"

class ctrlm_thunder_powermanager_t {
public:
   ctrlm_thunder_powermanager_t();
   virtual ~ctrlm_thunder_powermanager_t();

   virtual bool is_ready();

   void get_power_state(ctrlm_power_state_t &power_state);
<<<<<<< HEAD
   #ifdef NETWORKED_STANDBY_MODE
   void get_networked_standby_mode(bool networked_standby_mode);
   void get_wakeup_reason_voice(bool wakeup_reason_voice);
   #endif

private:
   Thunder::PowerManager::ctrlm_thunder_plugin_powermanager_t *plugin;
=======
   void get_networked_standby_mode(bool networked_standby_mode);
   void get_wakeup_reason_voice(bool wakeup_reason_voice);

protected:
    //static void event_handler(Thunder::PowerManager::authservice_event_t event, void *event_data, void *data);

private:
    Thunder::PowerManager::ctrlm_thunder_plugin_powermanager_t *plugin;
>>>>>>> 66311b27882fe408e6bd16e2fb839c102febd562
};

ctrlm_thunder_powermanager_t *ctrlm_thunder_powermanager_create();

#endif
