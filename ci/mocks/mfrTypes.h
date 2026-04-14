/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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
 * @see the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @addtogroup HPK Hardware Porting Kit
 * @{
 * @par The Hardware Porting Kit
 * HPK is the next evolution of the well-defined Hardware Abstraction Layer
 * (HAL), but augmented with more comprehensive documentation and test suites
 * that OEM or SOC vendors can use to self-certify their ports before taking
 * them to RDKM for validation or to an operator for final integration and
 * deployment. The Hardware Porting Kit effectively enables an OEM and/or SOC
 * vendor to self-certify their own Video Accelerator devices, with minimal RDKM
 * assistance
 *
 */
/** @defgroup MFR MFR Module
 *  @{
 */
/** @defgroup MFR_HAL MFR HAL
 *  @{
 * @par Application API Specification
 * MFR HAL provides an interface for reading and writing device serialization information and doing image flashing operations
 */

/** @defgroup PLAT_MFR_DATA PLAT MFR DATA
 *  @{
 */

/**
 * @file mfrTypes.h
 * 
 * @brief MFR HAL header
 *
 * This file defines APIs, datatypes and error codes used by the MFR HAL
 *
 * @par Document
 * Document reference.
 *
 * @par Open Issues (in no particular order)
 * -# None
 *
 * @par Assumptions
 * -# None
 *
 * @par Abbreviations
 * - MFR:      Manufacturer library
 * - HDMI:     High-Definition multimedia Interface
 * - HDCP:     High-Bandwidth digital content protection
 * - MOCA:     Multimedia over coax alliance
 * - auth:     Authentication
 * - DTCP:     Digital transmission content protection
 * - CDL:      Code download
 * - RCDL:     Remote code download
 * - CA:       Certificate authority
 * - DVR:      Digital video recording
 * - SVN:      Software version number
 * - CRC:      Cyclic redundancy check
 * - oui:      Organizationally unique identifier
 * - DRI:      Disaster recovery image
 * - PDRI:     Peripheral disaster recovery image
 * - WIFI:     Wireless fidelity
 * - MAC:      Media access control address
 * - RF4CE:    Radio frequency for consumer electronics
 * - DTB:      Device tree binary
 * - PMI:      Product manufacturer information
 * - SOC:      System on chip
 * - TV:       Television
 * - BDRI:     Backup disaster recovery image
 * - CPD:      Critical panel data
 * - WB:       White balancing
 * - ALS:      Ambient light sensor
 * - LUX:      Unit of luminance or illumination of a one metre square area
 * - PCI:      Peripheral component interconnect
 * - AV:       Audio video
 * - TPV:      TPV technology limited
 * - FTA:      Factory test app
 * - WPS:      Wi-Fi protected setup
 */


#ifndef _MFR_TYPES_H
#define _MFR_TYPES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


/**
 * @brief MFR status codes.
 * 
 */
typedef enum _mfrError_t
{
    mfrERR_NONE = 0,                           ///< Input output operation is successful
    mfrERR_GENERAL = 0x1000,                   ///< Operation general error. This enum is deprecated
    mfrERR_INVALID_PARAM,                      ///< Invalid argument is passed to the module
    mfrERR_NOT_INITIALIZED,                    ///< Module is not initialised
    mfrERR_OPERATION_NOT_SUPPORTED,            ///< Not suppoted operation is performed
    mfrERR_UNKNOWN,                            ///< Unknown error. This enum is deprecated
    /* Please add Error Code here */
    mfrERR_MEMORY_EXHAUSTED,                   ///< Memory exhausted
    mfrERR_SRC_FILE_ERROR,                     ///< File related errors
    mfrERR_WRITE_FLASH_FAILED,                 ///< Flash write failed
    mfrERR_UPDATE_BOOT_PARAMS_FAILED,          ///< Boot params update failed
    mfrERR_FAILED_CRC_CHECK,                   ///< CRC check failed
    mfrERR_BAD_IMAGE_HEADER,                   ///< Bad image header error. Invalid Image(Not a valid image to flash in the partition)
    mfrERR_IMPROPER_SIGNATURE,                 ///< Improper signature error. Invalidate section data available in the image
    mfrERR_IMAGE_TOO_BIG,                      ///< Image too big error
    mfrERR_FAILED_INVALID_SIGNING_TIME,        ///< Invalid image signing time value
    mfrERR_FAILED_INVALID_SVN,                 ///< Invalid SVN error 
    mfrERR_FAILED_IMAGE_SIGNING_TIME_OLDER,    ///< Image signing time is older than expected. By comparing the signing time available in flash data with current image timing. return mfrERR_FAILED_IMAGE_SIGNING_TIME_OLDER flash signing image time is older
    mfrERR_FAILED_IMAGE_SVN_OLDER,             ///< SVN is older 
    mfrERR_FAILED_SAME_DRI_CODE_VERSION,       ///< Same DRI trying to write again. Current DRI image is requested to flash again. If curren image is corrupted this operation will corrupt the alternate bank also
    mfrERR_FAILED_SAME_PCI_CODE_VERSION,       ///< Same PCI trying to write again. Current PCI image is requested to flash again. If curren image is corrupted this operation will corrupt the alternate bank also
    mfrERR_IMAGE_FILE_OPEN_FAILED,             ///< Not able to open image file
    mfrERR_GET_FLASHED_IMAGE_DETAILS_FAILED,   ///< Not able to retrieve the flashed image details
    mfrERR_FLASH_VERIFY_FAILED,                ///< Not able to verify the flash
    mfrERR_ALREADY_INITIALIZED,                ///< Module already initialised
    mfrERR_FLASH_READ_FAILED,                  ///< Flash read failed
    mfrERR_FLASH_SOFT_LOCK_FAILED,             ///< Flash soft lock failed
    mfrERR_TEMP_READ_FAILED,                   ///< Temperature read failed
    mfrERR_MAX                                 ///< Out of range - required to be the last item of the enum
} mfrError_t;

/**
 * @brief Serialization data
 * 
 */
typedef struct _mfrSerializedData_t
{
    char * buf;                                ///< Buffer containing the data
    size_t bufLen;                             ///< Length of the data buffer
    void (* freeBuf) (char *buf);              ///< Function used to free the buffer. If NULL, the user does not need to free the buffer
} mfrSerializedData_t;

/**
 * @brief Serialization data types. All values are platform specific
 * 
 * 
 * White balancing calibration for TV sources involves applying calibration to the linear playback streams.
 * White balancing calibration for AV involves applying calibration specifically to the composite source
 * 
 */
typedef enum _mfrSerializedType_t
{
    mfrSERIALIZED_TYPE_MANUFACTURER = 0,             ///< manufacture field. ASCII string
    mfrSERIALIZED_TYPE_MANUFACTUREROUI,              ///< manufacture oui field. HEX string value
    mfrSERIALIZED_TYPE_MODELNAME,                    ///< model name field. ASCII string
    mfrSERIALIZED_TYPE_DESCRIPTION,                  ///< description field. ASCII string
    mfrSERIALIZED_TYPE_PRODUCTCLASS,                 ///< product class field. ASCII string
    mfrSERIALIZED_TYPE_SERIALNUMBER,                 ///< serial number field. Alphanumerical string value
    mfrSERIALIZED_TYPE_HARDWAREVERSION,              ///< Hardware version field. String value
    mfrSERIALIZED_TYPE_SOFTWAREVERSION,              ///< software field. Decimal stirng value
    mfrSERIALIZED_TYPE_PROVISIONINGCODE,             ///< provisioning code field. String value
    mfrSERIALIZED_TYPE_FIRSTUSEDATE,                 ///< first use date field. String value
    mfrSERIALIZED_TYPE_DEVICEMAC,                    ///< device mac field. HEX string MAC value separated with colon
    mfrSERIALIZED_TYPE_MOCAMAC,                      ///< MOCA mac field. HEX string MAC value separated with colon
    mfrSERIALIZED_TYPE_HDMIHDCP,                     ///< HDMI HDCP field. String value

    mfrSERIALIZED_TYPE_PDRIVERSION,                  ///< PDRI version field. It provide the primary Disaster Recovery Image version information
    mfrSERIALIZED_TYPE_WIFIMAC,                      ///< wifi mac field. HEX string MAC value separated with colon
    mfrSERIALIZED_TYPE_BLUETOOTHMAC,                 ///< bluetooth MAC field. HEX string MAC value separated with colon
    mfrSERIALIZED_TYPE_WPSPIN,                       ///< WPS PIN filed. Decimal string
    mfrSERIALIZED_TYPE_MANUFACTURING_SERIALNUMBER,   ///< manufacturing serial number field. Alphanumerical string value
    mfrSERIALIZED_TYPE_ETHERNETMAC,                  ///< ethernet MAC field. HEX string MAC value separated with colon
    mfrSERIALIZED_TYPE_ESTBMAC,                      ///< estb mac field. HEX string MAC value separated with colon. Device MAC is same as the ethernet (mfrSERIALIZED_TYPE_ETHERNETMAC) mac
    mfrSERIALIZED_TYPE_RF4CEMAC,                     ///< RF4CE MAC field. HEX string MAC value separated with colon

    mfrSERIALIZED_TYPE_PROVISIONED_MODELNAME,        ///< provisioned model name field
    mfrSERIALIZED_TYPE_PMI,                          ///< PMI field. Alphanumerical string value of the model
    mfrSERIALIZED_TYPE_HWID,                         ///< hardware ID field. HEX sting value of hardware id
    mfrSERIALIZED_TYPE_MODELNUMBER,                  ///< Model number field. HEX sting value of model number
    /* boot data */
    mfrSERIALIZED_TYPE_SOC_ID,                       ///< SOC id field. String value
    mfrSERIALIZED_TYPE_IMAGENAME,                    ///< image name field. Alphanumerical name of the monolitic image flashed
    mfrSERIALIZED_TYPE_IMAGETYPE,                    ///< image type field. Alphanumerical type name of the image. Eg: PC1
    mfrSERIALIZED_TYPE_BLVERSION,                    ///< boot loader version field. Boot loader version value separated with dots. Eg: 6.9.7
    /* provisional data */
    mfrSERIALIZED_TYPE_REGION,                       ///< region field. String value
    /* other data */
    mfrSERIALIZED_TYPE_BDRIVERSION,                  ///< BDRI version field. It provide the Backup DRI version information

    /* led data */
    mfrSERIALIZED_TYPE_LED_WHITE_LEVEL,              ///< led white level field. String value
    mfrSERIALIZED_TYPE_LED_PATTERN,                  ///< led pattern field. String value
    mfrSERIALIZED_TYPE_MAX,                          ///< Out of range - required to be the last item of the enum
#ifdef PANEL_SERIALIZATION_TYPES
    //As MFR HAL is a precompiled binary across all existing platforms, a distinct region is allocated for panel-based enums, beginning at 0x51. 
    //This approach allows us to utilize the same type field for corresponding APIs.
    mfrSERIALIZED_TYPE_COREBOARD_SERIALNUMBER=0x51,  ///< core board serial number field
    mfrSERIALIZED_TYPE_FACTORYBOOT,                  ///< factory boot field. String value
    mfrSERIALIZED_TYPE_COUNTRYCODE,                  ///< country code field. To configure country code 
    mfrSERIALIZED_TYPE_LANGUAGECODE,                 ///< language code field.To configure language code 
    mfrSERIALIZED_TYPE_MANUFACTURERDATA,             ///< manufacture data field. To configure HDCP filename, PCBA serial number, project id, logo, device lock information 
    mfrSERIALIZED_TYPE_CPD_SIZE,                     ///< CPD size field. It represent the Size of the Critical panel data
    mfrSERIALIZED_TYPE_PANEL_ID,                     ///< panel id field. It is unique ID to represent the each unique version(ex: 43", 55" inches etc) of panel
    mfrSERIALIZED_TYPE_PANEL_TYPE,                   ///< panel type field. Its unique id mapped with each panel ID 
    mfrSERIALIZED_TYPE_PANEL_HDMI_WB_DATA_NORMAL,    ///< HDMI WB data normal field. Standard colour temperature in HDMI source, to adjust r,g,b gain equally to achieve accurate color reproduction.
    mfrSERIALIZED_TYPE_PANEL_HDMI_WB_DATA_COLD,      ///< HDMI WB data cold field. COLD colour temperature in HDMI source, to adjust the blue gain alone to achieve blueish color pattern
    mfrSERIALIZED_TYPE_PANEL_HDMI_WB_DATA_WARM,      ///< HDMI WB data warm field. WARM colour temperature in HDMI source, to adjust the red gain alone to achieve reddish colour pattern
    mfrSERIALIZED_TYPE_PANEL_HDMI_WB_DATA_USER,      ///< HDMI WB data user field. User can change the picture modes as per their requirement in USER colour temperature mode
    mfrSERIALIZED_TYPE_PANEL_TV_WB_DATA_NORMAL,      ///< panel TV WB data normal field Standard colour temperature in TV source, to adjust r,g,b gain equally to achieve accurate color reproduction. Applicable for linear playback streams
    mfrSERIALIZED_TYPE_PANEL_TV_WB_DATA_COLD,        ///< panel TV WB data cold field. COLD colour temperature in TV source, to adjust the blue gain alone to to achieve blueish color pattern. Applicable for linear playback streams
    mfrSERIALIZED_TYPE_PANEL_TV_WB_DATA_WARM,        ///< panel TV WB data warm field. WARM colour temperature in TV source, to adjust the red gain alone to to achieve reddish colour pattern. Applicable for linear playback streams
    mfrSERIALIZED_TYPE_PANEL_TV_WB_DATA_USER,        ///< panel TV WB data user field. User can change the picture modes as per their requirement in USER colour temperature mode. Applicable for linear playback streams
    mfrSERIALIZED_TYPE_PANEL_AV_WB_DATA_NORMAL,      ///< panel AV WB data normal field what is the difference between TV and AV  white balance here. TV targets for Linear playback source and For AV target composite source. Applicable for composite source
    mfrSERIALIZED_TYPE_PANEL_AV_WB_DATA_COLD,        ///< panel AV WB data cold field. COLD colour temperature in AV source, to adjust the blue gain alone to to achieve blueish color pattern. Applicable for composite source
    mfrSERIALIZED_TYPE_PANEL_AV_WB_DATA_WARM,        ///< panel AV WB data warm field. WARM colour temperature in AV source, to adjust the red gain alone to to achieve reddish colour pattern. Applicable for composite source
    mfrSERIALIZED_TYPE_PANEL_AV_WB_DATA_USER,        ///< panel AV WB data user field. User can change the picture modes as per their requirement in USER colour temperature mode. Applicable for composite source
    mfrSERIALIZED_TYPE_PANEL_DTB_VERSION,            ///< Version of the device tree binary(DTB)
    mfrSERIALIZED_TYPE_PANEL_DTB_DATA_SIZE,          ///< Size of the device tree binary(DTB)
    mfrSERIALIZED_TYPE_PANEL_DTB_DATA,               ///< panel DTB data. The DTB file contains a binary-formatted flattened device tree data

    /* panel data*/
    mfrSERIALIZED_TYPE_PANEL_DATA_FUNCTION_STATUS,
    mfrSERIALIZED_TYPE_PANEL_DATA_AGEING_TIME,
    mfrSERIALIZED_TYPE_PANEL_DATA_POWER_ON_TIME,
    mfrSERIALIZED_TYPE_PANEL_DATA_BACKLIGHT_TIME,
    mfrSERIALIZED_TYPE_PANEL_DATA_VALID,
    mfrSERIALIZED_TYPE_PANEL_DATA_TPV_APP_VERSION,
    mfrSERIALIZED_TYPE_PANEL_ALS_CALIBRATION_INDEX0,
    mfrSERIALIZED_TYPE_PANEL_ALS_CALIBRATION_INDEX1,
    /*Gamma data*/
    mfrSERIALIZED_TYPE_PANEL_GAMMA_CALIBRATED_NORMAL,
    mfrSERIALIZED_TYPE_PANEL_GAMMA_CALIBRATED_COLD,
    mfrSERIALIZED_TYPE_PANEL_GAMMA_CALIBRATED_WARM,
    mfrSERIALIZED_TYPE_PANEL_GAMMA_CALIBRATED_BOOST_NORMAL,
    mfrSERIALIZED_TYPE_PANEL_GAMMA_CALIBRATED_BOOST_COLD,
    mfrSERIALIZED_TYPE_PANEL_GAMMA_CALIBRATED_BOOST_WARM,
    /*WB data boost*/
    mfrSERIALIZED_TYPE_PANEL_HDMI_WB_DATA_BOOST_NORMAL,
    mfrSERIALIZED_TYPE_PANEL_HDMI_WB_DATA_BOOST_COLD,
    mfrSERIALIZED_TYPE_PANEL_HDMI_WB_DATA_BOOST_WARM,
    /*Tmax*/
    mfrSERIALIZED_TYPE_PANEL_PEAK_BRIGHTNESS_NONBOOST,
    mfrSERIALIZED_TYPE_PANEL_PEAK_BRIGHTNESS_BOOST,
    mfrSERIALIZED_TYPE_PANEL_PEAK_BRIGHTNESS_BURST,
    mfrSERIALIZED_TYPE_PANEL_HDMI_WB_DATA_SUPERCOLD,
    mfrSERIALIZED_TYPE_PANEL_HDMI_WB_DATA_BOOST_SUPERCOLD,
    mfrSERIALIZED_TYPE_PANEL_GAMMA_CALIBRATED_SUPERCOLD,
    mfrSERIALIZED_TYPE_PANEL_GAMMA_CALIBRATED_BOOST_SUPERCOLD,
    mfrSERIALIZED_TYPE_PANEL_MAX,
#endif
    mfrSERIALIZED_TYPE_SKYMODELNAME = 8000,         //Sky specific MFRLib flags
    mfrSERIALIZED_TYPE_DE_SERIAL_PREFIX,            //Sky specific MFRLib flags
} mfrSerializedType_t;


/**
 * @brief MFR image types
 * 
 */
typedef enum _mfrImageType_t
{
    mfrIMAGE_TYPE_CDL,                               ///< CDL image type
    mfrIMAGE_TYPE_RCDL,                              ///< RCDL image type.
    mfrUPGRADE_IMAGE_MONOLITHIC,                     ///< Monolithic image type
    mfrUPGRADE_IMAGE_PACKAGEHEADER,                  ///< Package header image type
    mfrIMAGE_TYPE_MAX,                               ///< Out of range - required to be the last item of the enum
} mfrImageType_t;


/**
 * @brief MFR image write progress status
 * 
 */
 typedef enum _mfrUpgradeProgress_t
 {
   mfrUPGRADE_PROGRESS_NOT_STARTED = 0,               ///< not started
   mfrUPGRADE_PROGRESS_STARTED,                       ///< in progress
   mfrUPGRADE_PROGRESS_ABORTED,                       ///< failed
   mfrUPGRADE_PROGRESS_VERIFYING,                     ///< Verifying
   mfrUPGRADE_PROGRESS_FLASHING,                      ///< Flashing
   mfrUPGRADE_PROGRESS_REBOOTING,                     ///< Rebooting
   mfrUPGRADE_PROGRESS_COMPLETED,                     ///< success
   mfrUPGRADE_PROGRESS_MAX                            ///< Out of range - required to be the last item of the enum 
 } mfrUpgradeProgress_t;


/**
 * @brief MFR boot loader patterns
 * 
 */
typedef enum _mfrBlPattern_t
{
    mfrBL_PATTERN_NORMAL = 0,                        ///< normal boot loader pattern. This Boot pattern enable both LOGO as well LED ON during boot up
    mfrBL_PATTERN_SILENT,                            ///< silent boot loader pattern. Keep the led off
    mfrBL_PATTERN_SILENT_LED_ON,                     ///< silent LED on pattern. This Boot pattern enable only LED and disable LOGO during this boot up
    mfrBL_PATTERN_LOGO_DISABLED,                     ///< Logo is disabled
    mfrBL_PATTERN_MAX,                               ///< Out of range - required to be the last item of the enum 
} mfrBlPattern_t;

/**
 * @brief MFR image upgrade status
 * 
 */
typedef struct _mfrUpgradeStatus_t
{
  mfrUpgradeProgress_t progress;                    ///< MFR upgrade progress status. @see mfrUpgradeProgress_t
  mfrError_t error;                                 ///< Error @see mfrError_t
  char error_string[32];                            ///< Error string
  int percentage;                                   ///< MFR upgrade percentage
} mfrUpgradeStatus_t;

/**
 * @brief MFR image upgrade status notify stucture
 * 
 */
typedef struct _mfrUpgradeStatusNotify_t
{
   void * cbData;                                         ///< Upgrade status notify call back data
   void (*cb) (mfrUpgradeStatus_t * status);                ///< Upgrade status notify call back 
   int interval;                                          ///< number of seconds between two callbacks. 0 means invoking callback only once to report final upgrade result
} mfrUpgradeStatusNotify_t;

/**
 * @brief Initializes the MFR library
 *
 * This function will initialize all the respective internal components responsible for MFR functionalities.
 * This API need to be called before any other APIs in this module@n
 * 
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_ALREADY_INITIALIZED      - Module is already initialised
 * @retval mfrERR_MEMORY_EXHAUSTED         - memory allocation failure 
 * 
 * @warning  This API is Not thread safe
 * 
 */
mfrError_t mfr_init( void );

/**
 * @brief Initialize the mfr partition.
 *
 * mfr_init invokes this mfr_partition_init.
 * This function should be call once before accessing serialized data.
 *
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_ALREADY_INITIALIZED      - Module is already initialised
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * 
 *
 * @pre  mfr_init() should be called before calling this API. If this precondition is not met, the API will return mfrERR_NOT_INITIALIZED. 
*/
mfrError_t mfr_partition_init(void);

/**
 * @brief Uninitializes the MFR library
 *
 * This function will uninitialize all the respective internal components responsible for MFR functionalities.
 * 
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * 
 * @warning  This API is Not thread safe
 * 
 */
mfrError_t mfr_term( void );

/**
 * @brief Retrieves serialized Read-Only data from device
 * 
 * 
 * @param [in] type :  specifies the serialized data type to be read. @see mfrSerializedType_t
 * @param [in] data :  serialized data for the specific type requested. (buffer location, length, and func to free the buffer). @see mfrSerializedData_t
 *
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * @retval mfrERR_INVALID_PARAM            - Parameter passed to this function is invalid
 * @retval mfrERR_MEMORY_EXHAUSTED         - memory allocation failure
 * @retval mfrERR_FAILED_CRC_CHECK         - CRC check failed
 * @retval mfrERR_FLASH_READ_FAILED        - Flash read failed
 * 
 * @note The serialized data is returned as a byte stream. It is upto the  application to deserialize and make sense of the data returned.
 *  Even if the serialized data returned is "string", the buffer is not required to contain the null-terminator
 * 
 * @pre  mfr_init() should be called before calling this API. If this precondition is not met, the API will return mfrERR_NOT_INITIALIZED. 
 * @warning  This API is Not thread safe
 * 
 */
mfrError_t mfrGetSerializedData( mfrSerializedType_t type,  mfrSerializedData_t *data );

/**
 * @brief Sets the read write Serialization data on device
 *
 * @param [in] type :  specifies the serialized data type to write. @see mfrSerializedType_t
 * @param [in] data :  serialized data to set for the specific type requested. (buffer location, length, and func to free the buffer). @see mfrSerializedData_t
 *
 * @return mfrError_t                       - Status
 * @retval mfrERR_NONE                      - Success
 * @retval mfrERR_NOT_INITIALIZED           - Module is not initialised
 * @retval mfrERR_INVALID_PARAM             - Parameter passed to this function is invalid
 * @retval mfrERR_MEMORY_EXHAUSTED          - memory allocation failure
 * @retval mfrERR_FAILED_CRC_CHECK          - CRC check failed
 * @retval mfrERR_WRITE_FLASH_FAILED        - Flash write failed
 * @retval mfrERR_FLASH_READ_FAILED        - Flash read failed
 * @retval mfrERR_FLASH_VERIFY_FAILED       - Flash verification failed
 * 
 * @pre  mfr_init() should be called before calling this API. If this precondition is not met, the API will return mfrERR_NOT_INITIALIZED. 
 * @warning  This API is Not thread safe
 * 
 */
mfrError_t mfrSetSerializedData( mfrSerializedType_t type,  mfrSerializedData_t *data);

/**
 * @brief Writes the image into flash
 * 
 *    The process should follow these major steps:
 *    1) Verify the validity of the image and flash 
 *    2) Update boot params and switch banks to prepare for a reboot event
 *    3) All upgrades should be done in the alternate bank. The current bank should not be disturbed
 *
 *    State Transition:
 *    0) Before the API is invoked, the Upgrade process should be in PROGRESS_NOT_STARTED state
 *    1) After the API returns with success, the Upgrade process moves to PROGRESS_STARTED state
 *    2) After the API returns with error,   the Upgrade process stays in PROGRESS_NOT_STARTED state. Notify function will not be invoked
 *    3) The notify function is called at regular interval with process = PROGRESS_STARTED
 *    4) The last invocation of notify function should have either progress = PROGRESS_COMPLETED or progress = PROGRESS_ABORTED with error code set
 *
 *  @note mfrWriteImage() should work without any issue when device transition to DEEPSLEEP state and Wakeup. During DEEPSLEEP state processor will
 * cache all the pc and stack state and will enter to low power state. On wakeup system will use the saved pc and stack and resume from the same point.
 *   
 * @param [in] name :  the filename of the image file
 * @param [in] path :  the path of the image file in the file system
 * @param [in] type :  the type (format, signature type) of the image.  This can dictate the handling of the image within the MFR library. @see mfrImageType_t
 * @param[in] notify: function to provide status of the image flashing process.  @see mfrUpgradeStatusNotify_t
 * 
 * 
 * @return mfrError_t                              - Status
 * 
 * @retval mfrERR_NONE                             - Success
 * @retval mfrERR_NOT_INITIALIZED                  - Module is not initialised
 * @retval mfrERR_INVALID_PARAM                    - Parameter passed to this function is invalid
 * @retval mfrERR_MEMORY_EXHAUSTED                 - memory allocation failure
 * @retval mfrERR_FAILED_CRC_CHECK                 - CRC is failed
 * @retval mfrERR_WRITE_FLASH_FAILED               - Flash write failed
 * @retval mfrERR_FLASH_VERIFY_FAILED              - Flash verification failed
 * @retval mfrERR_BAD_IMAGE_HEADER                 - Image header is corrupted
 * @retval mfrERR_IMPROPER_SIGNATURE               - Image signature is invalid
 * @retval mfrERR_IMAGE_TOO_BIG                    - Image size is more than allocated maximum
 * @retval mfrERR_FAILED_INVALID_SIGNING_TIME      - Image signing time invalid
 * @retval mfrERR_FAILED_IMAGE_SVN_OLDER           - software version number is older than existing image
 * @retval mfrERR_FAILED_SAME_DRI_CODE_VERSION     - DRI code version is same
 * @retval mfrERR_FAILED_SAME_PCI_CODE_VERSION     - PCI code version is same
 * @retval mfrERR_IMAGE_FILE_OPEN_FAILED           - Not able to open the input image file
 * @retval mfrERR_GET_FLASHED_IMAGE_DETAILS_FAILED - Not able to get the current image version details
 * 
 * 
 * @pre  mfr_init() should be called before calling this API. If this precondition is not met, the API will return mfrERR_NOT_INITIALIZED. .
 * @warning  This API is Not thread safe
 * 
 */
mfrError_t mfrWriteImage(const char *name,  const char *path, mfrImageType_t type,  mfrUpgradeStatusNotify_t notify);

/**
 * @brief Deletes the PDRI image if it is present
 * 
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * @retval mfrERR_WRITE_FLASH_FAILED       - Flash write failed
 * @retval mfrERR_FLASH_VERIFY_FAILED      - Flash verification failed
 * 
 * @pre  mfr_init() should be called before calling this API. If this precondition is not met, the API will return mfrERR_NOT_INITIALIZED. 
 * @warning  This API is Not thread safe
 * 
 */
mfrError_t mfrDeletePDRI(void);

/**
 * @brief Deletes the platform images. Deletes the main image from primary and secondary bank
 * 
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * @retval mfrERR_WRITE_FLASH_FAILED       - Flash write failed
 * @retval mfrERR_FLASH_VERIFY_FAILED      - Flash verification failed
 * 
 * @pre  mfr_init() should be called before calling this API. If this precondition is not met, the API will return mfrERR_NOT_INITIALIZED. 
 * @warning  This API is Not thread safe
 * 
 */
mfrError_t mfrScrubAllBanks(void);

/**
 * @brief Sets bootloader LED pattern
 * 
 * This function stores the bootup pattern in the persistance storage for bootloader to read 
 * and control the front panel LED and/or TV backlight sequence on bootup
 * 
 * @param [in] pattern : options are defined by enum mfrBlPattern_t. @see mfrBlPattern_t
 * 
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * @retval mfrERR_INVALID_PARAM            - Parameter passed to this function is invalid
 * @retval mfrERR_WRITE_FLASH_FAILED       - Flash write failed
 * @retval mfrERR_FLASH_VERIFY_FAILED      - Flash verification failed
 * 
 * @pre  mfr_init() should be called before calling this API. If this precondition is not met, the API will return mfrERR_NOT_INITIALIZED. 
 * @warning  This API is Not thread safe
 * 
 */
mfrError_t mfrSetBootloaderPattern(mfrBlPattern_t pattern);

/**
 * @brief API to update Primary Splash screen Image and to override the default the Splash screen image
 *
 * @param [in] path : char pointer which holds the path of input bootloader OSD image.
 *
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * @retval mfrERR_INVALID_PARAM            - Parameter passed to this function is invalid
 * @retval mfrERR_IMAGE_FILE_OPEN_FAILED   - Failed to open the downloaded splash screen file
 * @retval mfrERR_MEMORY_EXHAUSTED         - memory allocation failure
 *
 * @pre  mfr_init() should be called before calling this API. If this precondition is not met, the API will return mfrERR_NOT_INITIALIZED. 
 * @warning  This API is Not thread safe
 *
 */
mfrError_t mfrSetBlSplashScreen(const char *path);

/**
 * @brief API to clear the primary Splash screen Image and to make
 * use of default Splash screen image
 *
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * @retval mfrERR_IMAGE_FILE_OPEN_FAILED   - Failed to open the downloaded splash screen file
 * @retval mfrERR_MEMORY_EXHAUSTED         - memory allocation failure
 *
 * @pre  mfr_init() should be called before calling this API. If this precondition is not met, the API will return mfrERR_NOT_INITIALIZED.
 * @warning  This API is Not thread safe
 *
 */
mfrError_t mfrClearBlSplashScreen(void);

/**
* @brief API to retrive the secure time from TEE
*
* @param [in] params : unit32 timeptr to get the UTC time in seconds
*
* @return Error Code:  Return mfrERR_NONE if operation is successful, mfrERR_GENERAL if it fails
*/
mfrError_t mfrGetSecureTime(uint32_t *timeptr);

/**
* @brief API to set the secure time from TEE
*
* @param [in] params : unit32 timeptr to set the UTC time in seconds
*
* @return Error Code:  Return mfrERR_NONE if operation is successful, mfrERR_GENERAL if it fails
*/
mfrError_t mfrSetSecureTime(uint32_t *timeptr);


/**
 * @brief API to set the fsr flag into the emmc raw area
 *
 * @param [in] params : uint16_t fsrflag to set the FSR flag
 *
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * @retval mfrERR_INVALID_PARAM            - Parameter passed to this function is invalid
 * @return Error Code:  Return mfrERR_NONE if operation is successful, mfrERR_GENERAL if it fails
 *
 **/
mfrError_t mfrSetFSRflag(uint16_t *newFsrFlag);

/**
 * @brief API to get the fsr flag from emmc
 *
 * @param [in] params : uint16_t fsrflag to get the FSR flag
 *
 * @return mfrError_t                      - Status
 * @retval mfrERR_NONE                     - Success
 * @retval mfrERR_INVALID_PARAM            - Parameter passed to this function is invalid
 * @retval mfrERR_NOT_INITIALIZED          - Module is not initialised
 * @return Error Code:  Return mfrERR_NONE if operation is successful, mfrERR_GENERAL if it fails
 *
 **/
mfrError_t mfrGetFSRflag(uint16_t *newFsrFlag);

/**
* @brief API to retrive the secure time from TEE
*
* @param [in] params : unit32 timeptr to get the UTC time in seconds
*
* @return Error Code:  Return mfrERR_NONE if operation is successful, mfrERR_GENERAL if it fails
*/
mfrError_t mfrGetSecureTime(uint32_t *timeptr);

/**
* @brief API to set the secure time from TEE
*
* @param [in] params : unit32 timeptr to set the UTC time in seconds
*
* @return Error Code:  Return mfrERR_NONE if operation is successful, mfrERR_GENERAL if it fails
*/
mfrError_t mfrSetSecureTime(uint32_t *timeptr);


/**
* @brief API to set the fsr flag into the emmc raw area
*
* @param [in] params : unit32 fsrflag to set the FSR flag
*
* @return Error Code:  Return mfrERR_NONE if operation is successful, mfrERR_GENERAL if it fails
*/
mfrError_t mfrSetFSRflag(uint16_t *newFsrFlag);


/**
* @brief API to get the fsr flag from emmc
*
* @param [in] params : unit32 fsrflag to get the FSR flag
*
* @return Error Code:  Return mfrERR_NONE if operation is successful, mfrERR_GENERAL if it fails
*/
mfrError_t mfrGetFSRflag(uint16_t *newFsrFlag);

#endif

/** @} */ // End of PLAT_MFR_DATA
/** @} */ // End of MFR_HAL
/** @} */ // End of MFR Module
/** @} */ // End of HPK
