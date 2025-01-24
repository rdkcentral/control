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
#ifndef __CTRLM_VERSION_H__
#define __CTRLM_VERSION_H__
#include "ctrlm_attr.h"
#include "ctrlm_log.h"
#include <stdint.h>
#include <string>

typedef uint16_t version_num;

// USED FOR TRANSITION COMPATIABILITY
typedef struct {
   uint8_t  manufacturer;
   uint8_t  model;
   uint8_t  hw_revision;
   uint16_t lot_code;
} version_hardware_t;

typedef struct {
   uint8_t major;
   uint8_t minor;
   uint8_t revision;
   uint8_t patch;
} version_software_t;
// END USED FOR TRANSITION COMPATIABILITY

/**
 * @brief ControlMgr Version Class
 * 
 * This interface is used for all ControlMgr versions (i.e. Controller Software Version). Subclasses will
 * need to implement function to compare versions
 */
class ctrlm_version_t : public ctrlm_attr_t {
public:
    /**
     * ControlMgr Version Default Constructor
     */
    ctrlm_version_t();
    /**
     * ControlMgr Version Destructor
     */
    virtual ~ctrlm_version_t();
};

/**
 * @brief ControlMgr Software Version Class
 * 
 * This class is used within ControlMgr to signify a Remote Software Version.
 */
class ctrlm_sw_version_t : public ctrlm_version_t {
public:
    /**
     * ControlMgr Software Version Default Constructor
     * @param major The major version number
     * @param minor The minor version number
     * @param revision The revision version number
     * @param patch The patch version number
     */
    ctrlm_sw_version_t(version_num _major = 0, version_num _minor = 0, version_num _revision = 0, version_num _patch = 0, int _num_components = 4);
    /**
     * ControlMgr Software Version Copy Constructor
     * @param version Reference to a Software Version object
     */
    ctrlm_sw_version_t(const ctrlm_sw_version_t &version);
    /**
     * ControlMgr Software Version Constructor from String
     * @param version represented as a string
     */
    ctrlm_sw_version_t(const std::string version);
    /**
     * ControlMgr Software Version Destructor
     */
    virtual ~ctrlm_sw_version_t();

public:
    /**
     * Getter function for the major version number
     * @return The major version number
     */
    version_num get_major() const;
    /**
     * Getter function for the minor version number
     * @return The minor version number
     */
    version_num get_minor() const;
    /**
     * Getter function for the revision version number
     * @return The revision version number
     */
    version_num get_revision() const;
    /**
     * Getter function for the patch version number
     * @return The patch version number
     */
    version_num get_patch() const;
    /**
     * Setter function for the major version number
     * @param major The major version number
     */
    void set_major(version_num major);
    /**
     * Setter function for the minor version number
     * @param minor The minor version number
     */
    void set_minor(version_num minor);
    /**
     * Setter function for the revision version number
     * @param revision The revision version number
     */
    void set_revision(version_num revision);
    /**
     * Setter function for the patch version number
     * @param patch The major patch number
     */
    void set_patch(version_num patch);
    /**
     * Setter function for num_components
     * @param num the number of categories this version object uses.  For example, if this
     * object only uses major and minor, then num_components should be set to 2 so that
     * the to_string function does not print the unused revision and patch.
     */
    void set_num_components(int num);

    /**
     * Function used for transitioning from the old to new version structures
     * @return The version represented in the legacy format 
     * @todo remove once the re-factor is complete
     */
    version_software_t to_versiont() const;

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the version number formatted as "X.X.X.X"
     */
    virtual std::string to_string() const;

    /**
     * load version components based on a string of the form "X.X.X.X"
     */
    bool from_string(const std::string &version_str);

protected:
    version_num major_;
    version_num minor_;
    version_num revision_;
    version_num patch_;
    int num_components_;
};

inline bool operator==(const ctrlm_sw_version_t& v1, const ctrlm_sw_version_t& v2) {
    return((v1.get_major()    == v2.get_major())    &&
           (v1.get_minor()    == v2.get_minor())    &&
           (v1.get_revision() == v2.get_revision()) &&
           (v1.get_patch()    == v2.get_patch()));
}

inline bool operator!=(const ctrlm_sw_version_t& v1, const ctrlm_sw_version_t& v2) {
    return(!(v1 == v2));
}

inline bool operator>(const ctrlm_sw_version_t& v1, const ctrlm_sw_version_t& v2) {
    bool ret = true;
    if(v1.get_major() == v2.get_major()) {
        if(v1.get_minor() == v2.get_minor()) {
            if(v1.get_revision() == v2.get_revision()) {
                ret = v1.get_patch() > v2.get_patch();
            } else {
                ret = v1.get_revision() > v2.get_revision();
            }
        } else {
            ret = v1.get_minor() > v2.get_minor();
        }
    } else {
        ret = v1.get_major() > v2.get_major();
    }
    return(ret);
}

inline bool operator<(const ctrlm_sw_version_t& v1, const ctrlm_sw_version_t& v2) {
    return(!(v1 == v2) && !(v1 > v2));
}

inline bool operator>=(const ctrlm_sw_version_t& v1, const ctrlm_sw_version_t& v2) {
    return((v1 == v2) || (v1 > v2));
}

inline bool operator<=(const ctrlm_sw_version_t& v1, const ctrlm_sw_version_t& v2) {
    return((v1 == v2) || (v1 < v2));
}

/**
 * @brief ControlMgr Hardware Version Class
 * 
 * This class is used within ControlMgr to signify a Remote Hardware Version.
 */
class ctrlm_hw_version_t : public ctrlm_version_t {
public:
    /**
     * ControlMgr Hardware Version Default Constructor
     * @param manufacturer The manufacturer version number
     * @param model The model version number
     * @param revision The revision version number
     * @param lot The lot version number
     */
    ctrlm_hw_version_t(version_num manufacturer = 0, version_num model = 0, version_num revision = 0, version_num lot = 0);
    /**
     * ControlMgr Hardware Version Copy Constructor
     * @param version Reference to a Hardware Version object
     */
    ctrlm_hw_version_t(const ctrlm_hw_version_t &version);
    /**
     * ControlMgr Hardware Version Destructor
     */
    virtual ~ctrlm_hw_version_t();

public:
    /**
     * Getter function for the manufactuer version number
     * @return The manufactuer version number
     */
    version_num get_manufacturer() const;
    /**
     * Getter function for the model version number
     * @return The model version number
     */
    version_num get_model() const;
    /**
     * Getter function for the revision version number
     * @return The revision version number
     */
    version_num get_revision() const;
    /**
     * Getter function for the lot version number
     * @return The lot version number
     */
    version_num get_lot() const;
    /**
     * Setter function for the manufacturer version number
     * @param manufacturer The manufacturer version number
     */
    void set_manufacturer(version_num manufacturer);
    /**
     * Setter function for the model version number
     * @param model The model version number
     */
    void set_model(version_num model);
    /**
     * Setter function for the revision version number
     * @param revision The revision version number
     */
    void set_revision(version_num revision);
    /**
     * Setter function for the lot version number
     * @param lot The major lot number
     */
    void set_lot(version_num lot);
    /**
     * Function used for transitioning from the old to new version structures
     * @return The version represented in the legacy format 
     * @todo remove once the re-factor is complete
     */
    version_hardware_t to_versiont() const;
    /**
     * Function to set internal data equal to another hardware version (this is needed for class extensions)
     * @param v The hardware version to copy
     */
    void from(ctrlm_hw_version_t *v);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the version number formatted as "X.X.X.X"
     */
    std::string to_string() const;

    /**
     * load version components based on a string of the form "X.X.X.X"
     */
    bool from_string(const std::string &version_str);

protected:
    version_num manufacturer;
    version_num model;
    version_num revision;
    version_num lot;
};

inline bool operator==(const ctrlm_hw_version_t& v1, const ctrlm_hw_version_t& v2) {
    return((v1.get_manufacturer()    == v2.get_manufacturer())    &&
           (v1.get_model()           == v2.get_model())           &&
           (v1.get_revision()        == v2.get_revision()));
}

inline bool operator!=(const ctrlm_hw_version_t& v1, const ctrlm_hw_version_t& v2) {
    return(!(v1 == v2));
}

inline bool operator>(const ctrlm_hw_version_t& v1, const ctrlm_hw_version_t& v2) {
    if((v1.get_manufacturer() != v2.get_manufacturer()) ||
       (v1.get_model()        != v2.get_model())) {
        XLOGD_WARN("manufacturer / model mismatch");
    }
    return(v1.get_revision() > v2.get_revision());
}

inline bool operator<(const ctrlm_hw_version_t& v1, const ctrlm_hw_version_t& v2) {
    return(!(v1 == v2) && !(v1 > v2));
}

inline bool operator>=(const ctrlm_hw_version_t& v1, const ctrlm_hw_version_t& v2) {
    return((v1 == v2) || (v1 > v2));
}

inline bool operator<=(const ctrlm_hw_version_t& v1, const ctrlm_hw_version_t& v2) {
    return((v1 == v2) || (v1 < v2));
}

/**
 * @brief ControlMgr Build ID Version Class
 * 
 * This class is used within ControlMgr to signify a Remote Build ID.
 */
class ctrlm_sw_build_id_t : public ctrlm_version_t {
public:
    /**
     * ControlMgr Software Build ID Default Constructor
     * @param id The software build id
     */
    ctrlm_sw_build_id_t(const std::string &id = "");
    /**
     * ControlMgr Software Build ID Destructor
     */
    virtual ~ctrlm_sw_build_id_t();

public:
    /**
     * Getter function for the build id
     * @return The build id
     */
    std::string get_id() const;
    /**
     * Setter function for the build id
     * @param id The build id
     */
    void set_id(const std::string &id);

public:
    /**
     * Implementation of the ctrlm_attr_t to_string interface
     * @return String containing the build id
     */
    std::string to_string() const;

protected:
    std::string id;
};

inline bool operator==(const ctrlm_sw_build_id_t& v1, const ctrlm_sw_build_id_t& v2) {
    return(v1.get_id() == v2.get_id());
}

inline bool operator!=(const ctrlm_sw_build_id_t& v1, const ctrlm_sw_build_id_t& v2) {
    return(!(v1 == v2));
}

#endif
