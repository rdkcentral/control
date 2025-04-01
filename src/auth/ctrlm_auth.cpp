#include "ctrlm_auth.h"
#include "ctrlm_auth_thunder.h"

ctrlm_auth_t *ctrlm_auth_service_create(std::string url) {
   return(new ctrlm_auth_thunder_t());
}

ctrlm_auth_t::ctrlm_auth_t() {
}

ctrlm_auth_t::~ctrlm_auth_t() {
}

