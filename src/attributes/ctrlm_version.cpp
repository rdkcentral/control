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
#include "ctrlm_version.h"
#include <sstream>
#include "ctrlm_log.h"
#include "safec_lib.h"

#define MAX_REVISION_COMPONENTS 4


// ctrlm_version_t
ctrlm_version_t::ctrlm_version_t() : ctrlm_attr_t("Version") {

}

ctrlm_version_t::~ctrlm_version_t() {
    
}
// end ctrlm_version_t

// ctrlm_sw_version_t
ctrlm_sw_version_t::ctrlm_sw_version_t(version_num _major, version_num _minor, version_num _revision, version_num _patch, int _num_components) : 
    ctrlm_version_t(),
    major_(_major),
    minor_(_minor),
    revision_(_revision),
    patch_(_patch),
    num_components_(_num_components)
{
    this->set_name_prefix("SW ");
}

ctrlm_sw_version_t::ctrlm_sw_version_t(const ctrlm_sw_version_t &version) : 
    ctrlm_version_t(),
    major_(version.major_),
    minor_(version.minor_),
    revision_(version.revision_),
    patch_(version.patch_),
    num_components_(version.num_components_)
{
    this->set_name_prefix("SW ");
}

ctrlm_sw_version_t::ctrlm_sw_version_t(const std::string version) : 
    ctrlm_version_t()
{
    if (false == this->from_string(version)) {
        XLOGD_ERROR("failed to load version from string <%s>", version.c_str());
    }
    this->set_name_prefix("SW ");
}

ctrlm_sw_version_t::~ctrlm_sw_version_t() {
    
}

version_num ctrlm_sw_version_t::get_major() const {
    return(this->major_);
}

version_num ctrlm_sw_version_t::get_minor() const {
    return(this->minor_);
}

version_num ctrlm_sw_version_t::get_revision() const {
    return(this->revision_);
}

version_num ctrlm_sw_version_t::get_patch() const {
    return(this->patch_);
}

void ctrlm_sw_version_t::set_major(version_num major) {
    this->major_ = major;
}

void ctrlm_sw_version_t::set_minor(version_num minor) {
    this->minor_ = minor;
}

void ctrlm_sw_version_t::set_revision(version_num revision) {
    this->revision_ = revision;
}

void ctrlm_sw_version_t::set_patch(version_num patch) {
    this->patch_ = patch;
}

void ctrlm_sw_version_t::set_num_components(int num) {
    this->num_components_ = num;
}

version_software_t ctrlm_sw_version_t::to_versiont() const {
    version_software_t version;
    version.major    = this->major_    & 0xFF;
    version.minor    = this->minor_    & 0xFF;
    version.revision = this->revision_ & 0xFF;
    version.patch    = this->patch_    & 0xFF;
    return(version);
}

std::string ctrlm_sw_version_t::to_string() const {
    std::stringstream ss;
    if (num_components_ >= 1) {
        ss << this->major_;
    }
    if (num_components_ >= 2) {
        ss << "." << this->minor_;
    }
    if (num_components_ >= 3) {
        ss << "." << this->revision_;
    }
    if (num_components_ >= 4) {
        ss << "." << this->patch_;
    }
    return(ss.str());
}

bool ctrlm_sw_version_t::from_string(const std::string &version_str) {
    std::istringstream iss(version_str);
    std::string token;
    int i = 0;
    unsigned long ul;
    version_num buffer[MAX_REVISION_COMPONENTS];
    errno_t safec_rc = memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    ERR_CHK(safec_rc);

    while (std::getline(iss, token, '.')) {
        if (i >= MAX_REVISION_COMPONENTS) {
            return false;
        }
        errno = 0;
        ul = strtoul (token.c_str(), NULL, 0);
        if (errno) {
            return false;
        }
        buffer[i] = (version_num)ul;
        i++;
    }
    this->major_    = buffer[0];
    this->minor_    = buffer[1];
    this->revision_ = buffer[2];
    this->patch_    = buffer[3];
    this->num_components_ = i;
    return true;
}

// end ctrlm_sw_version_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_hw_version_t
ctrlm_hw_version_t::ctrlm_hw_version_t(version_num _manufacturer, version_num _model, version_num _revision, version_num _lot) : 
    ctrlm_version_t(),
    manufacturer(_manufacturer),
    model(_model),
    revision(_revision),
    lot(_lot)
{
    this->set_name_prefix("HW ");
}

ctrlm_hw_version_t::ctrlm_hw_version_t(const ctrlm_hw_version_t &version) : 
    ctrlm_version_t(),
    manufacturer(version.manufacturer),
    model(version.model),
    revision(version.revision),
    lot(version.lot)
{
    this->set_name_prefix("HW ");
}

ctrlm_hw_version_t::~ctrlm_hw_version_t() {
    
}

version_num ctrlm_hw_version_t::get_manufacturer() const {
    return(this->manufacturer);
}

version_num ctrlm_hw_version_t::get_model() const {
    return(this->model);
}

version_num ctrlm_hw_version_t::get_revision() const {
    return(this->revision);
}

version_num ctrlm_hw_version_t::get_lot() const {
    return(this->lot);
}

void ctrlm_hw_version_t::set_manufacturer(version_num manufacturer) {
    this->manufacturer = manufacturer;
}

void ctrlm_hw_version_t::set_model(version_num model) {
    this->model = model;
}

void ctrlm_hw_version_t::set_revision(version_num revision) {
    this->revision = revision;
}

void ctrlm_hw_version_t::set_lot(version_num lot) {
    this->lot = lot;
}

version_hardware_t ctrlm_hw_version_t::to_versiont() const {
    version_hardware_t version;
    version.manufacturer    = this->manufacturer    & 0xFF;
    version.model           = this->model           & 0xFF;
    version.hw_revision     = this->revision        & 0xFF;
    version.lot_code        = this->lot             & 0xFF;
    return(version);
}

void ctrlm_hw_version_t::from(ctrlm_hw_version_t *v) {
    if(v) {
        this->manufacturer = v->get_manufacturer();
        this->model        = v->get_model();
        this->revision     = v->get_revision();
        this->lot          = v->get_lot();
    }
}

std::string ctrlm_hw_version_t::to_string() const {
    std::stringstream ss;
    ss << this->manufacturer << "." << this->model << "." << this->revision << "." << this->lot;
    return(ss.str());
}

bool ctrlm_hw_version_t::from_string(const std::string &version_str) {
    std::istringstream iss(version_str);
    std::string token;
    int i = 0;
    unsigned long ul;
    version_num buffer[MAX_REVISION_COMPONENTS];
    errno_t safec_rc = memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    ERR_CHK(safec_rc);

    while (std::getline(iss, token, '.')) {
        if (i >= MAX_REVISION_COMPONENTS) {
            return false;
        }
        errno = 0;
        ul = strtoul (token.c_str(), NULL, 0);
        if (errno) {
            return false;
        }
        buffer[i] = (version_num)ul;
        i++;
    }
    this->manufacturer  = buffer[0];
    this->model         = buffer[1];
    this->revision      = buffer[2];
    this->lot           = buffer[3];
    return true;
}

// end ctrlm_hw_version_t

///////////////////////////////////////////////////////////////////////////////////////////

// ctrlm_sw_build_id_t
ctrlm_sw_build_id_t::ctrlm_sw_build_id_t(const std::string &id) : ctrlm_version_t() {
    this->set_name("Build ID");
    this->id = id;
}

ctrlm_sw_build_id_t::~ctrlm_sw_build_id_t() {

}

std::string ctrlm_sw_build_id_t::get_id() const {
    return(this->id);
}

void ctrlm_sw_build_id_t::set_id(const std::string &id) {
    this->id = id;
}

std::string ctrlm_sw_build_id_t::to_string() const {
    return(this->id);
}
// end ctrlm_sw_build_id_t
