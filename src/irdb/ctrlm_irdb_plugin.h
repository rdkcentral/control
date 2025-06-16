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

#ifndef __CTRLM_IRDB_PLUGIN_H__
#define __CTRLM_IRDB_PLUGIN_H__

#include <string>
#include <map>
#include <vector>


typedef enum {
    CTRLM_IRDB_MODE_OFFLINE,
    CTRLM_IRDB_MODE_ONLINE,
    CTRLM_IRDB_MODE_HYBRID
} ctrlm_irdb_mode_t;

typedef enum {
    CTRLM_IRDB_DEV_TYPE_TV,
    CTRLM_IRDB_DEV_TYPE_AVR,
    CTRLM_IRDB_DEV_TYPE_INVALID
} ctrlm_irdb_dev_type_t;

typedef enum {
    CTRLM_IRDB_KEY_POWER_OFF = 0,
    CTRLM_IRDB_KEY_POWER_ON,
    CTRLM_IRDB_KEY_POWER_TOGGLE,
    CTRLM_IRDB_KEY_VOLUME_MUTE,
    CTRLM_IRDB_KEY_VOLUME_UP,
    CTRLM_IRDB_KEY_VOLUME_DOWN,
    CTRLM_IRDB_KEY_INPUT_SELECT,
    CTRLM_IRDB_KEY_INVALID,
    CTRLM_IRDB_KEY_MAX
} ctrlm_irdb_key_code_t;

typedef struct {
    std::string   name;
    unsigned char rcu_support_bitmask;
} ctrlm_irdb_vendor_info_t;

typedef std::vector<std::string> ctrlm_irdb_manufacturer_list_t;
typedef std::vector<std::string> ctrlm_irdb_model_list_t;
typedef std::vector<std::string> ctrlm_irdb_entry_id_list_t;

typedef std::map<ctrlm_irdb_key_code_t, std::vector<unsigned char>> ctrlm_irdb_ir_waveforms_t;

typedef struct {
    ctrlm_irdb_dev_type_t       type;
    std::string                 id;
    ctrlm_irdb_ir_waveforms_t   waveforms;
} ctrlm_irdb_ir_code_set_t;

typedef struct {
    std::string manufacturer;
    std::string model;
    std::string id;
    int         rank;
} ctrlm_irdb_autolookup_entry_ranked_t;

typedef std::vector<ctrlm_irdb_autolookup_entry_ranked_t> ctrlm_irdb_autolookup_ranked_list_t;


#ifdef __cplusplus
extern "C" {
#endif

std::string irdb_version();

bool ctrlm_irdb_open(bool platform_tv, const std::string &unique_id = "");

bool ctrlm_irdb_close();

bool ctrlm_irdb_initialize();

bool ctrlm_irdb_get_vendor_info(ctrlm_irdb_vendor_info_t &info);

bool ctrlm_irdb_get_manufacturers(ctrlm_irdb_manufacturer_list_t &manufacturers, ctrlm_irdb_dev_type_t type, const std::string &prefix);

bool ctrlm_irdb_get_models(ctrlm_irdb_model_list_t &models, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &prefix);

bool ctrlm_irdb_get_entry_ids(ctrlm_irdb_entry_id_list_t &ids, ctrlm_irdb_dev_type_t type, const std::string &manufacturer, const std::string &model);

bool ctrlm_irdb_get_ir_code_set(ctrlm_irdb_ir_code_set_t &code_set, ctrlm_irdb_dev_type_t type, const std::string &id);

bool ctrlm_irdb_get_ir_codes_by_infoframe(ctrlm_irdb_autolookup_ranked_list_t &codes, ctrlm_irdb_dev_type_t &type, unsigned char *infoframe, unsigned int infoframe_len);

bool ctrlm_irdb_get_ir_codes_by_edid(ctrlm_irdb_autolookup_ranked_list_t &codes, ctrlm_irdb_dev_type_t &type, unsigned char *edid, unsigned int edid_len);

bool ctrlm_irdb_get_ir_codes_by_cec(ctrlm_irdb_autolookup_ranked_list_t &codes, ctrlm_irdb_dev_type_t &type, const std::string &osd, unsigned int vendor_id, unsigned int logical_address);

#ifdef __cplusplus
}
#endif

#endif