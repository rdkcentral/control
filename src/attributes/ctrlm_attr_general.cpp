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
#include "ctrlm_attr_general.h"
#include "ctrlm_db_types.h"
#include <sstream>
#include "ctrlm_log.h"

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_string_attr_t
ctrlm_string_attr_t::ctrlm_string_attr_t(const std::string &_name, const std::string &_value) :
    ctrlm_attr_t(_name),
    value(_value)
{

}

ctrlm_string_attr_t::~ctrlm_string_attr_t() {

}

void ctrlm_string_attr_t::set_value(const std::string &_value) {
    this->value = _value;
}

std::string ctrlm_string_attr_t::to_string() const {
    return(this->value);
}
// end ctrlm_string_attr_t

//-----------------------------------------------------

// ctrlm_string_db_attr_t
ctrlm_string_db_attr_t::ctrlm_string_db_attr_t(const std::string &_name, const std::string &_value, const std::string &table, const std::string &db_key) :
    ctrlm_string_attr_t(_name, _value),
    ctrlm_db_attr_t(table, db_key)
{
}

ctrlm_string_db_attr_t::ctrlm_string_db_attr_t(const std::string &_name, const std::string &_value, ctrlm_obj_network_t *net, const std::string &db_key) :
    ctrlm_string_attr_t(_name, _value),
    ctrlm_db_attr_t(net, db_key)
{
}

ctrlm_string_db_attr_t::ctrlm_string_db_attr_t(const std::string &_name, const std::string &_value, ctrlm_obj_network_t *net, ctrlm_controller_id_t id, const std::string &db_key) :
    ctrlm_string_attr_t(_name, _value),
    ctrlm_db_attr_t(net, id, db_key)
{
}

ctrlm_string_db_attr_t::~ctrlm_string_db_attr_t() {

}

bool ctrlm_string_db_attr_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->get_key(), this->get_table());

    if(blob.read_db(ctx)) {
        this->value = blob.to_string();
        ret = true;
        XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_string_db_attr_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_blob_t blob(this->get_key(), this->get_table());

    if(blob.from_string(this->value)) {
        if(blob.write_db(ctx)) {
            ret = true;
            XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
        } else {
            XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
        }
    } else {
        XLOGD_ERROR("failed to convert string to blob <%s>", this->get_name().c_str());
    }
    return(ret);
}
// end ctrlm_string_db_attr_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_uint64_attr_t
ctrlm_uint64_attr_t::ctrlm_uint64_attr_t(uint64_t _value, std::string _name) : 
    ctrlm_attr_t(_name),
    value(_value)
{

}

ctrlm_uint64_attr_t::~ctrlm_uint64_attr_t() {

}

void ctrlm_uint64_attr_t::set_value(uint64_t _value) {
    this->value = _value;
}

uint64_t ctrlm_uint64_attr_t::get_value() const {
    return this->value;
}

std::string ctrlm_uint64_attr_t::to_string() const {
    return(std::to_string(this->value));
}
// end ctrlm_string_attr_t

//-----------------------------------------------------

// ctrlm_uint64_db_attr_t
ctrlm_uint64_db_attr_t::ctrlm_uint64_db_attr_t(const std::string &_name, uint64_t _value, const std::string &table, const std::string &db_key) :
    ctrlm_uint64_attr_t(_value, _name),
    ctrlm_db_attr_t(table, db_key)
{
}
ctrlm_uint64_db_attr_t::ctrlm_uint64_db_attr_t(const std::string &_name, uint64_t _value, ctrlm_obj_network_t *net, const std::string &db_key) :
    ctrlm_uint64_attr_t(_value, _name),
    ctrlm_db_attr_t(net, db_key)
{
}
ctrlm_uint64_db_attr_t::ctrlm_uint64_db_attr_t(const std::string &_name, uint64_t _value, ctrlm_obj_network_t *net, ctrlm_controller_id_t id, const std::string &db_key) :
    ctrlm_uint64_attr_t(_value, _name),
    ctrlm_db_attr_t(net, id, db_key)
{
}

ctrlm_uint64_db_attr_t::~ctrlm_uint64_db_attr_t() {

}

bool ctrlm_uint64_db_attr_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_uint64_t data(this->get_key(), this->get_table());
    if(data.read_db(ctx)) {
        this->set_value(data.get_uint64());
        ret = true;
        XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_uint64_db_attr_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_uint64_t data(this->get_key(), this->get_table(), this->get_value());
    if(data.write_db(ctx)) {
        ret = true;
        XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
    } else {
        XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
    }
    return(ret);
}

// end ctrlm_uint64_db_attr_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_ieee_addr_t
ctrlm_ieee_addr_t::ctrlm_ieee_addr_t(uint64_t ieee) : 
    ctrlm_uint64_attr_t(ieee, "IEEE Address"),
    num_bytes(8)
{
}

ctrlm_ieee_addr_t::~ctrlm_ieee_addr_t() {

}

std::string ctrlm_ieee_addr_t::to_string() const {
    return ctrlm_ieee_addr_t::to_string(true);
}

std::string ctrlm_ieee_addr_t::to_string(bool colons) const {
    std::stringstream ret;
    int max = num_bytes - 1;
    for(int i = max; i >= 0; i--) {
        unsigned char octet = (this->value >> (8 * i)) & 0xFF;
        if(i != max && colons) {
            ret << ":";
        }
        ret << COUT_HEX_MODIFIER << (int)octet;
    }
    return(ret.str());
}

void ctrlm_ieee_addr_t::set_num_bytes(uint8_t _num_bytes) {
    num_bytes = _num_bytes;
}
// end ctrlm_ieee_addr_t

//-----------------------------------------------------

// ctrlm_ieee_db_addr_t
ctrlm_ieee_db_addr_t::ctrlm_ieee_db_addr_t(uint64_t ieee, ctrlm_obj_network_t *net, const std::string &db_key) :
    ctrlm_ieee_addr_t(ieee),
    ctrlm_db_attr_t(net, db_key)
{
}
ctrlm_ieee_db_addr_t::ctrlm_ieee_db_addr_t(uint64_t ieee, ctrlm_obj_network_t *net, ctrlm_controller_id_t id, const std::string &db_key) :
    ctrlm_ieee_addr_t(ieee),
    ctrlm_db_attr_t(net, id, db_key)
{
}

ctrlm_ieee_db_addr_t::~ctrlm_ieee_db_addr_t() {

}

bool ctrlm_ieee_db_addr_t::read_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_uint64_t data(this->get_key(), this->get_table());
    if(data.read_db(ctx)) {
        this->set_value(data.get_uint64());
        ret = true;
        XLOGD_INFO("%s read from database: %s", this->get_name().c_str(), this->to_string().c_str());
    } else {
        XLOGD_ERROR("failed to read from db <%s>", this->get_name().c_str());
    }
    return(ret);
}

bool ctrlm_ieee_db_addr_t::write_db(ctrlm_db_ctx_t ctx) {
    bool ret = false;
    ctrlm_db_uint64_t data(this->get_key(), this->get_table(), this->get_value());
    if(data.write_db(ctx)) {
        ret = true;
        XLOGD_INFO("%s written to database: %s", this->get_name().c_str(), this->to_string().c_str());
    } else {
        XLOGD_ERROR("failed to write to db <%s>", this->get_name().c_str());
    }
    return(ret);
}

// end ctrlm_ieee_db_addr_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_audio_profiles_t
ctrlm_audio_profiles_t::ctrlm_audio_profiles_t(int profiles) : ctrlm_attr_t("Audio Profiles") {
    this->supported_profiles = profiles;
}

ctrlm_audio_profiles_t::~ctrlm_audio_profiles_t() {

}

int ctrlm_audio_profiles_t::get_profiles() const {
    return(this->supported_profiles);
}

void ctrlm_audio_profiles_t::set_profiles(int profiles) {
    this->supported_profiles = profiles;
}

bool ctrlm_audio_profiles_t::supports(ctrlm_audio_profiles_t::profile p) const {
    return(this->supported_profiles & p);
}

std::string ctrlm_audio_profiles_t::profile_str(ctrlm_audio_profiles_t::profile p) {
    std::stringstream ss; ss << "INVALID";
    switch(p) {
        case ctrlm_audio_profiles_t::profile::NONE:               {ss.str("NONE"); break;}
        case ctrlm_audio_profiles_t::profile::ADPCM_16BIT_16KHZ:  {ss.str("ADPCM_16BIT_16KHZ"); break;}
        case ctrlm_audio_profiles_t::profile::PCM_16BIT_16KHZ:    {ss.str("PCM_16BIT_16KHZ"); break;}
        case ctrlm_audio_profiles_t::profile::OPUS_16BIT_16KHZ:   {ss.str("OPUS_16BIT_16KHZ"); break;}
        default:  {ss << " (" << (int)p << ")"; break;}
    }
    return(ss.str());
}

std::string ctrlm_audio_profiles_t::to_string() const {
    std::stringstream ss;
    if(this->supported_profiles == ctrlm_audio_profiles_t::profile::NONE) {
        ss << "NONE";
    } else {
        bool comma = false;
        if(this->supported_profiles & ctrlm_audio_profiles_t::profile::ADPCM_16BIT_16KHZ) {
            ss << this->profile_str(ctrlm_audio_profiles_t::profile::ADPCM_16BIT_16KHZ);
            comma = true;
        }
        if(this->supported_profiles & ctrlm_audio_profiles_t::profile::PCM_16BIT_16KHZ) {
            ss << (comma ? "," : "") << this->profile_str(ctrlm_audio_profiles_t::profile::PCM_16BIT_16KHZ);
            comma = true;
        }
        if(this->supported_profiles & ctrlm_audio_profiles_t::profile::OPUS_16BIT_16KHZ) {
            ss << (comma ? "," : "") << this->profile_str(ctrlm_audio_profiles_t::profile::OPUS_16BIT_16KHZ);
            comma = true;
        }
    }
    return(ss.str());
}
// end ctrlm_audio_profiles_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_controller_capabilities_t
ctrlm_controller_capabilities_t::ctrlm_controller_capabilities_t() : ctrlm_attr_t("Controller Capabilities") {
    for(int i = 0; i < ctrlm_controller_capabilities_t::capability::INVALID; i++) {
        this->capabilities[i] = false;
    }
}

ctrlm_controller_capabilities_t::ctrlm_controller_capabilities_t(const ctrlm_controller_capabilities_t& cap) {
    for(int i = 0; i < ctrlm_controller_capabilities_t::capability::INVALID; i++) {
        this->capabilities[i] = cap.has_capability((ctrlm_controller_capabilities_t::capability)i);
    }
}

ctrlm_controller_capabilities_t::~ctrlm_controller_capabilities_t() {

}

bool ctrlm_controller_capabilities_t::operator==(const ctrlm_controller_capabilities_t& cap) {
    bool ret = true;
    for(int i = 0; i < ctrlm_controller_capabilities_t::capability::INVALID; i++) {
        if(this->has_capability((ctrlm_controller_capabilities_t::capability)i) != cap.has_capability((ctrlm_controller_capabilities_t::capability)i)) {
            ret = false;
            break;
        }
    }
    return(ret);
}

bool ctrlm_controller_capabilities_t::operator!=(const ctrlm_controller_capabilities_t& cap) {
    return(!(*this == cap));
}

void ctrlm_controller_capabilities_t::add_capability(ctrlm_controller_capabilities_t::capability c) {
    this->capabilities[c] = true;
}

void ctrlm_controller_capabilities_t::remove_capability(ctrlm_controller_capabilities_t::capability c) {
    this->capabilities[c] = false;
}

bool ctrlm_controller_capabilities_t::has_capability(ctrlm_controller_capabilities_t::capability c) const {
    return(this->capabilities[c]);
}

std::string ctrlm_controller_capabilities_t::capability_str(ctrlm_controller_capabilities_t::capability c) {
    std::stringstream ss; ss << "INVALID";
    switch(c) {
        case ctrlm_controller_capabilities_t::capability::FMR:     {ss.str("FindMyRemote"); break;}
        case ctrlm_controller_capabilities_t::capability::PAR:     {ss.str("PressAndReleaseVoice"); break;}
        case ctrlm_controller_capabilities_t::capability::HAPTICS: {ss.str("Haptics"); break;}
        default: {ss << " <" << (int)c << ">"; break;}
    }
    return(ss.str());
}

std::string ctrlm_controller_capabilities_t::to_string() const {
    std::stringstream ret;
    bool comma = false;
    for(int i = 0; i < ctrlm_controller_capabilities_t::capability::INVALID; i++) {
        if(this->capabilities[i] == true) {
            if(comma) {
                ret << ",";
            }
            ret << this->capability_str((ctrlm_controller_capabilities_t::capability)i);
            comma = true;
        }
    }
    return(ret.str());
}
// end ctrlm_controller_capabilities_t
