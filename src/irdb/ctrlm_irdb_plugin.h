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

#ifdef __cplusplus
extern "C" {
#endif


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
    unsigned int list_qty;
    char **names;
} ctrlm_irdb_manufacturer_list_t;

typedef struct {
    unsigned int list_qty;
    char **names;
} ctrlm_irdb_model_list_t;

typedef struct {
    unsigned int list_qty;
    char **ids;
} ctrlm_irdb_entry_id_list_t;

typedef struct {
    ctrlm_irdb_key_code_t key_code;
    unsigned int list_qty;
    unsigned char *data;
} ctrlm_irdb_ir_waveform_t;

typedef struct {
    ctrlm_irdb_dev_type_t type;
    char *id;
    unsigned int list_qty;
    ctrlm_irdb_ir_waveform_t *waveforms;
} ctrlm_irdb_ir_code_set_t;

typedef struct {
    char *manufacturer;
    char *model;
    char *id;
    int  rank;
} ctrlm_irdb_autolookup_entry_ranked_t;

typedef struct {
    unsigned int                         list_qty;
    ctrlm_irdb_autolookup_entry_ranked_t *ranked_list;
} ctrlm_irdb_autolookup_ranked_list_t;


char *irdb_version();

bool ctrlm_irdb_open(bool platform_tv, const char* unique_id = NULL);

bool ctrlm_irdb_initialize();

unsigned char ctrlm_irdb_get_vendor_support_bit();

bool ctrlm_irdb_get_manufacturers(ctrlm_irdb_manufacturer_list_t *manufacturers, ctrlm_irdb_dev_type_t type, const char *prefix);

bool ctrlm_irdb_get_models(ctrlm_irdb_model_list_t *models, ctrlm_irdb_dev_type_t type, const char *manufacturer, const char *prefix);

bool ctrlm_irdb_get_codes_by_names(ctrlm_irdb_entry_id_list_t *ids, ctrlm_irdb_dev_type_t type, const char *manufacturer, const char *model);

bool ctrlm_irdb_get_ir_code_set(ctrlm_irdb_ir_code_set_t *code_set, ctrlm_irdb_dev_type_t type, const char *id);

bool ctrlm_irdb_get_ir_codes_by_infoframe(ctrlm_irdb_autolookup_ranked_list_t *codes, ctrlm_irdb_dev_type_t *type, unsigned char *infoframe, unsigned int infoframe_len);

bool ctrlm_irdb_get_ir_codes_by_edid(ctrlm_irdb_autolookup_ranked_list_t *codes, ctrlm_irdb_dev_type_t *type, unsigned char *edid, unsigned int edid_len);

bool ctrlm_irdb_get_ir_codes_by_cec(ctrlm_irdb_autolookup_ranked_list_t *codes, ctrlm_irdb_dev_type_t *type, const char *osd, unsigned int vendor_id, unsigned int logical_address);

bool ctrlm_irdb_close();

#ifdef __cplusplus
}
#endif

#endif