/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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
#include "ctrlm_rf4ce_controller_attr_general.h"
#include "ctrlm_rf4ce_controller_attr_voice.h"
#include "ctrlm_db_types.h"
#include "ctrlm.h"
#include "ctrlm_log.h"
#include "ctrlm_utils.h"
#include <uuid/uuid.h>
#include <algorithm>
#include <sstream>
#include "ctrlm_database.h"
#include "rf4ce/ctrlm_rf4ce_controller.h"
#include "rf4ce/ctrlm_rf4ce_utils.h"

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_rf4ce_controller_memory_dump_t

// Specification: Comcast-SP-RF4CE-XRC-Profile-D11-160505
// 6.5.8.12 Memory Dump
#define   CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_DUMP        (16)
#define   CTRLM_CRASH_DUMP_OFFSET_MEMORY_SIZE         (0)
#define   CTRLM_CRASH_DUMP_OFFSET_MEMORY_IDENTIFIER   (2)
#define   CTRLM_CRASH_DUMP_OFFSET_RESERVED_0xFF       (4)
#define   CTRLM_CRASH_DUMP_MAX_SIZE                   (255*CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_DUMP)

// Remote Crash Dump File header                          // Dump File Format
#define CTRLM_CRASH_DUMP_FILE_LEN_SIGNATURE           (4)   // 4 bytes signature 'XDMP'
#define CTRLM_CRASH_DUMP_FILE_OFFSET_HDR_SIZE         (4)   // 2 bytes header size
#define CTRLM_CRASH_DUMP_FILE_OFFSET_DEVICE_ID        (6)   
#define CTRLM_CRASH_DUMP_FILE_LEN_DEVICE_ID           (4)   // 4 bytes device id
#define CTRLM_CRASH_DUMP_FILE_OFFSET_HW_VER          (10)   // 4 bytes hw version
#define CTRLM_CRASH_DUMP_FILE_OFFSET_SW_VER          (14)   // 4 bytes sw version
#define CTRLM_CRASH_DUMP_FILE_OFFSET_NUM_DUMPS       (18)   // 2 bytes number of dumps in the file
#define CTRLM_CRASH_DUMP_FILE_HDR_SIZE               (20)    
// Remote Crash FILE Data Header (offsets are relative to beginning of theheader)
#define CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_MEM_ID       (0)   // 2 bytes memory Id
#define CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_DATA         (2)   // 4 bytes offset to dump data
#define CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_DATA_LEN     (6)   // 2 bytes data length
#define CTRLM_CRASH_DUMP_DATA_HDR_SIZE                (8)

#define MEMORY_DUMP_ID       (0xFE)
#define MEMORY_DUMP_INDEX    (RIB_ATTR_INDEX_ALL)
ctrlm_rf4ce_controller_memory_dump_t::ctrlm_rf4ce_controller_memory_dump_t(ctrlm_obj_controller_rf4ce_t *controller) : 
ctrlm_attr_t("Controller Memory Dump"),
ctrlm_rf4ce_rib_attr_t(MEMORY_DUMP_ID, MEMORY_DUMP_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER)
{
    this->crash_dump_buf = NULL;
    this->crash_dump_size   = 0;
    this->crash_dump_expected_size = 0;
    this->controller = controller;
}

ctrlm_rf4ce_controller_memory_dump_t::~ctrlm_rf4ce_controller_memory_dump_t() {
    if(this->crash_dump_buf) {
        free(this->crash_dump_buf);
        this->crash_dump_buf = NULL;
    }
}

std::string ctrlm_rf4ce_controller_memory_dump_t::to_string() const {
    //empty on purpose.. nothing to dump
    return("");
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_controller_memory_dump_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
    int buf_index = 0;
    uint16_t memory_size = 0;
    errno_t safec_rc = -1;
    if(len == CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_DUMP) {
        XLOGD_INFO("Index %u: %d bytes received", index, len);
        // first chunk of data, Index 0 of memory dump RIB
        if(index == 0) {
            // check if ctrlm_db_rf4ce_write_file() was never called and memory was not freed
            if (crash_dump_buf != NULL) {
                XLOGD_WARN("Not all the Remote Memory Dump data has been received!");
                free(crash_dump_buf);
                crash_dump_buf = NULL;
            }

            memory_size = (uint16_t)data[CTRLM_CRASH_DUMP_OFFSET_MEMORY_SIZE + 1] << 8 |
                            (uint16_t)data[CTRLM_CRASH_DUMP_OFFSET_MEMORY_SIZE];
            // validate if Memory Size is in valid range
            if(memory_size == 0 || memory_size > CTRLM_CRASH_DUMP_MAX_SIZE) {
                XLOGD_ERROR("Memory Size value %hu is bad", memory_size);
                ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
                return(ret);
            }

            // perform extra checking if Reserved bytes are good
            const uint8_t* data_end =  (uint8_t *)data + len - CTRLM_CRASH_DUMP_OFFSET_RESERVED_0xFF;
            if(data_end == (uint8_t *)std::find((const char*)data + CTRLM_CRASH_DUMP_OFFSET_RESERVED_0xFF, (const char*)data_end, '\xFF')) {
                XLOGD_ERROR("Reserved bytes are bad" );
                ctrlm_print_data_hex(__FUNCTION__, (uint8_t *)data + CTRLM_CRASH_DUMP_OFFSET_RESERVED_0xFF, len - CTRLM_CRASH_DUMP_OFFSET_RESERVED_0xFF, 32);
                ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
                return(ret);
            }

            // normally memory  will be released in ctrlm_db_rf4ce_write_file() after write completes
            crash_dump_buf = (uint8_t *)malloc(memory_size + CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE);
            if(crash_dump_buf == NULL) {
                XLOGD_ERROR("Crash Dump Memory Allocation failed");
                ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
                return(ret);
            }
            crash_dump_expected_size = memory_size;
            crash_dump_size          = 0;

            uint16_t memory_id = (uint16_t)data[CTRLM_CRASH_DUMP_OFFSET_MEMORY_IDENTIFIER + 1] << 8 |
                                (uint16_t)data[CTRLM_CRASH_DUMP_OFFSET_MEMORY_IDENTIFIER];

            XLOGD_INFO("Expected crash dump size %hu bytes. Memory Identifier 0x%04X", memory_size,  memory_id);
            // Fill out File Header
            // signature XDMP
            safec_rc = strcpy_s((char*)crash_dump_buf, memory_size + CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE, "XDMP");
            if(safec_rc != EOK) {
                ERR_CHK(safec_rc);
                free(crash_dump_buf);
                crash_dump_buf = NULL;
                ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
                return(ret);
            }

            ctrlm_sw_version_t sw_version;
            ctrlm_hw_version_t hw_version;
            std::string        device_name;
            if(this->controller) {
                sw_version  = this->controller->version_software_get();
                hw_version  = this->controller->version_hardware_get();
                device_name = ctrlm_rf4ce_controller_type_str(this->controller->controller_type_get());
            }

            // header size
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_HDR_SIZE]     = CTRLM_CRASH_DUMP_FILE_HDR_SIZE & 0xFF;
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_HDR_SIZE + 1] = CTRLM_CRASH_DUMP_FILE_HDR_SIZE >> 8;
            // device ID
            safec_rc = strncpy_s((char *)crash_dump_buf + CTRLM_CRASH_DUMP_FILE_OFFSET_DEVICE_ID, memory_size + CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE - CTRLM_CRASH_DUMP_FILE_OFFSET_DEVICE_ID, device_name.c_str(), CTRLM_CRASH_DUMP_FILE_LEN_DEVICE_ID);
            if(safec_rc != EOK) {
                ERR_CHK(safec_rc);
                free(crash_dump_buf);
                crash_dump_buf = NULL;
                ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
                return(ret);
            }
            // HW Version
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_HW_VER]     = hw_version.get_manufacturer() << 4 | hw_version.get_model();
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_HW_VER + 1] = hw_version.get_revision();
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_HW_VER + 2] = hw_version.get_lot() >> 8;
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_HW_VER + 3] = hw_version.get_lot() & 0xFF;
            // SW Version
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_SW_VER]     = sw_version.get_major();
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_SW_VER + 1] = sw_version.get_minor();
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_SW_VER + 2] = sw_version.get_revision();
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_SW_VER + 3] = sw_version.get_patch();
            // Num dumps in file
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_NUM_DUMPS]     = 1; // for now file is written when first dump is received
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_OFFSET_NUM_DUMPS + 1] = 0;
            // Remote Crash FILE Data Header
            // Memory Identifier
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_MEM_ID]     = memory_id & 0xFF;
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_MEM_ID + 1] = memory_id >> 8;
            // offset to dump data
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_DATA]     = (CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE) & 0xFF;
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_DATA + 1] = (CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE) >> 8  & 0xFF;
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_DATA + 2] = (CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE) >> 16 & 0xFF;
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_DATA + 3] = (CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE) >> 24;
            // dump data length
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_DATA_LEN]     = crash_dump_expected_size & 0xFF;
            crash_dump_buf[CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_OFFSET_DATA_LEN + 1] = crash_dump_expected_size >> 8;
        } else {
            // Process Index 1-255
            // Sanity check
            if(crash_dump_buf == NULL || crash_dump_expected_size <= crash_dump_size) {
                XLOGD_ERROR("Unexpected data chunk!" );
                ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
                return(ret);
            }
            // If the memory dump is not a multiple of 16, the final index of the memory dump MUST be structured as:
            // Bytes (Memory Size % 16) - Memory Data
            // Bytes 16 - (Memory Size % 16) - Reserved (0xFF)
            if(crash_dump_size + len > crash_dump_expected_size) {
                len = crash_dump_expected_size % CTRLM_RF4CE_RIB_ATTR_LEN_MEMORY_DUMP;
            }

            buf_index = CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE + crash_dump_size;
            safec_rc = memcpy_s(&crash_dump_buf[buf_index],memory_size + CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE - buf_index, data, len);
            if(safec_rc != EOK) {
                ERR_CHK(safec_rc);
                ctrlm_hal_free(crash_dump_buf);
                crash_dump_buf = NULL;
                ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
                return(ret);
            }
            crash_dump_size += len;

            // Check if transfer is completed
            if(crash_dump_size == crash_dump_expected_size) {
                // generate binary UUID
                uuid_t uuid_binary_value;
                uuid_generate(uuid_binary_value);
                // convert to string
                char uuid_string[64];
                uuid_unparse_lower(uuid_binary_value, uuid_string);
                // generate full path name
                std::stringstream dump_path;
                dump_path << ctrlm_minidump_path_get() << '/' <<  uuid_string << ".xr";
                // handle writing to file on db thread to release RIB thread
                ctrlm_db_rf4ce_write_file(dump_path.str().c_str(), crash_dump_buf, crash_dump_size + CTRLM_CRASH_DUMP_FILE_HDR_SIZE + CTRLM_CRASH_DUMP_DATA_HDR_SIZE);
                crash_dump_buf = NULL;
            }
        }
    } else {
        XLOGD_ERROR("wrong length <%s>", this->get_name());
        ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
    }
    return(ret);
}
// end ctrlm_rf4ce_controller_memory_dump_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_rf4ce_product_name_t
#define PRODUCT_NAME_MAX_LEN                               (20)
#define RF4CE_PRODUCT_NAME_UNKNOWN                         ("UNKNOWN")
#define RF4CE_PRODUCT_NAME_IMPORT_ERROR                    ("IMPORT")
#define RF4CE_PRODUCT_NAME_XR11                            ("XR11-20")
#define RF4CE_PRODUCT_NAME_XR15                            ("XR15-10")
#define RF4CE_PRODUCT_NAME_XR15V2                          ("XR15-20")
#define RF4CE_PRODUCT_NAME_XR15V2_TYPE_Z                   ("XR15-20Z")
#define RF4CE_PRODUCT_NAME_XR16                            ("XR16-10")
#define RF4CE_PRODUCT_NAME_XR16_TYPE_Z                     ("XR16-10Z")
#define RF4CE_PRODUCT_NAME_XR19                            ("XR19-10")
#define RF4CE_PRODUCT_NAME_XR5                             ("XR5-40")
#define RF4CE_PRODUCT_NAME_XRA                             ("XRA-10")

#define PRODUCT_NAME_ID    (0x32)
#define PRODUCT_NAME_INDEX (0x00)
ctrlm_rf4ce_product_name_t::ctrlm_rf4ce_product_name_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_string_db_attr_t("Product Name", RF4CE_PRODUCT_NAME_UNKNOWN, net, id, "product_name"),
ctrlm_rf4ce_rib_attr_t(PRODUCT_NAME_ID, PRODUCT_NAME_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER) {
    this->set_name_prefix("Controller ");
}

ctrlm_rf4ce_product_name_t::~ctrlm_rf4ce_product_name_t() {

}

void ctrlm_rf4ce_product_name_t::set_product_name(std::string product_name) {
    ctrlm_string_db_attr_t::set_value(product_name);
    if(this->value.length() > 20) {
        this->value = this->value.substr(20);
    }
    if(this->updated_listener) {
        XLOGD_INFO("calling updated listener for %s", this->get_name().c_str());
        this->updated_listener(*this);
    }
}

std::string ctrlm_rf4ce_product_name_t::get_predicted_chipset() const {
    std::string ret = "UNKNOWN";
    if(this->value.find("XR11") != std::string::npos ||
       this->value.find("XR5")  != std::string::npos ||
       this->value.find("XR2")  != std::string::npos) {
        ret = "TI";
    } else if(this->value.find("XR15") != std::string::npos ||
              this->value.find("XR16") != std::string::npos ||
              this->value.find("XR18") != std::string::npos ||
              this->value.find("XR19") != std::string::npos ||
              this->value.find("XRA") != std::string::npos) {
        ret = "QORVO";
    }
    return(ret);
}

void ctrlm_rf4ce_product_name_t::set_updated_listener(ctrlm_rf4ce_product_name_listener_t listener) {
    this->updated_listener = listener;
}

bool ctrlm_rf4ce_product_name_t::to_buffer(char *data, size_t len) {
    bool ret = false;
    if(len >= this->value.length()) {
        memcpy(data, this->value.data(), this->value.length());
        ret = true;
    }
    return(ret);
}

bool ctrlm_rf4ce_product_name_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;

    if (ctrlm_string_db_attr_t::read_db(ctx)) {
        if(this->updated_listener) {
            XLOGD_INFO("calling updated listener for %s", this->get_name().c_str());
            this->updated_listener(*this);
        }
        ret = true;
    }
    return(ret);
}

bool ctrlm_rf4ce_product_name_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->get_key(), this->get_table());
    // NEED TO PUT 20 BYTE BUFFER IN DB FOR BACKWARDS COMPAT
    char buf[PRODUCT_NAME_MAX_LEN];
    
    memset(buf, 0, sizeof(buf));
    std::copy(this->value.begin(), this->value.end(), buf);

    if(blob.from_buffer(buf, sizeof(buf))) {
        if(blob.write_db(ctx)) {
            ret = true;
            XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert to blob <%s>", this->get_name().c_str());
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_product_name_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(this->to_buffer(data, *len)) {
            *len = this->value.length();
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_product_name_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len <= 20) {
            std::string temp = this->value;
            this->value = std::string(data);
            if(this->value.length() > 20) {
                XLOGD_WARN("copied more than max allowed bytes, using substr");
                this->value = this->value.substr(20);
            }
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s write to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(this->value != temp) {
                if(false == importing) {
                    ctrlm_db_attr_write(shared_from_this());
                }
                if(this->updated_listener) {
                    XLOGD_INFO("calling updated listener for %s", this->get_name().c_str());
                    this->updated_listener(*this);
                }
            }
        } else {
            XLOGD_ERROR("data is invalid size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

void ctrlm_rf4ce_product_name_t::export_rib(rf4ce_rib_export_api_t export_api) {
    char buf[PRODUCT_NAME_MAX_LEN];
    if(this->to_buffer(buf, sizeof(buf))) {
        export_api(this->get_identifier(), (uint8_t)this->get_index(), (uint8_t *)buf, (uint8_t)sizeof(buf));
    }
}
// end ctrlm_rf4ce_product_name_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_rf4ce_rib_entries_updated_t
#define RIB_ENTRIES_UPDATED_ID    (0x1A)
#define RIB_ENTRIES_UPDATED_INDEX (0x00)
#define RIB_ENTRIES_UPDATED_LEN   (1)
ctrlm_rf4ce_rib_entries_updated_t::ctrlm_rf4ce_rib_entries_updated_t() :
ctrlm_attr_t("RIB Entries Updated"),
ctrlm_rf4ce_rib_attr_t(RIB_ENTRIES_UPDATED_ID, RIB_ENTRIES_UPDATED_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_TARGET)
{
    this->reu = ctrlm_rf4ce_rib_entries_updated_t::updated::UPDATED_TRUE; // Old code defaulted to true.
}

ctrlm_rf4ce_rib_entries_updated_t::~ctrlm_rf4ce_rib_entries_updated_t() {

}

void ctrlm_rf4ce_rib_entries_updated_t::voice_command_length_updated(const ctrlm_rf4ce_voice_command_length_t& length) {
    this->reu = ctrlm_rf4ce_rib_entries_updated_t::updated::UPDATED_TRUE;
    XLOGD_INFO("%s was updated, %s set to %s", length.get_name().c_str(), this->get_name().c_str(), this->to_string().c_str());
}

std::string ctrlm_rf4ce_rib_entries_updated_t::updated_str(ctrlm_rf4ce_rib_entries_updated_t::updated u) {
    std::stringstream ss; ss << "INVALID";
    switch(u) {
        case ctrlm_rf4ce_rib_entries_updated_t::updated::UPDATED_FALSE: {ss.str("FALSE"); break;}
        case ctrlm_rf4ce_rib_entries_updated_t::updated::UPDATED_TRUE:  {ss.str("TRUE");  break;}
        case ctrlm_rf4ce_rib_entries_updated_t::updated::UPDATED_STOP:  {ss.str("STOP");  break;}
        default: {ss << " <" << (int)u << ">"; break;}
    }
    return(ss.str());
}

std::string ctrlm_rf4ce_rib_entries_updated_t::to_string() const {
    return(this->updated_str(this->reu));
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_rib_entries_updated_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(*len >= RIB_ENTRIES_UPDATED_LEN) {
            data[0]   = this->reu & 0xFF;
            *len      = RIB_ENTRIES_UPDATED_LEN;
            ret       = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            this->reu = ctrlm_rf4ce_rib_entries_updated_t::updated::UPDATED_FALSE;
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_rib_entries_updated_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == RIB_ENTRIES_UPDATED_LEN) {
            this->reu = (ctrlm_rf4ce_rib_entries_updated_t::updated)data[0];
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s written to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is wrong size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}
// end ctrlm_rf4ce_rib_entries_updated_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_rf4ce_controller_capabilities_t
#define CONTROLLER_CAPABILITIES_ID    (0x0C)
#define CONTROLLER_CAPABILITIES_INDEX (0x00)
#define CONTROLLER_CAPABILITIES_LEN   (8)

#define BYTE0_FLAGS_FMR          (0x01)
#define BYTE0_FLAGS_PAR          (0x02)
#define BYTE0_FLAGS_HAPTICS      (0x04)
ctrlm_rf4ce_controller_capabilities_t::ctrlm_rf4ce_controller_capabilities_t(ctrlm_obj_network_t *net, ctrlm_controller_id_t id) :
ctrlm_controller_capabilities_t(),
ctrlm_db_attr_t(net, id),
ctrlm_rf4ce_rib_attr_t(CONTROLLER_CAPABILITIES_ID, CONTROLLER_CAPABILITIES_INDEX, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_BOTH, ctrlm_rf4ce_rib_attr_t::permission::PERMISSION_CONTROLLER) {

}

ctrlm_rf4ce_controller_capabilities_t::~ctrlm_rf4ce_controller_capabilities_t() {

}

bool ctrlm_rf4ce_controller_capabilities_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob("controller_capabilities", this->get_table());
    char buf[CONTROLLER_CAPABILITIES_LEN];

    memset(buf, 0, sizeof(buf));
    if(blob.read_db(ctx)) {
        size_t len = blob.to_buffer(buf, sizeof(buf));
        if(len >= 0) {
            if(len >= CONTROLLER_CAPABILITIES_LEN) {
                if(buf[0] & BYTE0_FLAGS_FMR) {
                    this->add_capability(ctrlm_controller_capabilities_t::capability::FMR);
                }
                if(buf[0] & BYTE0_FLAGS_PAR) {
                    this->add_capability(ctrlm_controller_capabilities_t::capability::PAR);
                }
                if(buf[0] & BYTE0_FLAGS_HAPTICS) {
                    this->add_capability(ctrlm_controller_capabilities_t::capability::HAPTICS);
                }
                ret = true;
                XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
            } else {
                XLOGD_ERROR("data from db is too small <%s>", this->get_name().c_str());
            }
        } else {
            XLOGD_ERROR("failed to convert blob to buffer <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_rf4ce_controller_capabilities_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob("controller_capabilities", this->get_table());
    char buf[CONTROLLER_CAPABILITIES_LEN];

    memset(buf, 0, sizeof(buf));
    if(this->has_capability(ctrlm_controller_capabilities_t::capability::FMR)) {
        buf[0] |= BYTE0_FLAGS_FMR;
    }
    if(this->has_capability(ctrlm_controller_capabilities_t::capability::PAR)) {
        buf[0] |= BYTE0_FLAGS_PAR;
    }
    if(this->has_capability(ctrlm_controller_capabilities_t::capability::HAPTICS)) {
        buf[0] |= BYTE0_FLAGS_HAPTICS;
    }
    if(blob.from_buffer(buf, sizeof(buf))) {
        if(blob.write_db(ctx)) {
            ret = true;
            XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert buffer to blob <%s>", this->get_name().c_str());
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_controller_capabilities_t::read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data && len) {
        if(*len >= CONTROLLER_CAPABILITIES_LEN) {
            if(this->has_capability(ctrlm_controller_capabilities_t::capability::FMR)) {
                data[0] |= BYTE0_FLAGS_FMR;
            }
            if(this->has_capability(ctrlm_controller_capabilities_t::capability::PAR)) {
                data[0] |= BYTE0_FLAGS_PAR;
            }
            if(this->has_capability(ctrlm_controller_capabilities_t::capability::HAPTICS)) {
                data[0] |= BYTE0_FLAGS_HAPTICS;
            }
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s read from RIB: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("buffer is not large enough <%d>", *len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data and/or length is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

ctrlm_rf4ce_rib_attr_t::status ctrlm_rf4ce_controller_capabilities_t::write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing) {
    ctrlm_rf4ce_rib_attr_t::status ret = ctrlm_rf4ce_rib_attr_t::status::FAILURE;
    if(data) {
        if(len == CONTROLLER_CAPABILITIES_LEN) {
            ctrlm_controller_capabilities_t temp(*this);
            if(data[0] & BYTE0_FLAGS_FMR) {
                this->add_capability(ctrlm_controller_capabilities_t::capability::FMR);
            }
            if(data[0] & BYTE0_FLAGS_PAR) {
                this->add_capability(ctrlm_controller_capabilities_t::capability::PAR);
            }
            if(data[0] & BYTE0_FLAGS_HAPTICS) {
                this->add_capability(ctrlm_controller_capabilities_t::capability::HAPTICS);
            }
            ret = ctrlm_rf4ce_rib_attr_t::status::SUCCESS;
            XLOGD_INFO("%s write to RIB: %s", this->get_name().c_str(), this->to_string().c_str());
            if(temp != *this) {
                ctrlm_db_attr_write(shared_from_this());
            }
        } else {
            XLOGD_ERROR("data is invalid size <%d>", len);
            ret = ctrlm_rf4ce_rib_attr_t::status::WRONG_SIZE;
        }
    } else {
        XLOGD_ERROR("data is NULL");
        ret = ctrlm_rf4ce_rib_attr_t::status::INVALID_PARAM;
    }
    return(ret);
}

// end ctrlm_rf4ce_controller_capabilities_t

// class ctrlm_rf4ce_controller_capabilities_t : public ctrlm_controller_capabilities_t, public ctrlm_db_attr_t, public ctrlm_rf4ce_rib_attr_t {
// public:
//     ctrlm_rf4ce_controller_capabilities_t(ctrlm_obj_network_t *net = NULL, ctrlm_controller_id_t id = 0xFF);
//     virtual ~ctrlm_rf4ce_controller_capabilities_t();


// public:
//     /**
//      * Interface implementation to read the data from DB
//      * @see ctrlm_db_attr_t::read_db
//      */
//     virtual bool read_db(ctrlm_db_ctx_t ctx);
//     /**
//      * Interface implementation to write the data to DB
//      * @see ctrlm_db_attr_t::write_db
//      */
//     virtual bool write_db(ctrlm_db_ctx_t ctx);

// public:
//     /**
//      * Interface implementation for a RIB read
//      * @see ctrlm_rf4ce_rib_attr_t::read_rib
//      */
//     virtual ctrlm_rf4ce_rib_attr_t::status read_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t *len);
//     /**
//      * Interface implementation for a RIB write
//      * @see ctrlm_rf4ce_rib_attr_t::write_rib
//      */
//     virtual ctrlm_rf4ce_rib_attr_t::status write_rib(ctrlm_rf4ce_rib_attr_t::access accessor, rf4ce_rib_attr_index_t index, char *data, size_t len, bool importing);
// };
