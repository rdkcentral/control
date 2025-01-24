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

#include "ctrlm_hal.h"

#ifndef _CTRLM_HAL_RF4CE_H_
/// @cond DOXYGEN_IGNORE
#define _CTRLM_HAL_RF4CE_H_
/// @endcond

/// @file ctrlm_hal_rf4ce.h
///
/// @defgroup CTRL_MGR_HAL_RF4CE Control Manager HAL API - RF4CE Network
/// @{

/// @addtogroup HAL_RF4CE_Consts Constants
/// @{
/// @brief Macros for constant values used in the HAL RF4CE layer
/// @details The Control Manager HAL provides macros for some parameters which may change in the future.  Network implementations should use
/// these names to allow the network code to function correctly if the values change.

#define CTRLM_HAL_RF4CE_VENDOR_STRING_SIZE            (8) ///< Maximum size of the vendor string
#define CTRLM_HAL_RF4CE_USER_STRING_SIZE             (16) ///< Maximum size of the user string
#define CTRLM_HAL_RF4CE_DEVICE_TYPE_LIST_SIZE         (3) ///< Size of the device type list
#define CTRLM_HAL_RF4CE_PROFILE_ID_LIST_SIZE          (7) ///< Size of the profile id list
#define CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE (92) ///< Maximum size in bytes of the elements of the attributes in the RIB.
#if (CTRLM_HAL_RF4CE_API_VERSION >= 11)
#define CTRLM_HAL_RF4CE_DEEPSLEEP_PATTERN_LENGTH     (12) ///< Maximum length in bytes a deepsleep payload pattern
#define CTRLM_HAL_RF4CE_DEEPSLEEP_PATTERN_MAX        (12) ///< Maximum number of patterns for deepsleep
#endif

#define CTRLM_HAL_RF4CE_RIB_ATTR_LEN_VERSIONING                 (4)
#define CTRLM_HAL_RF4CE_RIB_ATTR_LEN_BATTERY_STATUS            (11)
#define CTRLM_HAL_RF4CE_RIB_ATTR_LEN_VOICE_CTRL_AUDIO_PROFILES  (2)
#define CTRLM_HAL_RF4CE_RIB_ATTR_LEN_VOICE_STATISTICS           (8)
#define CTRLM_HAL_RF4CE_RIB_ATTR_LEN_UPDATE_VERSIONING          (4)
#define CTRLM_HAL_RF4CE_RIB_ATTR_LEN_PRODUCT_NAME              (20)
#define CTRLM_HAL_RF4CE_RIB_ATTR_LEN_IR_RF_DATABASE_STATUS      (1)

/// @}

/// @addtogroup HAL_RF4CE_Types Type Definitions
/// @{
/// @brief Type definitions
/// @details The Control Manager HAL provides type definitions for Id's that are passed between the Control Manager and network networks.

/// @brief RF4CE Profile Id
typedef unsigned char      ctrlm_hal_rf4ce_profile_id_t;    ///< 8-bit RF4CE profile id value
/// @brief RF4CE Vendor Id
typedef unsigned short     ctrlm_hal_rf4ce_vendor_id_t;     ///< 16-bit RF4CE vendor id value
/// @brief RF4CE PAN Id
typedef unsigned short     ctrlm_hal_rf4ce_pan_id_t;        ///< 16-bit RF4CE PAN id value
/// @brief RF4CE IEEE Address
typedef unsigned long long ctrlm_hal_rf4ce_ieee_address_t;  ///< 64-bit RF4CE IEEE Address value
/// @brief RF4CE Short Address
typedef unsigned short     ctrlm_hal_rf4ce_short_address_t; ///< 16-bit RF4CE Short Address value
#if (CTRLM_HAL_RF4CE_API_VERSION >= 11)
/// @brief RF4CE Frame Control
typedef unsigned char      ctrlm_hal_rf4ce_frame_control_t; ///< 8 bit RF4CE Frame Control value
/// @brief RF4CE Deepsleep Payload Pattern
typedef unsigned char      ctrlm_hal_rf4ce_deepsleep_payload_pattern_t[CTRLM_HAL_RF4CE_DEEPSLEEP_PATTERN_LENGTH]; ///< 0-X byte RF4CE Payload Pattern
#endif
/// @}

/// @addtogroup HAL_RF4CE_Enums   Enumerations
/// @{
/// @brief Enumerated Types
/// @details The Control Manager HAL provides enumerated types for logical groups of values.

/// @brief RF4CE Result Values
/// @details An enumeration of the RF4CE result values that can be returned by the HAL layer. The values are defined in the RF4CE Network Layer specification.
typedef enum {
   CTRLM_HAL_RF4CE_RESULT_SUCCESS                  = 0x00,
   CTRLM_HAL_RF4CE_RESULT_NO_ORG_CAPACITY          = 0xB0,
   CTRLM_HAL_RF4CE_RESULT_NO_REC_CAPACITY          = 0xB1,
   CTRLM_HAL_RF4CE_RESULT_NO_PAIRING               = 0xB2,
   CTRLM_HAL_RF4CE_RESULT_NO_RESPONSE              = 0xB3,
   CTRLM_HAL_RF4CE_RESULT_NOT_PERMITTED            = 0xB4,
   CTRLM_HAL_RF4CE_RESULT_DUPLICATE_PAIRING        = 0xB5,
   CTRLM_HAL_RF4CE_RESULT_FRAME_COUNTER_EXPIRED    = 0xB6,
   CTRLM_HAL_RF4CE_RESULT_DISCOVERY_ERROR          = 0xB7,
   CTRLM_HAL_RF4CE_RESULT_DISCOVERY_TIMEOUT        = 0xB8,
   CTRLM_HAL_RF4CE_RESULT_SECURITY_TIMEOUT         = 0xB9,
   CTRLM_HAL_RF4CE_RESULT_SECURITY_FAILURE         = 0xBA,
   CTRLM_HAL_RF4CE_RESULT_BEACON_LOSS              = 0xE0,
   CTRLM_HAL_RF4CE_RESULT_CHANNEL_ACCESS_FAILURE   = 0xE1,
   CTRLM_HAL_RF4CE_RESULT_DENIED                   = 0xE2,
   CTRLM_HAL_RF4CE_RESULT_DISABLE_TRX_FAILURE      = 0xE3,
   CTRLM_HAL_RF4CE_RESULT_FAILED_SECURITY_CHECK    = 0xE4,
   CTRLM_HAL_RF4CE_RESULT_FRAME_TOO_LONG           = 0xE5,
   CTRLM_HAL_RF4CE_RESULT_INVALID_GTS              = 0xE6,
   CTRLM_HAL_RF4CE_RESULT_INVALID_HANDLE           = 0xE7,
   CTRLM_HAL_RF4CE_RESULT_INVALID_PARAMETER        = 0xE8,
   CTRLM_HAL_RF4CE_RESULT_NO_ACK                   = 0xE9,
   CTRLM_HAL_RF4CE_RESULT_NO_BEACON                = 0xEA,
   CTRLM_HAL_RF4CE_RESULT_NO_DATA                  = 0xEB,
   CTRLM_HAL_RF4CE_RESULT_NO_SHORT_ADDRESS         = 0xEC,
   CTRLM_HAL_RF4CE_RESULT_OUT_OF_CAP               = 0xED,
   CTRLM_HAL_RF4CE_RESULT_PAN_ID_CONFLICT          = 0xEE,
   CTRLM_HAL_RF4CE_RESULT_REALIGNMENT              = 0xEF,
   CTRLM_HAL_RF4CE_RESULT_TRANSACTION_EXPIRED      = 0xF0,
   CTRLM_HAL_RF4CE_RESULT_TRANSACTION_OVERFLOW     = 0xF1,
   CTRLM_HAL_RF4CE_RESULT_TX_ACTIVE                = 0xF2,
   CTRLM_HAL_RF4CE_RESULT_UNAVAILABLE_KEY          = 0xF3,
   CTRLM_HAL_RF4CE_RESULT_UNSUPPORTED_ATTRIBUTE    = 0xF4,
   CTRLM_HAL_RF4CE_RESULT_INVALID_INDEX            = 0xF9,
} ctrlm_hal_rf4ce_result_t;

/// @brief RF4CE Rib Attribute Identifier Values
/// @details An enumeration of the RF4CE Rib Attribute Identifiers that can be imported by Control Manager.
typedef enum {
   CTRLM_HAL_RF4CE_RIB_ATTR_ID_VERSIONING                = 0x02, ///< RIB Attribute - Versioning
   CTRLM_HAL_RF4CE_RIB_ATTR_ID_BATTERY_STATUS            = 0x03, ///< RIB Attribute - Battery Status
   CTRLM_HAL_RF4CE_RIB_ATTR_ID_VOICE_CTRL_AUDIO_PROFILES = 0x17, ///< RIB Attribute - Voice Controller Audio Profiles
   CTRLM_HAL_RF4CE_RIB_ATTR_ID_VOICE_STATISTICS          = 0x19, ///< RIB Attribute - Voice Statistics
   CTRLM_HAL_RF4CE_RIB_ATTR_ID_UPDATE_VERSIONING         = 0x31, ///< RIB Attribute - Update Versioning
   CTRLM_HAL_RF4CE_RIB_ATTR_ID_PRODUCT_NAME              = 0x32, ///< RIB Attribute - Product Name
   CTRLM_HAL_RF4CE_RIB_ATTR_ID_IR_RF_DATABASE_STATUS     = 0xDA, ///< RIB Attribute - IR RF Database Status
} ctrlm_hal_rf4ce_rib_attr_id_t;

/// @}

/// @addtogroup HAL_RF4CE_Callback_Typedefs   Callback Function Type Definitions
/// @{
/// @brief Functions that are implemented by the HAL Network
/// @details The Control Manager provides function type definitions for callback functions that are optionally implemented by the HAL network.

/// @brief RF4CE Data Read Function Prototype
/// @details This function is called by Control Manager to read data during the data indication function.
/// @param[in] length - Amount of data to read
/// @param[in] data   - Buffer in which to place the data
/// @return Number of bytes read.
typedef unsigned char (*ctrlm_hal_rf4ce_data_read_t)(unsigned char length, unsigned char *data, void *param);

/// @brief Data Confirm Function Prototype
/// @details This function is called by the HAL layer to provide a confirmation to a data request from Control Manager.
/// @param[in] result    - Result of the data request
/// @param[in] user_data - Pointer to user data that is passed in the callback
/// @return None
typedef void (*ctrlm_hal_rf4ce_cfm_data_t)(ctrlm_hal_rf4ce_result_t result, void *user_data);

/// @}

/// @addtogroup HAL_RF4CE_Structs Structures
/// @{

/// @brief RF4CE Data Indication Parameters Structure
/// @details The structure which is passed from the HAL Network device in the RF4CE data indication.
typedef struct {
   ctrlm_controller_id_t        controller_id;       ///< Identifier of the controller
   ctrlm_hal_rf4ce_profile_id_t profile_id;          ///< Profile identifier
   ctrlm_hal_rf4ce_vendor_id_t  vendor_id;           ///< Vendor identifier
   unsigned char                flag_broadcast;      ///< Flag indicating whether the transmission is broadcast (1) or unicast (0)
   unsigned char                flag_acknowledged;   ///< Flag indicating whether the transmission is acknowledged (1) or unacknowledged (0)
   unsigned char                flag_single_channel; ///< Flag indication whether the transmission is single channel (1) or multi-channel (0)
#if CTRLM_HAL_RF4CE_API_VERSION >= 14
   unsigned char                flag_indirect;       ///< Flag indication whether the transmission is indirect (1) or direct (0)
#endif
   ctrlm_timestamp_t            tx_window_start;     ///< Timestamp of the start of the transmission window.  The data must not be transmitted until this time is reached.
   unsigned long                tx_window_duration;  ///< The duration of the transmission window in microseconds. The data transmission should not be continued beyond the end of the transmission window.
   unsigned char                length;              ///< Length of the data to transmit
   unsigned char *              data;                ///< Pointer to the data to transmit (or NULL when using read callback)
   ctrlm_hal_rf4ce_data_read_t  cb_data_read;        ///< Callback function to read the data (or NULL when using data pointer)
   void *                       cb_data_param;       ///< Parameter to be passed to the data read callback function (optional)
   ctrlm_hal_rf4ce_cfm_data_t   cb_confirm;          ///< Callback function to be called when the transmission is complete. This parameter is set to NULL if no confirmation is requested.
   void *                       cb_confirm_param;    ///< Parameter to be passed to the data confirmation callback.
} ctrlm_hal_rf4ce_req_data_params_t;

/// @brief RF4CE RIB Data Import Parameters Structure
/// @details The structure which is passed from the HAL Network device in the RF4CE RIB data import call.
typedef struct {
   unsigned long long            ieee_address;                                       ///< IN     - ieee address of the controller
   ctrlm_hal_rf4ce_rib_attr_id_t identifier;                                         ///< IN     - RIB attribute identifier
   unsigned char                 index;                                              ///< IN     - RIB attribute index
   unsigned char                 length;                                             ///< IN/OUT - RIB data length
   unsigned char                 data[CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE]; ///< OUT    - RIB entry's data
} ctrlm_hal_rf4ce_rib_data_import_params_t;
/// @}

/// @brief RF4CE RIB Data Export Parameters Structure
/// @details The structure which is passed from the HAL Network device in the RF4CE RIB data export call.
typedef struct {
   ctrlm_controller_id_t         controller_id;                                      ///< IN - Identifier of the controller
   ctrlm_hal_rf4ce_rib_attr_id_t identifier;                                         ///< IN - RIB attribute identifier
   unsigned char                 index;                                              ///< IN - RIB attribute index
   unsigned char                 length;                                             ///< IN - RIB data length
   unsigned char                 data[CTRLM_HAL_RF4CE_CONST_MAX_RIB_ATTRIBUTE_SIZE]; ///< IN - RIB entry's data
} ctrlm_hal_rf4ce_rib_data_export_params_t;
/// @}

/// @addtogroup HAL_RF4CE_Functions HAL Function Prototypes
/// @{

/// @brief Controller Pairing Function Prototype
/// @details This function is called by Control Manager to pair with the specified controller (reserved for future use).  At the moment, all pairing is initiated from the controller.
/// @param None - Reserved for future use
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_req_pair_t)(void);
/// @brief Controller Unpairing Function Prototype
/// @details This function is called by Control Manager to unpair the specified controller in the HAL network.  The controller id is the identifier of the controller.  The
/// RF4CE unpairing procedure must be executed and the controller must be removed from the pairing table. This request must be performed synchronously.
/// @param[in] controller_id - Identifier of the controller to unpair
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_req_unpair_t)(ctrlm_controller_id_t controller_id);
/// @brief Network Data Request Function Prototype
/// @details This function is called by Control Manager to send data to a device on the HAL Network.  The request must be performed
/// asynchronously. After completion of the request, the data confirmation callback must be called (if not NULL).
/// @param[in] params - Parameters for the data request
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_req_data_t)(ctrlm_hal_rf4ce_req_data_params_t params);
/// @brief Network Rib Data Import Function Prototype
/// @details This function is called by Control Manager to import the Rib data for a previously bound controller.  The request must be performed
/// synchronously.
/// @param[in] params - Parameters for the import request
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_rib_data_import_t)(ctrlm_hal_rf4ce_rib_data_import_params_t *params);
/// @brief Network Rib Data Export Function Prototype
/// @details This function is called by Control Manager to export the Rib data for a previously bound controller.  The request must be performed
/// synchronously.
/// @param[in] params - Parameters for the export request
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_rib_data_export_t)(ctrlm_hal_rf4ce_rib_data_export_params_t *params);

/// @}

/// @addtogroup HAL_RF4CE_Structs Structures
/// @{
/// @brief Structure Definitions
/// @details The Control Manager HAL provides structures that are used in calls to the HAL networks.

/// @brief RF4CE Confirm Init Parameters Structure
/// @details The structure which is passed from the HAL Network device in the RF4CE initialization confirmation.
typedef struct {
   ctrlm_hal_result_t                           result;                                         ///< Network Initialization Result
   char                                         version[CTRLM_HAL_NETWORK_VERSION_STRING_SIZE]; ///< Network Version String
   char                                         chipset[CTRLM_HAL_NETWORK_VERSION_STRING_SIZE]; ///< Network's Chipset Name
   ctrlm_hal_rf4ce_pan_id_t                     pan_id;                                         ///< Network's PAN Identifier
   ctrlm_hal_rf4ce_ieee_address_t               ieee_address;                                   ///< Network's IEEE Address
   ctrlm_hal_rf4ce_short_address_t              short_address;                                  ///< Network's Short Address
   ctrlm_hal_req_term_t                         term;                                           ///< Network Terminate Request
   ctrlm_hal_rf4ce_req_pair_t                   pair;                                           ///< RF4CE Pair Request
   ctrlm_hal_rf4ce_req_unpair_t                 unpair;                                         ///< RF4CE Unpair Request
   ctrlm_hal_req_property_get_t                 property_get;                                   ///< Network Property Get Request
   ctrlm_hal_req_property_set_t                 property_set;                                   ///< Network Property Set Request
   ctrlm_hal_rf4ce_req_data_t                   data;                                           ///< Network Data Request
   ctrlm_hal_rf4ce_rib_data_import_t            rib_data_import;                                ///< Import Rib Data
   ctrlm_hal_rf4ce_rib_data_export_t            rib_data_export;                                ///< Export Rib Data
#if CTRLM_HAL_RF4CE_API_VERSION >= 9
   unsigned char *                              nvm_backup_data;                                ///< Pointer to nvm image to backup
   unsigned long                                nvm_backup_len;                                 ///< Length of nvm image to backup (in bytes)
#endif
} ctrlm_hal_rf4ce_cfm_init_params_t;

/// @brief RF4CE Discovery Indication Parameters Structure
/// @details The structure which is passed from the HAL Network device in the RF4CE discovery indication.
typedef struct {
   ctrlm_timestamp_t              timestamp;
   ctrlm_hal_rf4ce_ieee_address_t src_ieee_addr;
   unsigned char                  org_node_capabilities;
   ctrlm_hal_rf4ce_vendor_id_t    org_vendor_id;
   char                           org_vendor_string[CTRLM_HAL_RF4CE_VENDOR_STRING_SIZE];
   unsigned char                  org_app_capabilities;
   unsigned char                  org_user_string[CTRLM_HAL_RF4CE_USER_STRING_SIZE];
   unsigned char                  org_dev_type_list[CTRLM_HAL_RF4CE_DEVICE_TYPE_LIST_SIZE];
   unsigned char                  org_profile_id_list[CTRLM_HAL_RF4CE_PROFILE_ID_LIST_SIZE];
   unsigned char                  search_dev_type;
   unsigned char                  rx_link_quality;
   void *                         user_data;
} ctrlm_hal_rf4ce_ind_disc_params_t;

/// @brief RF4CE Discovery Response Parameters Structure
/// @details The structure which is passed to the HAL Network device in the RF4CE discovery response.
typedef struct {
   ctrlm_hal_result_discovery_t   result;
   ctrlm_hal_rf4ce_ieee_address_t dst_ieee_addr;
   unsigned char                  rec_app_capabilities;
   unsigned char                  rec_dev_type_list[CTRLM_HAL_RF4CE_DEVICE_TYPE_LIST_SIZE];
   unsigned char                  rec_profile_id_list[CTRLM_HAL_RF4CE_PROFILE_ID_LIST_SIZE];
   unsigned char                  disc_req_lqi;
   char                           rec_user_string[CTRLM_HAL_RF4CE_USER_STRING_SIZE];
   void *                         user_data;
} ctrlm_hal_rf4ce_rsp_disc_params_t;

/// @brief RF4CE Pair Indication Parameters Structure
/// @details The structure which is passed from the HAL Network device in the RF4CE pair indication.
typedef struct {
   ctrlm_timestamp_t                 timestamp;
   ctrlm_hal_rf4ce_result_t          status;
   ctrlm_hal_rf4ce_pan_id_t          src_pan_id;
   ctrlm_hal_rf4ce_ieee_address_t    src_ieee_addr;
   unsigned char                     org_node_capabilities;
   ctrlm_hal_rf4ce_vendor_id_t       org_vendor_id;
   char                              org_vendor_string[CTRLM_HAL_RF4CE_VENDOR_STRING_SIZE];
   unsigned char                     org_app_capabilities;
   unsigned char                     org_user_string[CTRLM_HAL_RF4CE_USER_STRING_SIZE];
   unsigned char                     org_dev_type_list[CTRLM_HAL_RF4CE_DEVICE_TYPE_LIST_SIZE];
   unsigned char                     org_profile_id_list[CTRLM_HAL_RF4CE_PROFILE_ID_LIST_SIZE];
   void *                            pairing_data;
   void *                            user_data;
} ctrlm_hal_rf4ce_ind_pair_params_t;

/// @brief RF4CE Pair Status Parameters Structure
/// @details The structure which is passed from the HAL Network device in the RF4CE pair status indication.
typedef struct {
   ctrlm_timestamp_t              timestamp;
   ctrlm_controller_id_t          controller_id;
   ctrlm_hal_rf4ce_ieee_address_t dst_ieee_addr;
   ctrlm_hal_result_pair_t        result;
} ctrlm_hal_rf4ce_ind_pair_result_params_t;

/// @brief RF4CE Pair Response Parameters Structure
/// @details The structure which is passed to the HAL Network device in the RF4CE pair response.
typedef struct {
   ctrlm_hal_result_pair_request_t   result;
   ctrlm_hal_rf4ce_result_t          status;
   ctrlm_controller_id_t             controller_id;
   ctrlm_hal_rf4ce_pan_id_t          dst_pan_id;
   ctrlm_hal_rf4ce_ieee_address_t    dst_ieee_addr;
   unsigned char                     rec_app_capabilities;
#if (CTRLM_HAL_RF4CE_API_VERSION >= 12)
   unsigned char                     rec_user_string[CTRLM_HAL_RF4CE_USER_STRING_SIZE];
#endif
   unsigned char                     rec_dev_type_list[CTRLM_HAL_RF4CE_DEVICE_TYPE_LIST_SIZE];
   unsigned char                     rec_profile_id_list[CTRLM_HAL_RF4CE_PROFILE_ID_LIST_SIZE];
   void *                            user_data;
} ctrlm_hal_rf4ce_rsp_pair_params_t;

/// @brief RF4CE Unpair Indication Parameters Structure
/// @details The structure which is passed from the HAL Network device in the RF4CE unpair indication.
typedef struct {
   ctrlm_timestamp_t              timestamp;
   ctrlm_controller_id_t          controller_id;
   ctrlm_hal_rf4ce_ieee_address_t ieee_address;
} ctrlm_hal_rf4ce_ind_unpair_params_t;

/// @brief RF4CE Unpair Response Parameters Structure
/// @details The structure which is passed to the HAL Network device in the RF4CE unpair response.
typedef struct {
   ctrlm_hal_result_unpair_t result;                       ///< Result of the unpair indication
   ctrlm_controller_id_t     controller_id;                ///< Controller Id assigned by control manager
} ctrlm_hal_rf4ce_rsp_unpair_params_t;

/// @brief RF4CE Decrypt Parameters Structure
/// @details The structure which is passed to the HAL Network device in the RF4CE decrypt call.
typedef struct {
   unsigned char *  cipher_text;                           ///< encrypted ciphertext
   unsigned short   data_length;                           ///< ciphertext length
   unsigned char *  auth;                                  ///< additional authentication data (AAD) known as ccm-adata
   unsigned short   auth_length;                           ///< additional authentication data (AAD) or ccm-adata length
   unsigned char *  tag;                                   ///< Message Integrity Code (MIC) known as ccm-tag
   unsigned short   tag_length;                            ///< Message Integrity Code (MIC) length (ccm-tag length) This field contains the expected MIC length
   unsigned char *  key;                                   ///< AES-128 key. This field contains the pointer to the encryption key. The key size is fixed to 16 bytes.
   unsigned char *  nonce;                                 ///< This field contains the pointer to the nonce used for operation. The nonce length is fixed to 13 bytes.
   unsigned char *  plain_text;                            ///< decrypted plaintext
} ctrlm_hal_rf4ce_decrypt_params_t;


/// @brief RF4CE Data Indication Parameters Structure
/// @details The structure which is passed from the HAL Network device in the RF4CE data indication.
typedef struct {
   ctrlm_timestamp_t            timestamp;                 ///< Timestamp of packet receipt
   ctrlm_hal_rf4ce_profile_id_t profile_id;                ///< RF4CE Profile Identifier
   ctrlm_hal_rf4ce_vendor_id_t  vendor_id;                 ///< RF4CE Vendor Id
   unsigned char                flags;                     ///< Data indication flags
   unsigned char                lqi;                       ///< Packet quality indicator
   unsigned char                command_id;                ///< Command Id (first byte of the payload)
   unsigned char                length;                    ///< Length of the data
   unsigned char *              data;                      ///< Pointer to data buffer (or NULL if read callback is used)
   ctrlm_hal_rf4ce_data_read_t  cb_data_read;              ///< Callback function to read the data (or NULL if data buffer is used)
   void *                       cb_data_param;             ///< Parameter passed to the callback function
} ctrlm_hal_rf4ce_ind_data_params_t;

#if (CTRLM_HAL_RF4CE_API_VERSION >= 11)
/// @breif RF4CE Deepsleep Pattern Structure
/// @details The structure which is included in the Deepsleep Arguments Structure that describes a deepsleep pattern
typedef struct {
   ctrlm_hal_rf4ce_profile_id_t                profile_id;
   ctrlm_hal_rf4ce_frame_control_t             frame_control;
   unsigned char                               pattern_length;
   ctrlm_hal_rf4ce_deepsleep_payload_pattern_t pattern;
} ctrlm_hal_rf4ce_deepsleep_pattern_t;

/// @brief RF4CE Deepsleep Arguments Structure
/// @details The structure which is passed to the HAL Network device within the SET_PROPERTY HAL call for Deepsleep
typedef struct {
   unsigned char length;
   ctrlm_hal_rf4ce_deepsleep_pattern_t patterns[CTRLM_HAL_RF4CE_DEEPSLEEP_PATTERN_MAX];
} ctrlm_hal_rf4ce_deepsleep_arguments_t;
#endif

/// @}

/// @addtogroup HAL_RF4CE_Callback_Typedefs   Callback Function Type Definitions
/// @{

/// @brief Discovery Response Function Prototype
/// @details This function is called by Control Manager to provide a response to a discovery request from a HAL Network device.
/// @param[in] params    - Discovery response parameter structure
/// @param[in] user_data - Pointer to user data that is passed in the callback
/// @return None
typedef void (*ctrlm_hal_rf4ce_rsp_discovery_t)(ctrlm_hal_rf4ce_rsp_disc_params_t params, void *user_data);

/// @brief Pair Response Function Prototype
/// @details This function is called by Control Manager to provide a response to a pair request from a HAL Network device.
/// @param[in] params    - Pair response parameter structure
/// @param[in] user_data - Pointer to user data that is passed in the callback
/// @return None
typedef void (*ctrlm_hal_rf4ce_rsp_pair_t)(ctrlm_hal_rf4ce_rsp_pair_params_t params, void *user_data);

/// @brief Unpair Response Function Prototype
/// @details This function is called by Control Manager to provide a response to an unpair indication from a HAL Network device.
/// @param[in] params    - Unpair response parameter structure
/// @param[in] user_data - Pointer to user data that is passed in the callback
/// @return None
typedef void (*ctrlm_hal_rf4ce_rsp_unpair_t)(ctrlm_hal_rf4ce_rsp_unpair_params_t params, void *user_data);
/// @}

#ifdef __cplusplus
extern "C" {
#endif

/// @addtogroup HAL_RF4CE_Functions HAL Function Prototypes
/// @{
/// @brief Function that are implemented by the RF4CE network device for use by Control Manager.
/// @details The RF4CE HAL layer provides a set of functions that Control Manager can call after initialization is complete.

/// @brief Network Init Confirmation Function
/// @details This function is called after the HAL network initialization is complete.
/// @param[in] network_id - Identifier for this HAL network device
/// @param[in] params     - Initialization confirmation parameters
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_cfm_init_t)(ctrlm_network_id_t network_id, ctrlm_hal_rf4ce_cfm_init_params_t params);
/// @brief RF4CE Discovery Request Indication Function
/// @details This function is called by the HAL network upon receipt of a discovery request from a controller.  Control Manager will determine whether
/// to respond to the request or ignore it.  Parameters for the discovery response are provided by Control Manager.
/// @param[in]  network_id - Identifier for this HAL network device
/// @param[in]  params     - Discovery indication parameters
/// @param[out] rsp_params - Pointer to discovery response parameters (optional). Null implies asynchronous operation.
/// @param[in]  cb         - Discovery response callback function pointer (optional). Null implies synchronous operation.
/// @param[in]  cb_data    - Discovery response callback data to be passed in the callback (optional).
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_ind_discovery_t)(ctrlm_network_id_t network_id, ctrlm_hal_rf4ce_ind_disc_params_t params, ctrlm_hal_rf4ce_rsp_disc_params_t *rsp_params, ctrlm_hal_rf4ce_rsp_discovery_t cb, void *cb_data);
/// @brief RF4CE Pair Request Indication Function
/// @details This function is called by the HAL network upon receipt of a pair request from a controller.  Control Manager will determine whether
/// to respond to the request or ignore it.  Parameters for the pair response are provided by Control Manager.
/// @param[in]  network_id - Identifier for this HAL network device
/// @param[in]  params     - Pair indication parameters
/// @param[out] rsp_params - Pointer to pair response parameters (optional). Null implies asynchronous operation.
/// @param[in]  cb         - Pair response callback function pointer (optional). Null implies synchronous operation.
/// @param[in]  cb_data    - Pair response callback data to be passed in the callback (optional).
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_ind_pair_t)(ctrlm_network_id_t network_id, ctrlm_hal_rf4ce_ind_pair_params_t params, ctrlm_hal_rf4ce_rsp_pair_params_t *rsp_params, ctrlm_hal_rf4ce_rsp_pair_t cb, void *cb_data);
/// @brief RF4CE Pair Status Indication Function
/// @details This function is called by the HAL network upon completion of a pair attempt.
/// @param[in] network_id - Identifier for this HAL network device
/// @param[in] params     - Pair result indication parameters
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_ind_pair_result_t)(ctrlm_network_id_t network_id, ctrlm_hal_rf4ce_ind_pair_result_params_t params);
/// @brief RF4CE Unpair Request Indication Function
/// @details This function is called by the HAL network when a controller is unpaired to the target.
/// @param[in]  network_id - Identifier for this HAL network device
/// @param[in]  params     - Unpair indication parameters
/// @param[out] rsp_params - Pointer to unpair response parameters (optional). Null implies asynchronous operation.
/// @param[in]  cb         - Unpair response callback function pointer (optional). Null implies synchronous operation.
/// @param[in]  cb_data    - Unpair response callback data to be passed in the callback (optional).
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_ind_unpair_t)(ctrlm_network_id_t network_id, ctrlm_hal_rf4ce_ind_unpair_params_t params, ctrlm_hal_rf4ce_rsp_unpair_params_t *rsp_params, ctrlm_hal_rf4ce_rsp_unpair_t cb, void *cb_data);
/// @brief RF4CE Network Layer Data Indication Function
/// @details This function is called by the HAL network when data is received from a paired controller.
/// @param[in]  network_id    - Identifier for this HAL network device
/// @param[in]  controller_id - Identifier of the controller
/// @param[in]  params        - Data indication parameters
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_ind_data_t)(ctrlm_network_id_t network_id, ctrlm_controller_id_t controller_id, ctrlm_hal_rf4ce_ind_data_params_t params);
/// @brief RF4CE Network Layer Assert Function
/// @details This function is called by the HAL network when an unrecoverable error is encountered.
/// @param[in]  network_id    - Identifier for this HAL network device
/// @param[in]  assert_info   - Assert information string
/// @return Never returns.
#if CTRLM_HAL_RF4CE_API_VERSION >= 15
typedef void (*ctrlm_hal_rf4ce_assert_t)(ctrlm_network_id_t network_id, const char* assert_info);
#else
typedef void (*ctrlm_hal_rf4ce_assert_t)(ctrlm_network_id_t network_id);
#endif
/// @brief RF4CE SoC side Decrypt Function
/// @details This function is called by the HAL network when an SoC side decryption is enabled.
/// @param[in, out]  ctrlm_hal_rf4ce_decrypt_params_t    - Decryption parameters
/// @return Never returns.
typedef ctrlm_hal_result_t (*ctrlm_hal_rf4ce_decrypt_t)(ctrlm_hal_rf4ce_decrypt_params_t* param);




/// @}

/// @addtogroup HAL_RF4CE_Structs Structures
/// @{

/// @brief Network Initialization Structure
/// @details The structure which is passed from Control Manager to the HAL Network device for initialization.
typedef struct {
   unsigned long                     api_version;      ///< Version of the HAL API
   ctrlm_network_id_t                network_id;       ///< Unique identiifer for the network
   ctrlm_hal_rf4ce_cfm_init_t        cfm_init;         ///< Network Initialization Confirm
   ctrlm_hal_rf4ce_ind_discovery_t   ind_discovery;    ///< RF4CE Discovery Indication
   ctrlm_hal_rf4ce_ind_pair_t        ind_pair;         ///< RF4CE Pair Indication
   ctrlm_hal_rf4ce_ind_pair_result_t ind_pair_result;  ///< RF4CE Pair Result Indication
   ctrlm_hal_rf4ce_ind_unpair_t      ind_unpair;       ///< RF4CE Unpair Indication
   ctrlm_hal_rf4ce_ind_data_t        ind_data;         ///< RF4CE Data Indication
#if CTRLM_HAL_RF4CE_API_VERSION >= 9
   ctrlm_hal_rf4ce_assert_t          assert;           ///< Assert function
#if CTRLM_HAL_RF4CE_API_VERSION >= 15
   ctrlm_hal_rf4ce_decrypt_t         decrypt;          ///< Decryption function
#endif
   unsigned char                     nvm_reinit;       ///< Flag indicating whether to reinitialize nvm (1) or not (0)
   unsigned char *                   nvm_restore_data; ///< Pointer to nvm image to restore
   unsigned long                     nvm_restore_len;  ///< Length of nvm image to restore (in bytes)
#endif
} ctrlm_hal_rf4ce_main_init_t;
/// @}

/// @addtogroup HAL_RF4CE_Functions HAL Function Prototypes
/// @{

/// @brief RF4CE Network Main
/// @details This function is called to begin the RF4CE HAL Network initialization.  This function must be implemented by the HAL network and directly
/// linked with Control Manager. The provided network id must be used by the network in all communication with the Control Manager.
/// @param[in] main_init - Pointer to main initialization parameters
/// @return Zero for success, all other values indicate an error
void *ctrlm_hal_rf4ce_main(ctrlm_hal_rf4ce_main_init_t *main_init);
/// @}

/// @addtogroup CTRLM_RF4CE_Functions Control Manager Function Prototypes
/// @{
/// @brief Function that are implemented by Control Manager for use by the HAL network device
/// @details The Control Manager provides a set of functions that the HAL Network can call after initialization is complete.

/// @brief RF4CE Result String
/// @details This function returns a string representation of the HAL RF4CE result type.
/// @param[in] result - RF4CE result enumeration value
/// @return String representation of the enumeration value
const char *ctrlm_hal_rf4ce_result_str(ctrlm_hal_rf4ce_result_t result);
/// @}

#ifdef __cplusplus
}
#endif

/// @}
#endif
