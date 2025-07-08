#ifndef __CTRLM_AUTH_THUNDER_H__
#define __CTRLM_AUTH_THUNDER_H__

#include "ctrlm_auth.h"
#include "ctrlm_thunder_plugin_authservice.h"

class ctrlm_auth_thunder_t : public ctrlm_auth_t {
public:
   ctrlm_auth_thunder_t();
   virtual ~ctrlm_auth_thunder_t();

   virtual bool is_ready();
   virtual bool get_device_id(std::string &device_id);
   virtual bool get_account_id(std::string &account_id);
   virtual bool get_partner_id(std::string &partner_id);
   virtual bool get_experience(std::string &experience);
   virtual bool get_sat(std::string &sat, time_t &expiration);
   virtual bool supports_sat_expiration() const;

protected:
    static void event_handler(Thunder::AuthService::authservice_event_t event, void *event_data, void *data);
    static void activation_handler(Thunder::plugin_state_t state, void *data);

private:
    Thunder::AuthService::ctrlm_thunder_plugin_authservice_t *plugin;
};

#endif