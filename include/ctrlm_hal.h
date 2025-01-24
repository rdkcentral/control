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
#ifndef _CTRLM_HAL_H_
#define _CTRLM_HAL_H_

#include <time.h>
#include <stdint.h>

/// @file ctrlm_hal.h
///
/// @addtogroup CTRL_MGR_HAL Control Manager HAL API (common)
/// @{

/// @addtogroup HAL_Consts  Constants
/// @{
/// @brief Macros for constant values used in the HAL layer
/// @details The Control Manager HAL provides macros for some parameters which may change in the future.  Network implementations should use
/// these names to allow the network code to function correctly if the values change.

#ifndef CTRLM_HAL_RF4CE_API_VERSION
#define CTRLM_HAL_RF4CE_API_VERSION                   (16) ///< Version of the HAL RF4CE API
#endif

#ifndef CTRLM_HAL_BLE_API_VERSION
#define CTRLM_HAL_BLE_API_VERSION                     (1) ///< Version of the HAL BLE API
#endif

#ifndef CTRLM_HAL_IP_API_VERSION
#define CTRLM_HAL_IP_API_VERSION                      (1)  ///< Version of the HAL IP API
#endif

#if CTRLM_HAL_RF4CE_API_VERSION < 8 || CTRLM_HAL_RF4CE_API_VERSION > 16
#error "Values for CTRLM_HAL_RF4CE_API_VERSION must be in range 8 - 16"
#endif

#if CTRLM_HAL_RF4CE_API_VERSION < 12 && defined(ASB)
#error "ASB is only supported in RF4CE API 12 and above"
#endif

#define CTRLM_HAL_NETWORK_VERSION_STRING_SIZE   (128) ///< Maximum size of a network's version string
#define CTRLM_HAL_NETWORK_AES128_KEY_SIZE        (16) ///< Size of the AES128 key

#define CTRLM_HAL_NETWORK_ID_INVALID           (0xFF) ///< An invalid network identifier
#define CTRLM_HAL_CONTROLLER_ID_INVALID        (0xFF) ///< An invalid controller identifier

#define CTRLM_KEY_STATUS_ATOMIC                (0x04) ///< A key status that sends a keydown AND keyup
/// @}

/// @addtogroup HAL_Enums   Enumerations
/// @{
/// @brief Enumerated Types
/// @details The Control Manager HAL provides enumerated types for logical groups of values.

/// @brief HAL Result Values
/// @details When a Control Manager HAL API has a return code, it will be one of the enumerated values in this list.
typedef enum {
   CTRLM_HAL_RESULT_SUCCESS                  = 0, ///< The operation was completed successfully.
   CTRLM_HAL_RESULT_ERROR                    = 1, ///< An error occurred during the operation.
   CTRLM_HAL_RESULT_ERROR_NETWORK_ID         = 2, ///< The specified HAL Network Id is invalid.
   CTRLM_HAL_RESULT_ERROR_NETWORK_NOT_READY  = 3, ///< The specified HAL Network is not ready.
   CTRLM_HAL_RESULT_ERROR_CONTROLLER_ID      = 4, ///< The specified Controller Id is invalid.
   CTRLM_HAL_RESULT_ERROR_OUT_OF_MEMORY      = 5, ///< An error occurred allocating memory for the operation.
   CTRLM_HAL_RESULT_ERROR_BIND_TABLE_FULL    = 6, ///< The binding table is full.
   CTRLM_HAL_RESULT_ERROR_INVALID_PARAMS     = 7, ///< The operation failed due to invalid parameters.
   CTRLM_HAL_RESULT_ERROR_INVALID_NVM        = 8, ///< The operation failed due to invalid nvm data.
   CTRLM_HAL_RESULT_ERROR_NOT_IMPLEMENTED    = 9, ///< The operation is not implemented in HAL.
   CTRLM_HAL_RESULT_MAX                      = 10 ///< HAL result maximum value
} ctrlm_hal_result_t;

/// @brief Channel Hopping Values
/// @details When a Control Manager HAL API returns a channel hopping value, it will be one of the enumerated values in this list.
typedef enum {
   CTRLM_HAL_FREQUENCY_AGILITY_DISABLE   = 0, ///< Disable frequency agility
   CTRLM_HAL_FREQUENCY_AGILITY_ENABLE    = 1, ///< Enable frequency agility
   CTRLM_HAL_FREQUENCY_AGILITY_NO_CHANGE = 2, ///< No change to frequency agility
   CTRLM_HAL_FREQUENCY_AGILITY_MAX       = 3  ///< Frequency agility maximum value
} ctrlm_hal_frequency_agility_t;

/// @brief Discovery Response Return Values
/// @details Indicates whether the network should respond to the discovery request.
typedef enum {
   CTRLM_HAL_RESULT_DISCOVERY_PENDING = 0, ///< The discovery request's result is still pending.
   CTRLM_HAL_RESULT_DISCOVERY_RESPOND = 1, ///< Respond to the discovery request.
   CTRLM_HAL_RESULT_DISCOVERY_IGNORE  = 2, ///< Ignore the discovery request.
   CTRLM_HAL_RESULT_DISCOVERY_MAX     = 3  ///< Discovery result maximum value
} ctrlm_hal_result_discovery_t;

/// @brief Pair Request Result Values
/// @details Indicates whether the network should respond to the pair request.
typedef enum {
   CTRLM_HAL_RESULT_PAIR_REQUEST_PENDING = 0, ///< The pair request's result is still pending.
   CTRLM_HAL_RESULT_PAIR_REQUEST_RESPOND = 1, ///< Proceed with the pair request.
   CTRLM_HAL_RESULT_PAIR_REQUEST_IGNORE  = 2, ///< Ignore the pair request.
   CTRLM_HAL_RESULT_PAIR_REQUEST_MAX     = 3  ///< Pair request result maximum value
} ctrlm_hal_result_pair_request_t;

/// @brief Pair Result Return Values
/// @details Indicates the result of the pair operation.
typedef enum {
   CTRLM_HAL_RESULT_PAIR_SUCCESS = 0, ///< The pairing operation completed successfully.
   CTRLM_HAL_RESULT_PAIR_FAILURE = 1, ///< The pairing operation has failed.
   CTRLM_HAL_RESULT_PAIR_MAX     = 2  ///< Pair result maximum value
} ctrlm_hal_result_pair_t;

/// @brief Unpair Result Return Values
/// @details Indicates the result of the unpair operation.
typedef enum {
   CTRLM_HAL_RESULT_UNPAIR_PENDING = 0, ///< The unpair operation is still pending.
   CTRLM_HAL_RESULT_UNPAIR_SUCCESS = 1, ///< The unpair operation completed succesfully.
   CTRLM_HAL_RESULT_UNPAIR_FAILURE = 2, ///< The unpair operation has failed.
   CTRLM_HAL_RESULT_UNPAIR_MAX     = 3  ///< Unpair result maximum value
} ctrlm_hal_result_unpair_t;

/// @brief Thread Monitor Response Values
/// @details Indicates the result of a thread monitor poll.
typedef enum {
   CTRLM_HAL_THREAD_MONITOR_RESPONSE_DEAD  = 0,
   CTRLM_HAL_THREAD_MONITOR_RESPONSE_ALIVE = 1
} ctrlm_hal_thread_monitor_response_t;

/// @brief Network Properties
/// @details The properties enumeration is used in calls to Network Property Get/Set routines in the network instance.  They are used to get/set values for the HAL Network.
typedef enum {
   CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_LIST     = 0, ///< RO - List of controllers paired with this network
   CTRLM_HAL_NETWORK_PROPERTY_NETWORK_STATS       = 1, ///< RO - Statistics for the network
   CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_STATS    = 2, ///< RO - Statistics for a particular controller
   CTRLM_HAL_NETWORK_PROPERTY_FREQUENCY_AGILITY   = 3, ///< RW - Frequency Agility state
   CTRLM_HAL_NETWORK_PROPERTY_PAIRING_TABLE_ENTRY = 4, ///< RW - Pairing table entry
   CTRLM_HAL_NETWORK_PROPERTY_FACTORY_RESET       = 5, ///< WO - Reset the HAL layer to factory defaults (no structure is passed)
   CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_IMPORT   = 6, ///< RO - Import a particular controller
   CTRLM_HAL_NETWORK_PROPERTY_THREAD_MONITOR      = 7, ///< WO - Thread monitoring request
   CTRLM_HAL_NETWORK_PROPERTY_ENCRYPTION_KEY      = 8, ///< WO - Set Encryption Key request
#if CTRLM_HAL_RF4CE_API_VERSION >= 10
   CTRLM_HAL_NETWORK_PROPERTY_DPI_CONTROL         = 9, ///< WO - DPI control
#if CTRLM_HAL_RF4CE_API_VERSION >= 14
   CTRLM_HAL_NETWORK_PROPERTY_NVM_VERSION         = 10, ///< RO - Current NVM version
   CTRLM_HAL_NETWORK_PROPERTY_INDIRECT_TX_TIMEOUT = 11, ///< WO - Timeout for indirect tx packet in milliseconds, used for mac polling
#if CTRLM_HAL_RF4CE_API_VERSION >= 16
   CTRLM_HAL_NETWORK_PROPERTY_AUTO_ACK            = 12, ///< RW - Auto Acknowledge Packets
#endif // 16
#endif // 14
#endif // 10
   CTRLM_HAL_NETWORK_PROPERTY_MAX                      ///< NA - Network property maximum value
} ctrlm_hal_network_property_t;
/// @}

/// @addtogroup HAL_Types Type Definitions
/// @{
/// @brief Type definitions
/// @details The Control Manager HAL provides type definitions for Id's that are passed between the Control Manager and network networks.

/// @brief Network Id Type
/// @details During initialization, of the HAL network, Control Manager will assign a unique id to the network.  It must be used in all
/// subsequent communication with the Control Manager.
typedef unsigned char   ctrlm_network_id_t;
/// @brief Controller Id Type
/// @details When a controller is paired, the Control Manager will assign an id (typically 48/64 bit MAC address) to the controller.
typedef unsigned char   ctrlm_controller_id_t;
/// @brief Timestamp Type
/// @details When a timestamp is required, this type will be used to store the timestamp.  Control manager provides an interface to get and manipulate timestamp values.
/// Timestamps should not be directly manipulated by the HAL network device.
typedef struct timespec ctrlm_timestamp_t;

/// @}

/// @addtogroup HAL_Function_Typedefs   HAL Network Request Function Prototypes
/// @{
/// @brief Functions that are implemented by the HAL Network
/// @details The Control Manager provides function type definitions for functions that must be implemented by the HAL network.

/// @brief Network Property Get Function Prototype
/// @details This function is called by Control Manager to get a property of the HAL Network device. This request must be performed synchronously.
/// @param[in]  property - Network property to get
/// @param[out] value    - Pointer to a pointer of the value for the network property
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_req_property_get_t)(ctrlm_hal_network_property_t property, void **value);
/// @brief Network Property Set Function Prototype
/// @details This function is called by Control Manager to set a property of the HAL Network device. This request must be performed synchronously.
/// @param[in] property - Network property to set
/// @param[in] value    - Pointer to the value for the network property
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_req_property_set_t)(ctrlm_hal_network_property_t property, void *value);
/// @brief Network Terminate Function Prototype
/// @details This function is called by Control Manager to terminate the HAL network.
/// @return CTRLM_HAL_RESULT_SUCCESS for success.  All other values indicate an error has occurred.
typedef ctrlm_hal_result_t (*ctrlm_hal_req_term_t)(void);

/// @}

/// @addtogroup HAL_Structs Structures
/// @{
/// @brief Structure Definitions
/// @details The Control Manager HAL provides structures that are used in calls to the HAL networks.

/// @brief Network Initialization Structure
/// @details The structure which is passed from Control Manager to the HAL Network device for initialization.
typedef struct {
   ctrlm_network_id_t network_id; ///< Unique identifier for the network
} ctrlm_hal_main_init_t;

/// @brief Network Property Controller List
/// @details The structure which is passed from the HAL Network device to Control Manager in the network_property_get request with property value CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_LIST.
///          GET: The memory for this structure must be allocated by the HAL driver (ctrlm_hal_malloc) since the size is not known.  Control manager is responsible for freeing the memory.
///          SET: Not applicable
typedef struct {
   unsigned long          quantity;       ///< Number of paired controllers
   ctrlm_controller_id_t *controller_ids; ///< Array of controller ids
   unsigned long long    *ieee_addresses; ///< Array of ieee addresses of the controllers
} ctrlm_hal_network_property_controller_list_t;

/// @brief Network Property Network Statistics
/// @details The structure which is passed from the HAL Network device to Control Manager in the network_property_get request with property value CTRLM_HAL_NETWORK_PROPERTY_NETWORK_STATS.
///          GET: The memory for this structure is allocated by Control Manager since the size is fixed.  The value parameter contains a pointer to this structure.
///          SET: Not applicable
typedef struct {
   unsigned char          rf_channel;     ///< Current RF channel in use
   unsigned char          rf_quality;     ///< Quality of the current RF channel
#if CTRLM_HAL_RF4CE_API_VERSION >= 15
   unsigned long long    ieee_address; ///< ieee addresses read from chip
#endif
} ctrlm_hal_network_property_network_stats_t;

/// @brief Network Property Controller Statistics
/// @details The structure which is passed from the HAL Network device to Control Manager in the network_property_get request with property value CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_STATS.
///          GET: The memory for this structure must be allocated by the Control Manager since input parameters are provided to the HAL and the size is fixed.  The value parameter contains a pointer to this structure.
///          SET: Not applicable
typedef struct {
   ctrlm_controller_id_t controller_id;   ///< IN  - Controller Identifier
   unsigned short        short_address;   ///< OUT - Short Address of the controller
#if CTRLM_HAL_RF4CE_API_VERSION >= 14
   ctrlm_timestamp_t     checkin_time;    ///< OUT - Timestamp indicating the most recent poll indication of the controller
#endif /* CTRLM_HAL_RF4CE_API_VERSION */
} ctrlm_hal_network_property_controller_stats_t;

/// @brief Network Property Frequency Agility
/// @details The structure which is passed from the HAL Network device to Control Manager in the network_property_get or set request with property value CTRLM_HAL_NETWORK_PROPERTY_FREQUENCY_AGILITY.
///          GET/SET: The memory for this structure is allocated by Control Manager since the size is fixed.  The value parameter contains a pointer to this structure.
typedef struct {
   ctrlm_hal_frequency_agility_t state;   ///< Frequency agility state
} ctrlm_hal_network_property_frequency_agility_t;

/// @brief Network Property Pairing Table Entry
/// @details The structure which is passed from the HAL Network device to Control Manager in the network_property_get or set request with property value CTRLM_HAL_NETWORK_PROPERTY_PAIRING_TABLE_ENTRY.
///          GET/SET: The memory for this structure is allocated by Control Manager since the size is fixed.  The value parameter contains a pointer to this structure.
typedef struct {
   ctrlm_controller_id_t controller_id;   ///< Controller Identifier
   void *                pairing_data;    ///< Pairing Data
} ctrlm_hal_network_property_pairing_table_entry_t;

/// @brief Network Property Controller Import
/// @details The structure which is passed from the HAL Network device to Control Manager in the network_property_get request with property value CTRLM_HAL_NETWORK_PROPERTY_CONTROLLER_IMPORT.
///          GET: The memory for this structure must be allocated by the Control Manager since input parameters are provided to the HAL and the size is fixed.  The value parameter contains a pointer to this structure.
///          SET: Not applicable
typedef struct {
   ctrlm_controller_id_t controller_id; ///< IN  - Controller Identifier (HAL layer must replace mapping from IEEE address to controller id)
   unsigned long long    ieee_address;  ///< IN  - IEEE Address of the controller
   unsigned char         autobind;      ///< OUT - Set to (1) if the controller was bound using autobind otherwise set to (0),
   time_t                time_binding;  ///< OUT - Time that the controller was bound or 0 if unknown
   time_t                time_last_key; ///< OUT - Time that the last key was received from the controller or 0 if unknown
} ctrlm_hal_network_property_controller_import_t;

/// @brief Network Property Thread Monitor
/// @details The structure which is passed from the HAL Network device to Control Manager in the network_property_get request with property value CTRLM_HAL_NETWORK_PROPERTY_THREAD_MONITOR.
///          GET: Not applicable
///          SET: The memory for this structure is allocated by Control Manager.  The value parameter contains a pointer to this structure.
typedef struct {
   ctrlm_hal_thread_monitor_response_t *response; ///< IN - Pointer to thread monitor response.  HAL layer must check to ensure all threads are responsive.  If they are responsive, set the value to CTRLM_HAL_THREAD_MONITOR_RESPONSE_ALIVE.
} ctrlm_hal_network_property_thread_monitor_t;

/// @brief Network Property Encryption Key
/// @details The structure which is passed from the HAL Network device to Control Manager in the network_property_set request with property value CTRLM_HAL_NETWORK_PROPERTY_ENCRYPTION_KEY.
///          GET: Not applicable
///          SET: The memory for this structure is allocated by Control Manager.  The value parameter contains a pointer to this structure.
typedef struct {
   ctrlm_controller_id_t     controller_id;                                 ///< Identifier of the controller
   unsigned char             aes128_key[CTRLM_HAL_NETWORK_AES128_KEY_SIZE]; ///< AES128 Encryption key
} ctrlm_hal_network_property_encryption_key_t;

#if CTRLM_HAL_RF4CE_API_VERSION >= 10
/// @brief Network Property DPI Control
/// @details The structure which is passed from the HAL Network device to Control Manager in the network_property_set request with property value CTRLM_HAL_NETWORK_PROPERTY_DPI_CONTROL.
///          GET: Not applicable
///          SET: The memory for this structure is allocated by Control Manager.  The value parameter contains a pointer to this structure. If dpi_enable is 0, params MUST be NULL.
///               If dpi_enable is 1 AND defaults is 1, the HAL MUST use it's default patterns for Deepsleep.
///               If dpi_enable is 1 AND defaults is 0, the HAL MUST use the params for it's Deepsleep patterns.
typedef struct {
   unsigned char             dpi_enable; ///< 1 - enable, 0 - disable
#if (CTRLM_HAL_RF4CE_API_VERSION >= 11)
   unsigned char             defaults;   ///< 1 - Use default, 0 - Use params
   void *                    params;
#endif
 } ctrlm_hal_network_property_dpi_control_t;
#endif

/// @}

#ifdef __cplusplus
extern "C" {
#endif

/// @addtogroup CTRLM_Functions Control Manager Function Prototypes
/// @{
/// @brief Function that are implemented by Control Manager for use by the HAL network device
/// @details The Control Manager provides a set of functions that the HAL Network can call after initialization is complete.

/// @brief Memory Allocation Function Prototype
/// @details 
/// @param[in] size - Number of bytes of memory to request
/// @return Void pointer to the memory or NULL if an error occurred.
void *ctrlm_hal_malloc(unsigned long size);
/// @brief Memory Free Function Prototype
/// @details 
/// @param[in] ptr - Pointer to memory previously allocated with ctrlm_hal_malloc
/// @return None
void  ctrlm_hal_free(void *ptr);

/// @brief Timestamp Get Function Prototype
/// @details This function returns the current timestamp.
/// @param[out] timestamp - Pointer to a ctrlm_timestamp_t structure in which to return the current realtime clock timestamp.
/// @return None
void               ctrlm_timestamp_get(ctrlm_timestamp_t *timestamp);
/// @brief Timestamp Get Function Prototype
/// @details This function returns the current timestamp.
/// @param[out] timestamp - Pointer to a ctrlm_timestamp_t structure in which to return the current monotonic clock timestamp.
/// @return None
void               ctrlm_timestamp_get_monotonic(ctrlm_timestamp_t *timestamp);
/// @brief Timestamp Compare Function Prototype
/// @details This function compares two timestamps.
/// @param[in] one - First timestamp
/// @param[in] two - Second timestamp
/// @return Returns 0 (equal), +1 (one > two), or -1 (one < two).
int                ctrlm_timestamp_cmp(ctrlm_timestamp_t one, ctrlm_timestamp_t two);
/// @brief Timestamp Subtract (ns) Function Prototype
/// @details This function subtracts timestamp one from two and returns the difference in nanoseconds.
/// @param[in] one - First timestamp
/// @param[in] two - Second timestamp
/// @return Returns the difference in nanoseconds.
signed long long   ctrlm_timestamp_subtract_ns(ctrlm_timestamp_t one, ctrlm_timestamp_t two);
/// @brief Timestamp Subtract (us) Function Prototype
/// @details This function subtracts timestamp one from two and returns the difference in microseconds.
/// @param[in] one - First timestamp
/// @param[in] two - Second timestamp
/// @return Returns the difference in microseconds.
signed long long   ctrlm_timestamp_subtract_us(ctrlm_timestamp_t one, ctrlm_timestamp_t two);
/// @brief Timestamp Subtract (ms) Function Prototype
/// @details This function subtracts timestamp one from two and returns the difference in milliseconds.
/// @param[in] one - First timestamp
/// @param[in] two - Second timestamp
/// @return Returns the difference in milliseconds.
signed long long   ctrlm_timestamp_subtract_ms(ctrlm_timestamp_t one, ctrlm_timestamp_t two);
/// @brief Timestamp Add (ns) Function Prototype
/// @details This function adds the specified number of nanoseconds to the timestamp.
/// @param[in] timestamp   - Pointer to a ctrlm_timestamp_t structure to add to.
/// @param[in] nanoseconds - Number of nanoseconds to add.
/// @return None
void               ctrlm_timestamp_add_ns(ctrlm_timestamp_t *timestamp, unsigned long nanoseconds);
/// @brief Timestamp Add (us) Function Prototype
/// @details This function adds the specified number of microseconds to the timestamp.
/// @param[in] timestamp    - Pointer to a ctrlm_timestamp_t structure to add to.
/// @param[in] microseconds - Number of microseconds to add.
/// @return None
void               ctrlm_timestamp_add_us(ctrlm_timestamp_t *timestamp, unsigned long microseconds);
/// @brief Timestamp Add (ms) Function Prototype
/// @details This function adds the specified number of milliseconds to the timestamp.
/// @param[in] timestamp    - Pointer to a ctrlm_timestamp_t structure to add to.
/// @param[in] milliseconds - Number of milliseconds to add.
/// @return None
void               ctrlm_timestamp_add_ms(ctrlm_timestamp_t *timestamp, unsigned long milliseconds);
/// @brief Timestamp Add (secs) Function Prototype
/// @details This function adds the specified number of seconds to the timestamp.
/// @param[in] timestamp - Pointer to a ctrlm_timestamp_t structure to add to.
/// @param[in] seconds   - Number of seconds to add.
/// @return None
void               ctrlm_timestamp_add_secs(ctrlm_timestamp_t *timestamp, unsigned long seconds);
/// @brief Timestamp Until (ns) Function Prototype
/// @details This function returns the amount of time (in ns) until the timestamp is reached.
/// @param[in] timestamp - Timestamp to compare against
/// @return Returns the number of nanoseconds until the timestamp is reached or 0 if the time has already passed.
unsigned long long ctrlm_timestamp_until_ns(ctrlm_timestamp_t timestamp);
/// @brief Timestamp Until (us) Function Prototype
/// @details This function returns the amount of time (in us) until the timestamp is reached.
/// @param[in] timestamp - Timestamp to compare against
/// @return Returns the number of microseconds until the timestamp is reached or 0 if the time has already passed.
unsigned long long ctrlm_timestamp_until_us(ctrlm_timestamp_t timestamp);
/// @brief Timestamp Until (ms) Function Prototype
/// @details This function returns the amount of time (in ms) until the timestamp is reached.
/// @param[in] timestamp - Timestamp to compare against
/// @return Returns the number of milliseconds until the timestamp is reached or 0 if the time has already passed.
unsigned long long ctrlm_timestamp_until_ms(ctrlm_timestamp_t timestamp);
/// @brief Timestamp Since (ns) Function Prototype
/// @details This function returns the amount of time (in ns) that has elapsed since a timestamp.
/// @param[in] timestamp - Timestamp to compare against
/// @return Returns the number of nanoseconds since the timestamp was reached or 0 if the time has not passed yet.
unsigned long long ctrlm_timestamp_since_ns(ctrlm_timestamp_t timestamp);
/// @brief Timestamp Since (us) Function Prototype
/// @details This function returns the amount of time (in us) that has elapsed since a timestamp.
/// @param[in] timestamp - Timestamp to compare against
/// @return Returns the number of microseconds since the timestamp was reached or 0 if the time has not passed yet.
unsigned long long ctrlm_timestamp_since_us(ctrlm_timestamp_t timestamp);
/// @brief Timestamp Since (ms) Function Prototype
/// @details This function returns the amount of time (in ms) that has elapsed since a timestamp.
/// @param[in] timestamp - Timestamp to compare against
/// @return Returns the number of milliseconds since the timestamp was reached or 0 if the time has not passed yet.
unsigned long long ctrlm_timestamp_since_ms(ctrlm_timestamp_t timestamp);

/// @brief Network Result String
/// @details This function returns a string representation of the HAL network result type.
/// @param[in] result - HAL result enumeration value
/// @return String representation of the enumeration value
const char *ctrlm_hal_result_str(ctrlm_hal_result_t result);
/// @brief Network Frequency Agility String
/// @details This function returns a string representation of the HAL network frequency agility type.
/// @param[in] frequency_agility - HAL frequency agility enumeration value
/// @return String representation of the enumeration value
const char *ctrlm_hal_frequency_agility_str(ctrlm_hal_frequency_agility_t frequency_agility);
/// @brief Network Discovery Result String
/// @details This function returns a string representation of the HAL network discovery result.
/// @param[in] result - HAL discovery result enumeration value
/// @return String representation of the enumeration value
const char *ctrlm_hal_result_discovery_str(ctrlm_hal_result_discovery_t result);
/// @brief Network Pair Request Result String
/// @details This function returns a string representation of the HAL network pair request result.
/// @param[in] result - HAL pair request result enumeration value
/// @return String representation of the enumeration value
const char *ctrlm_hal_result_pair_request_str(ctrlm_hal_result_pair_request_t result);
/// @brief Network Pair Result String
/// @details This function returns a string representation of the HAL network pair result.
/// @param[in] result - HAL pair result enumeration value
/// @return String representation of the enumeration value
const char *ctrlm_hal_result_pair_str(ctrlm_hal_result_pair_t result);
/// @brief Network Unpair Result String
/// @details This function returns a string representation of the HAL network unpair result.
/// @param[in] result - HAL unpair result enumeration value
/// @return String representation of the enumeration value
const char *ctrlm_hal_result_unpair_str(ctrlm_hal_result_unpair_t result);
/// @brief Network Property String
/// @details This function returns a string representation of the HAL network property type.
/// @param[in] property - HAL network property enumeration value
/// @return String representation of the enumeration value
const char *ctrlm_hal_network_property_str(ctrlm_hal_network_property_t property);
/// @}

#ifdef __cplusplus
}
#endif
/// @}
/// @}
#endif
