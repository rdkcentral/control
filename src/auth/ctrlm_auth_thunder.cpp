#include "ctrlm_auth_thunder.h"
#include <secure_wrapper.h>
#include <string.h>
#include <array>
#include "ctrlm.h"
#include "ctrlm_log.h"

ctrlm_auth_thunder_t::ctrlm_auth_thunder_t() {
   XLOGD_INFO("Thunder Authservice Implementation");
   this->plugin = Thunder::AuthService::ctrlm_thunder_plugin_authservice_t::getInstance();
   this->plugin->add_event_handler(&ctrlm_auth_thunder_t::event_handler);
   this->plugin->add_activation_handler(&ctrlm_auth_thunder_t::activation_handler, (void *)this);
}

ctrlm_auth_thunder_t::~ctrlm_auth_thunder_t() {
}

bool ctrlm_auth_thunder_t::is_ready() {
   bool ret = false;
   if(this->plugin->is_activated()) {
#ifdef AUTH_ACTIVATION_STATUS
      if(this->plugin->is_device_activated()) {
         ret = true;
      } else {
         XLOGD_WARN("Device activation status is not activated");
      }
#else
      ret = true;
#endif
   } else {
      XLOGD_WARN("Plugin is not activated");
   }
   return(ret);
}

bool ctrlm_auth_thunder_t::get_receiver_id(std::string &receiver_id) {
   bool ret = this->plugin->get_receiver_id(receiver_id);
   return(ret);
}

bool ctrlm_auth_thunder_t::get_device_id(std::string &device_id) {
   bool ret = this->plugin->get_device_id(device_id);
   return(ret);
}

bool ctrlm_auth_thunder_t::get_account_id(std::string &account_id) {
   bool ret = this->plugin->get_account_id(account_id);
   return(ret);
}

bool ctrlm_auth_thunder_t::get_partner_id(std::string &partner_id) {
   bool ret = this->plugin->get_partner_id(partner_id);
   return(ret);
}

bool ctrlm_auth_thunder_t::get_experience(std::string &experience) {
   bool ret = this->plugin->get_experience(experience);
   return(ret);
}

bool ctrlm_auth_thunder_t::get_sat(std::string &sat, time_t &expiration) {
   bool ret = this->plugin->get_sat(sat, expiration);
   return(ret);
}

bool ctrlm_auth_thunder_t::supports_sat_expiration() const {
   return(true);
}

void ctrlm_auth_thunder_t::event_handler(Thunder::AuthService::authservice_event_t event, void *event_data, void *data) {
   switch(event) {
      case Thunder::AuthService::EVENT_SAT_CHANGED: {
         XLOGD_INFO("SAT Token Changed.. Invalidate SAT");
         ctrlm_main_invalidate_service_access_token();
         break;
      }
      case Thunder::AuthService::EVENT_ACCOUNT_ID_CHANGED: {
         XLOGD_INFO("Account ID Changed.. Set new Account ID");
         errno_t safec_rc = -1;
         const char *account_id = (const char *)event_data;
         ctrlm_main_queue_msg_account_id_update_t *msg = (ctrlm_main_queue_msg_account_id_update_t *)malloc(sizeof(ctrlm_main_queue_msg_account_id_update_t));
         if(msg == NULL) {
            XLOGD_ERROR("failed to allocate main queue message");
            return;
         }
         safec_rc = memset_s(msg, sizeof(ctrlm_main_queue_msg_account_id_update_t), 0, sizeof(ctrlm_main_queue_msg_account_id_update_t));
         ERR_CHK(safec_rc);
         msg->header.type       = CTRLM_MAIN_QUEUE_MSG_TYPE_ACCOUNT_ID_UPDATE;
         msg->header.network_id = CTRLM_MAIN_NETWORK_ID_ALL;
         safec_rc = strncpy_s(msg->account_id, sizeof(msg->account_id), account_id, strlen(account_id)); // strlen is OK, as this is a const char * from std::string.c_str()
         ERR_CHK(safec_rc);
         ctrlm_main_queue_msg_push(msg);
         break;
      }
#ifdef AUTH_ACTIVATION_STATUS
      case Thunder::AuthService::EVENT_ACTIVATION_STATUS_ACTIVATED: {
         XLOGD_INFO("Device Activated");
         ctrlm_main_auth_start_poll();
         break;
      }
#endif
      default: {
         XLOGD_WARN("Unknown authservice event <%d>", event);
         break;
      }
   }
}

void ctrlm_auth_thunder_t::activation_handler(Thunder::plugin_state_t state, void *data) {
   if(state == Thunder::PLUGIN_ACTIVATED) {
      ctrlm_main_auth_start_poll();
   }
}
