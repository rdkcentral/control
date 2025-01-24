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
#include "ctrlm_ipc.h"
#include "ctrlm_ipc_key_codes.h"

#ifndef _CTRLM_HAL_IP_H_
#define _CTRLM_HAL_IP_H_

#define CTRLM_HAL_IP_DATA_MAX_LEN                 (5120)

#define CTRLM_HAL_IP_SHA256_LEN                   (32)
#define CTRLM_HAL_IP_PSK_LEN                      CTRLM_HAL_IP_SHA256_LEN
#define CTRLM_HAL_IP_CERTIFICATE_FINGERPRINT_LEN  CTRLM_HAL_IP_SHA256_LEN
#define CTRLM_HAL_IP_PSK_KEY_IDENITITY_LEN         (8)
#define CTRLM_HAL_IP_PSK_PAIRING_IDENTIFIER_LEN    (8)
#define CTRLM_HAL_IP_PSK_KEY_SEED_LEN             (20)
#define CTRLM_HAL_IP_DEVICE_ID_LEN                (19)
#define CTRLM_HAL_IP_TARGET_LEN                   (20)

#define CTRLM_HAL_IP_PERMISSIONS_KEY_PRESS        (0b00000001)
#define CTRLM_HAL_IP_PERMISSIONS_VOICE            (0b00000010)
// Permissions bits 0-6 are reserved for other IP Control features


/// @file ctrlm_hal_ip.h
///
/// @defgroup CTRL_MGR_HAL_IP Control Manager IP Network HAL API
/// @{

/// @addtogroup HAL_IP_Enums   Enumerations
/// @{
/// @brief Enumerated Types
/// @details The Control Manager HAL provides enumerated types for logical groups of values.

/// @brief IP Control Security Types
typedef enum {
   CTRLM_HAL_IP_SECURITY_TYPE_PSK,
   CTRLM_HAL_IP_SECURITY_TYPE_WHITELIST,
   CTRLM_HAL_IP_SECURITY_TYPE_UNKNOWN
} ctrlm_hal_ip_security_type_t;

typedef enum {
   CTRLM_HAL_IP_MSG_TYPE_TEXT,
   CTRLM_HAL_IP_MSG_TYPE_BINARY,
   CTRLM_HAL_IP_MSG_TYPE_UNKNOWN
} ctrlm_hal_ip_msg_type_t;

/// @brief IP Control Command Types
typedef enum {
   CTRLM_HAL_IP_COMMAND_PAIR_REQUEST,
   CTRLM_HAL_IP_COMMAND_PAIR_RESPONSE,
   CTRLM_HAL_IP_COMMAND_BIND_REQUEST,
   CTRLM_HAL_IP_COMMAND_BIND_RESPONSE,
   CTRLM_HAL_IP_COMMAND_ACTION,
   CTRLM_HAL_IP_COMMAND_ACTION_RESPONSE,
   CTRLM_HAL_IP_COMMAND_DEVICE_INFORMATION_QUERY,
   CTRLM_HAL_IP_COMMAND_DEVICE_INFORMATION_QUERY_RESPONSE,
   CTRLM_HAL_IP_COMMAND_UNKNOWN
} ctrlm_hal_ip_command_type_t;

/// @brief IP Control Command Error Types
typedef enum {
   CTRLM_HAL_IP_ERROR_NONE,
   CTRLM_HAL_IP_ERROR_UNRECOGNIZED,
   CTRLM_HAL_IP_ERROR_NOT_SUPPORTED,
   CTRLM_HAL_IP_ERROR_PERMISSIONS,
   CTRLM_HAL_IP_ERROR_UNKNOWN
} ctrlm_hal_ip_error_t;
/// @}

/// @addtogroup HAL_IP_Types Type Definitions
/// @{
/// @brief Type definitions
/// @details The Control Manager HAL provides type definitions for Id's that are passed between the Control Manager and network networks.

// /// @brief IP Controller Name
// typedef char ctrlm_hal_ip_name_t[CTRLM_HAL_IP_CONTROLLER_NAME_LEN];
/// @brief IP Control Path String
typedef char ctrlm_hal_ip_path_t[20];
/// @brief IP Control PSK key identity
typedef char ctrlm_hal_ip_psk_key_identity_t[CTRLM_HAL_IP_PSK_KEY_IDENITITY_LEN];
/// @brief IP Control PSK pairing identifier
typedef char ctrlm_hal_ip_psk_pairing_identifier_t[CTRLM_HAL_IP_PSK_PAIRING_IDENTIFIER_LEN];
/// @brief IP Control PSK key seed
typedef char ctrlm_hal_ip_psk_key_seed_t[CTRLM_HAL_IP_PSK_KEY_SEED_LEN];
/// @brief IP Control Security Data
typedef unsigned char ctrlm_hal_ip_security_data_t[CTRLM_HAL_IP_CERTIFICATE_FINGERPRINT_LEN];
/// @brief IP Control Port
typedef int ctrlm_hal_ip_port_t;
/// @brief IP Control Target URL 
typedef char ctrlm_hal_ip_target_t[CTRLM_HAL_IP_TARGET_LEN];
/// @brief IP Control Device ID
typedef char ctrlm_hal_ip_device_id_t[CTRLM_HAL_IP_DEVICE_ID_LEN + 1];
/// @brief IP Control Permissions
typedef unsigned char ctrlm_hal_ip_permissions_t;
/// @}

/// @addtogroup HAL_IP_Structs Structures
/// @{
/// @brief Structure Definitions
/// @details The Control Manager HAL provides structures that are used in calls to the HAL networks.

/// @brief IP Control Revoke Data
/// @details This structure is passed to HAL Network on security revoke call.
typedef struct {
   ctrlm_hal_ip_security_type_t security_type;
   ctrlm_hal_ip_security_data_t security_data; // Either PSK identity or Fingerprint
} ctrlm_hal_ip_security_revoke_data_t;

typedef struct {
   ctrlm_timestamp_t            timestamp;
   char                         data[CTRLM_HAL_IP_DATA_MAX_LEN];
   uint32_t                     data_len;
} ctrlm_hal_ip_data_t;
/// @}

/// @addtogroup HAL_IP_Function_Typedefs   Request Function Prototypes
/// @{
/// @brief Functions that are implemented by the IP network device
/// @details The Control Manager provides function type definitions for functions that must be implemented by the RF4CE network.

/// TODO: Update documentation for these functions
typedef ctrlm_hal_result_t (*ctrlm_hal_ip_req_send_data_t)(void *conn, ctrlm_hal_ip_data_t *data);
typedef ctrlm_hal_result_t (*ctrlm_hal_ip_req_close_t)(void *conn);

/// @brief Network Property Get Function Prototype
/// @details This function is called by Control Manager to get a property of the HAL Network.
typedef ctrlm_hal_result_t (*ctrlm_hal_ip_req_property_get_t)(ctrlm_hal_network_property_t property, void **value);
/// @brief Network Property Set Function Prototype
/// @details This function is called by Control Manager to set a property of the HAL Network.
typedef ctrlm_hal_result_t (*ctrlm_hal_ip_req_property_set_t)(ctrlm_hal_network_property_t property, void *value);
// /// @brief Device ID Set Function Prototype
// /// @details This function is called by Control Manager to set the device id on the HAL Network. Specifically used for the PSK implementation of IP Control.
typedef ctrlm_hal_result_t (*ctrlm_hal_ip_req_device_id_set_t)(const char *value);
// /// @brief Security Revoke Function Prototype
// /// @detail This function is called by Control Manager to revoke either a certificate fingerprint or PSK key identity from the HAL network whitelist
typedef ctrlm_hal_result_t (*ctrlm_hal_ip_security_revoke_t)(ctrlm_hal_ip_security_revoke_data_t *data, bool save);
/// @}

/// @addtogroup HAL_IP_Structs Structures
/// @{
/// @brief Structure Definitions
/// @details The Control Manager HAL provides structures that are used in calls to the HAL networks.

/// @brief IP Confirm Init Parameters Structure
/// @details The structure which is passed from the HAL Network device in the IP initialization confirmation.
typedef struct {
   ctrlm_hal_result_t result;
   char version[CTRLM_HAL_NETWORK_VERSION_STRING_SIZE];
   ctrlm_hal_req_term_t                      term;
   ctrlm_hal_ip_req_property_get_t           property_get;
   ctrlm_hal_ip_req_property_set_t           property_set;
   ctrlm_hal_ip_req_device_id_set_t          device_id_set;
   ctrlm_hal_ip_req_close_t                  close;
   ctrlm_hal_ip_req_send_data_t              send_data;
   ctrlm_hal_ip_security_revoke_t            revoke;
} ctrlm_hal_ip_cfm_init_params_t;

/// TODO Update documentation
typedef struct {
   ctrlm_timestamp_t            timestamp;
   ctrlm_hal_ip_permissions_t   permissions;
   ctrlm_hal_ip_security_type_t security_type;
   ctrlm_hal_ip_security_data_t security_data;
   ctrlm_hal_ip_msg_type_t      data_type;
   char                        *data;
   uint32_t                     data_len;
} ctrlm_hal_ip_ind_data_params_t;

/// @brief IP Pair Response Parameters Structure
typedef struct {
   ctrlm_timestamp_t            timestamp;
   char                         data[CTRLM_HAL_IP_DATA_MAX_LEN];
   uint32_t                     data_len;
} ctrlm_hal_ip_rsp_data_params_t;

/// @}

/// @addtogroup HAL_IP_Callback_Typedefs   Callback Function Type Definitions
/// @{
/// @brief Functions that are implemented by the IP Network device
/// @details The Control Manager provides function type definitions for callback functions that can be implemented by the HAL network.

/// TODO: Update documentation
typedef void (*ctrlm_hal_ip_rsp_data_t)(ctrlm_hal_ip_rsp_data_params_t params, void *conn);

/// @}

#ifdef __cplusplus
extern "C" {
#endif

/// @addtogroup HAL_IP_Functions Function Prototypes
/// @{
/// @brief Function that are implemented by the by Control Manager that the HAL can call.
/// @details ControlMgr provides a set of functions that the IP HAL layer can call after initialization is complete.

/// TODO: Update Documentation
typedef ctrlm_hal_result_t (*ctrlm_hal_ip_ind_data_t)(ctrlm_network_id_t id, ctrlm_hal_ip_ind_data_params_t params, ctrlm_hal_ip_rsp_data_params_t *rsp_params, ctrlm_hal_ip_rsp_data_t cb, void *conn);

/// @}


/// @addtogroup HAL_IP_Structs Structures
/// @{

/// @brief Network Initialization Structure
/// @details The structure which is passed from Control Manager to the HAL Network device for initialization.
typedef struct {
   unsigned long                     api_version;      ///< Version of the HAL API
   ctrlm_network_id_t                network_id;       ///< Unique identiifer for the network
   ctrlm_hal_ip_ind_data_t           ind_data;
   ctrlm_hal_ip_port_t               port;             ///< IP Port   
   ctrlm_hal_ip_target_t             target;           ///< IP Target URL
   bool                              wss_logging;      ///< WSS Library logging
} ctrlm_hal_ip_main_init_t;

/// @brief Update Settings Structure
/// @details The structure used to update the settings in the HAL.
typedef struct {
   bool wss_logging;
   char target[CTRLM_HAL_IP_TARGET_LEN];
   unsigned int port;
   unsigned int blackout_threshold;
   unsigned int blackout_time;
} ctrlm_hal_ip_settings_t;

/// @}

/// @addtogroup HAL_IP_Functions Function Prototypes
/// @{

/// @brief IP Network Main
/// @details This function is called to begin the IP HAL Network initialization.  This function must be implemented by the HAL network and directly
/// linked with Control Manager. The provided network id must be used by the network in all communication with the Control Manager.
void *ctrlm_hal_ip_main(ctrlm_hal_ip_main_init_t *main_init);
/// @brief Network Init Confirmation Function
/// @details This function is called after the HAL network initialization is complete.
ctrlm_hal_result_t ctrlm_hal_ip_cfm_init(ctrlm_network_id_t network_id, ctrlm_hal_ip_cfm_init_params_t params);
/// @brief Update Settings Function
/// @details This function is used to update the settings for the IP HAL.
ctrlm_hal_result_t ctrlm_hal_ip_update_settings(ctrlm_hal_ip_settings_t *settings);
/// @brief SHA256 Function
/// @details This function can be utilized by both Control Manager and the HAL Network to calculate SHA256 hash of a buffer
bool ctrlm_ip_util_sha256(unsigned char *buf, size_t len, unsigned char *result);

/// @}

#ifdef __cplusplus
}
#endif

/// @}
#endif
