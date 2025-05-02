/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2014 RDK Management
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

#include "ctrlm_log.h"
#include "ctrlm_irdb_stub.h"


const char* STUB_irdb_version() {
    XLOGD_ERROR("not implemented");
    return("INVALID");
}
bool STUB_ctrlm_irdb_open(bool platform_tv) {
    XLOGD_ERROR("not implemented");
    return(false);
}
bool STUB_ctrlm_irdb_close() {
    XLOGD_ERROR("not implemented");
    return(false);
}
bool STUB_ctrlm_irdb_initialize() {
    XLOGD_ERROR("not implemented");
    return(false);
}
bool STUB_ctrlm_irdb_get_manufacturers(ctrlm_irdb_manufacturer_list_t *manufacturers, ctrlm_irdb_dev_type_t type, const char *prefix) {
    XLOGD_ERROR("not implemented");
    return(false);
}
bool STUB_ctrlm_irdb_get_models(ctrlm_irdb_model_list_t *models, ctrlm_irdb_dev_type_t type, const char *manufacturer, const char *prefix) {
    XLOGD_ERROR("not implemented");
    return(false);
}
bool STUB_ctrlm_irdb_get_entry_ids(ctrlm_irdb_entry_id_list_t *codes, ctrlm_irdb_dev_type_t type, const char *manufacturer, const char *model) {
    XLOGD_ERROR("not implemented");
    return(false);
}
bool STUB_ctrlm_irdb_get_ir_code_set(ctrlm_irdb_ir_code_set_t *code_set, ctrlm_irdb_dev_type_t type, const char *id) {
    XLOGD_ERROR("not implemented");
    return(false);
}
bool STUB_ctrlm_irdb_get_ir_codes_by_infoframe(ctrlm_irdb_autolookup_ranked_list_t *codes, ctrlm_irdb_dev_type_t *type, unsigned char *infoframe, unsigned int infoframe_len) {
    XLOGD_ERROR("not implemented");
    return(false);
}
bool STUB_ctrlm_irdb_get_ir_codes_by_edid(ctrlm_irdb_autolookup_ranked_list_t *codes, ctrlm_irdb_dev_type_t *type, unsigned char *edid, unsigned int edid_len) {
    XLOGD_ERROR("not implemented");
    return(false);
}
bool STUB_ctrlm_irdb_get_ir_codes_by_cec(ctrlm_irdb_autolookup_ranked_list_t *codes, ctrlm_irdb_dev_type_t *type, const char *osd, unsigned int vendor_id, unsigned int logical_address) {
    XLOGD_ERROR("not implemented");
    return(false);
}
