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

#ifndef _CTRLM_IPC_VOICE_H_
#define _CTRLM_IPC_VOICE_H_

#include "ctrlm_ipc.h"
/// @file ctrlm_ipc_voice.h
///
/// @defgroup CTRLM_IPC_VOICE IARM API - Voice
/// @{
///
/// @defgroup CTRLM_IPC_VOICE_COMMS       Communication Interfaces
/// @defgroup CTRLM_IPC_VOICE_CALLS       IARM Remote Procedure Calls
/// @defgroup CTRLM_IPC_VOICE_EVENTS      IARM Events
/// @defgroup CTRLM_IPC_VOICE_DEFINITIONS Constants
/// @defgroup CTRLM_IPC_VOICE_ENUMS       Enumerations
/// @defgroup CTRLM_IPC_VOICE_STRUCTS     Structures

/// @addtogroup CTRLM_IPC_VOICE_DEFINITIONS
/// @{
/// @brief Constant Defintions
/// @details The Control Manager provides definitions for constant values.

#define CTRLM_VOICE_IARM_CALL_UPDATE_SETTINGS            "Voice_UpdateSettings"          ///< IARM Call to update settings
#define CTRLM_VOICE_IARM_CALL_SESSION_BEGIN              "Voice_SessionBegin"            ///< starts a voice streaming session
#define CTRLM_VOICE_IARM_CALL_SESSION_END                "Voice_SessionEnd"              ///< ends a voice streaming session

#define CTRLM_VOICE_IARM_CALL_STATUS                     "Voice_Status"                  ///< IARM Call to get status
#define CTRLM_VOICE_IARM_CALL_CONFIGURE_VOICE            "Voice_ConfigureVoice"          ///< IARM Call to set up voice with JSON payload
#define CTRLM_VOICE_IARM_CALL_SET_VOICE_INIT             "Voice_SetVoiceInit"            ///< IARM Call to set application data with JSON payload in the voice server init message
#define CTRLM_VOICE_IARM_CALL_SEND_VOICE_MESSAGE         "Voice_SendVoiceMessage"        ///< IARM Call to send JSON payload to voice server
#define CTRLM_VOICE_IARM_CALL_SESSION_TYPES              "Voice_SessionTypes"            ///< IARM Call to get voice session request types
#define CTRLM_VOICE_IARM_CALL_SESSION_REQUEST            "Voice_SessionRequest"          ///< IARM Call to request a voice session
#define CTRLM_VOICE_IARM_CALL_SESSION_TERMINATE          "Voice_SessionTerminate"        ///< IARM Call to terminate a voice session
#define CTRLM_VOICE_IARM_CALL_SESSION_AUDIO_STREAM_START "Voice_SessionAudioStreamStart" ///< IARM Call to start a subsequent audio stream in a voice session

#define CTRLM_VOICE_IARM_CALL_RESULT_LEN_MAX       (2048) ///< IARM Call result length

#define CTRLM_VOICE_IARM_BUS_API_REVISION             (9) ///< Revision of the Voice IARM API
#define CTRLM_VOICE_MIME_MAX_LENGTH                  (64) ///< Audio mime string maximum length
#define CTRLM_VOICE_SUBTYPE_MAX_LENGTH               (64) ///< Audio subtype string maximum length
#define CTRLM_VOICE_LANG_MAX_LENGTH                  (16) ///< Audio language string maximum length
#define CTRLM_VOICE_SERVER_URL_MAX_LENGTH          (2048) ///< Server url string maximum length
#define CTRLM_VOICE_GUIDE_LANGUAGE_MAX_LENGTH        (16) ///< Guide language string maximum length
#define CTRLM_VOICE_ASPECT_RATIO_MAX_LENGTH          (16) ///< Aspect ratio string maximum length
#define CTRLM_VOICE_SESSION_ID_MAX_LENGTH            (64) ///< Session Id string maximum length
#define CTRLM_VOICE_SESSION_TEXT_MAX_LENGTH         (512) ///< Session text string maximum length
#define CTRLM_VOICE_SESSION_MSG_MAX_LENGTH          (128) ///< Session message string maximum length
#define CTRLM_VOICE_QUERY_STRING_MAX_LENGTH         (128) ///< Query string maximum name or value length
#define CTRLM_VOICE_QUERY_STRING_MAX_PAIRS           (16) ///< Query string maximum number of name/value pairs
#define CTRLM_VOICE_REQUEST_IP_MAX_LENGTH            (48) ///< cURL request primary IP address string maximum length (big enough for IPv6)

#define CTRLM_VOICE_MIN_UTTERANCE_DURATION_MAXIMUM  (600) ///< Maximum value of the utterance duration minimum setting (in milliseconds)

// Bitmask defines for setting the available value in vrex_mgr_iarm_bus_event_voice_settings_t
#define CTRLM_VOICE_SETTINGS_VOICE_ENABLED            (0x0001) ///< Setting to enable/disable voice control
#define CTRLM_VOICE_SETTINGS_VREX_SERVER_URL          (0x0002) ///< Setting to update the vrex server url string
#define CTRLM_VOICE_SETTINGS_GUIDE_LANGUAGE           (0x0004) ///< Setting to update the guide language string
#define CTRLM_VOICE_SETTINGS_ASPECT_RATIO             (0x0008) ///< Setting to update the aspect ratio string
#define CTRLM_VOICE_SETTINGS_UTTERANCE_DURATION       (0x0010) ///< Setting to update the minimum utterance duration value
#define CTRLM_VOICE_SETTINGS_VREX_SAT_ENABLED         (0x0020) ///< Setting to enable/disable Service Access Token (SAT)
#define CTRLM_VOICE_SETTINGS_QUERY_STRINGS            (0x0040) ///< Setting to update the request query strings
#define CTRLM_VOICE_SETTINGS_FARFIELD_VREX_SERVER_URL (0x0080) ///< Setting to update the farfield vrex server url string
#define CTRLM_VOICE_SETTINGS_MIC_TAP_SERVER_URL       (0x0100) ///< Setting to update the microphone tap server url string

/// @}

/// @addtogroup CTRLM_IPC_VOICE_ENUMS
/// @{
/// @brief Enumerated Types
/// @details The Control Manager provides enumerated types for logical groups of values.

/// @brief Voice Session Results
/// @details An enumeration of the overall result for a voice session.
typedef enum {
   CTRLM_VOICE_SESSION_RESULT_SUCCESS           = 0, ///< Voice session completed successfully
   CTRLM_VOICE_SESSION_RESULT_FAILURE           = 1, ///< Voice session completed unsuccessfully
   CTRLM_VOICE_SESSION_RESULT_MAX               = 2  ///< Voice session result maximum value
} ctrlm_voice_session_result_t;

/// @brief Voice Session End Reasons
/// @details An enumeration of the reasons that cause a voice session to end.
typedef enum {
   CTRLM_VOICE_SESSION_END_REASON_DONE                    = 0, ///< Session completed normally
   CTRLM_VOICE_SESSION_END_REASON_TIMEOUT_FIRST_PACKET    = 1, ///< Session ended due to timeout on the first audio sample
   CTRLM_VOICE_SESSION_END_REASON_TIMEOUT_INTERPACKET     = 2, ///< Session ended due to timeout on a subsequent audio sample
   CTRLM_VOICE_SESSION_END_REASON_TIMEOUT_MAXIMUM         = 3, ///< Session ended due to maximum duration
   CTRLM_VOICE_SESSION_END_REASON_ADJACENT_KEY_PRESSED    = 4, ///< Session ended due to adjacent key press
   CTRLM_VOICE_SESSION_END_REASON_OTHER_KEY_PRESSED       = 5, ///< Session ended due to any other key press
   CTRLM_VOICE_SESSION_END_REASON_OTHER_ERROR             = 6, ///< Session ended due to any other reason
   CTRLM_VOICE_SESSION_END_REASON_NEW_SESSION             = 7, ///< Session ended due to a new voice session request before previous session is ended
   CTRLM_VOICE_SESSION_END_REASON_MINIMUM_QOS             = 8, ///< Session ended due to low quality of service
   CTRLM_VOICE_SESSION_END_REASON_MAX                     = 9  ///< Session End Reason maximum value
} ctrlm_voice_session_end_reason_t;

/// @brief Voice Session Abort Reasons
/// @details An enumeration of the reasons that cause a voice session to be aborted.
typedef enum {
   CTRLM_VOICE_SESSION_ABORT_REASON_BUSY                  =  0, ///< Session aborted because another session in progress
   CTRLM_VOICE_SESSION_ABORT_REASON_SERVER_NOT_READY      =  1, ///< Session aborted because the server cannot be reached
   CTRLM_VOICE_SESSION_ABORT_REASON_AUDIO_FORMAT          =  2, ///< Session aborted due to failure to negotiate an audio format
   CTRLM_VOICE_SESSION_ABORT_REASON_FAILURE               =  3, ///< Session aborted for any other reason
   CTRLM_VOICE_SESSION_ABORT_REASON_VOICE_DISABLED        =  4, ///< Session aborted because the voice feature is disabled
   CTRLM_VOICE_SESSION_ABORT_REASON_DEVICE_UPDATE         =  5, ///< Session aborted due to device update in progress
   CTRLM_VOICE_SESSION_ABORT_REASON_NO_RECEIVER_ID        =  6, ///< Session aborted because there is no receiver id
   CTRLM_VOICE_SESSION_ABORT_REASON_NEW_SESSION           =  7, ///< Session aborted because the remote's previous session is still active
   CTRLM_VOICE_SESSION_ABORT_REASON_INVALID_CONTROLLER_ID =  8, ///< Session aborted because the controller id isn't valid
   CTRLM_VOICE_SESSION_ABORT_REASON_APPLICATION_RESTART   =  9, ///< Session aborted due to restarting controlMgr.
   CTRLM_VOICE_SESSION_ABORT_REASON_MAX                   = 10  ///< Session Abort Reason maximum value
} ctrlm_voice_session_abort_reason_t;

/// @brief Voice Internal Errors
/// @details An enumeration of the Voice component's internal error codes.
typedef enum {
   CTRLM_VOICE_INTERNAL_ERROR_NONE              = 0, ///< No internal error occurred
   CTRLM_VOICE_INTERNAL_ERROR_EXCEPTION         = 1, ///< An exception generated an internal error
   CTRLM_VOICE_INTERNAL_ERROR_THREAD_CREATE     = 2, ///< Failure to launch a session thread
   CTRLM_VOICE_INTERNAL_ERROR_MAX               = 3  ///< Internal error type maximum value
} ctrlm_voice_internal_error_t;

/// @brief Voice Reset Types
/// @details An enumeration of the types of reset that can occur on a controller.
typedef enum {
   CTRLM_VOICE_RESET_TYPE_POWER_ON                   = 0, ///< Normal power up by inserting batteries
   CTRLM_VOICE_RESET_TYPE_EXTERNAL                   = 1, ///< Reset due to an external condition
   CTRLM_VOICE_RESET_TYPE_WATCHDOG                   = 2, ///< Reset due to watchdog timer expiration
   CTRLM_VOICE_RESET_TYPE_CLOCK_LOSS                 = 3, ///< Reset due to loss of main clock
   CTRLM_VOICE_RESET_TYPE_BROWN_OUT                  = 4, ///< Reset due to a low voltage condition
   CTRLM_VOICE_RESET_TYPE_OTHER                      = 5, ///< Reset due to any other reason
   CTRLM_VOICE_RESET_TYPE_MAX                        = 6  ///< Reset type maximum value
} ctrlm_voice_reset_type_t;

/// @}

/// @addtogroup CTRLM_IPC_VOICE_STRUCTS
/// @{
/// @brief Structure Definitions
/// @details The Control Manager provides structures that are used in IARM calls and events.

typedef struct {
    char    name[CTRLM_VOICE_QUERY_STRING_MAX_LENGTH];      ///< The name (null terminated string) for a query
    char    value[CTRLM_VOICE_QUERY_STRING_MAX_LENGTH];     ///< The value (null terminated string) for a query
} ctrlm_voice_query_pair_t;

typedef struct {
    unsigned char               pair_count;                                         ///< The number of name/value pairs in the following array
    ctrlm_voice_query_pair_t    query_string[CTRLM_VOICE_QUERY_STRING_MAX_PAIRS];   ///< An array of name/value pairs to contruct query strings
} ctrlm_voice_query_strings_t;

typedef struct {
   unsigned char                api_revision;                                              ///< The revision of this API.
   ctrlm_iarm_call_result_t     result;                                                    ///< Result of the IARM call
   unsigned long                available;                                                 ///< Bitmask indicating the settings that are available in this event
   unsigned char                voice_control_enabled;                                     ///< Boolean value to enable (non-zero) or disable (zero) voice control
   unsigned char                vrex_sat_enabled;                                          ///< Boolean value to enable (non-zero) or disable (zero) Service Access Token in requests to vrex server
   char                         vrex_server_url[CTRLM_VOICE_SERVER_URL_MAX_LENGTH];        ///< The url for the vrex server (null terminated string)
   char                         guide_language[CTRLM_VOICE_GUIDE_LANGUAGE_MAX_LENGTH];     ///< The guide's language [pass-thru] (null terminated string)
   char                         aspect_ratio[CTRLM_VOICE_ASPECT_RATIO_MAX_LENGTH];         ///< The guide's aspect ratio [pass-thru] (null terminated string)
   unsigned long                utterance_duration_minimum;                                ///< The minimum duration of an utterance (in milliseconds).  A value of zero disables utterance duration checking.
   ctrlm_voice_query_strings_t  query_strings;                                             ///< Query string name/value pairs, for inclusion in the VREX request
   char                         server_url_vrex_src_ff[CTRLM_VOICE_SERVER_URL_MAX_LENGTH]; ///< The url for the farfield vrex server (null terminated string)
   char                         server_url_src_mic_tap[CTRLM_VOICE_SERVER_URL_MAX_LENGTH]; ///< The url for the microphone tap server (null terminated string)
} ctrlm_voice_iarm_call_settings_t;

typedef struct {
   unsigned char available;        ///< Boolean value indicating that statistics are available (1) or not (0)
   unsigned long rf_channel;       ///< The rf channel that the voice session used (typically 15, 20 or 25)
   unsigned long buffer_watermark; ///< The highest local buffer level (estimated) in packets
   unsigned long packets_total;    ///< Total number of voice packets in the transmission
   unsigned long packets_lost;     ///< Number of packets lost in the transmission
   unsigned long dropped_retry;    ///< Number of packets dropped by the remote due to retry limit
   unsigned long dropped_buffer;   ///< Number of packets dropped by the remote due to insufficient local buffering
   unsigned long retry_mac;        ///< Total number of MAC retries during the session
   unsigned long retry_network;    ///< Total number of network level retries during the transmission
   unsigned long cca_sense;        ///< Total number of times a packet was not send due to energy detected over the CCA threshold
   unsigned long link_quality;     ///< The average link quality of all the voice packets in the transmission.
} ctrlm_voice_stats_session_t;

typedef struct {
   unsigned char            available;             ///< Boolean value indicating that a remote control reset was detected (1) or not (0)
   ctrlm_voice_reset_type_t reset_type;            ///< The type of reset that occurred
   unsigned char            voltage;               ///< RCU's voltage from 0.0 V (0) to 4.0 V (0xFF).  The value 0xFF indicates that the voltage is not available.
   unsigned char            battery_percentage;    ///< RCU's battery percentage from 0-100.
} ctrlm_voice_stats_reboot_t;


typedef struct {
   unsigned char              api_revision;        ///< The revision of this API.
   ctrlm_network_id_t         network_id;          ///< Identifier of network
   ctrlm_controller_id_t      controller_id;       ///< A unique identifier of the remote
   unsigned long long         ieee_address;        ///< IEEE MAC address of the remote
   ctrlm_iarm_call_result_t   result;              ///< OUT - The result of the operation.
} ctrlm_voice_iarm_call_voice_session_t;

typedef struct {
   unsigned char         api_revision;                             ///< The revision of this API.
   ctrlm_network_id_t    network_id;                               ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t  network_type;                             ///< Type of network on which the controller is bound
   ctrlm_controller_id_t controller_id;                            ///< A unique identifier of the remote
   unsigned long         session_id;                               ///< A unique id for the voice session.
   unsigned char         mime_type[CTRLM_VOICE_MIME_MAX_LENGTH];   ///< The mime type of the data audio/vnd.wave;codec=1 for PCM or audio/x-adpcm for ADPCM. see http://www.isi.edu/in-notes/rfc2361.txt wrt mime types
   unsigned char         sub_type[CTRLM_VOICE_SUBTYPE_MAX_LENGTH]; ///< The subtype (using exising definitions such as PCM_16_16K, PCM_16_32K, PCM_16_22K)
   unsigned char         language[CTRLM_VOICE_LANG_MAX_LENGTH];    ///< The language code (ISO-639-1 format)
   unsigned char         is_voice_assistant;                       ///< Boolean indicating if the device is a far-field device (1) as opposed to a hand-held remote (0).
} ctrlm_voice_iarm_event_session_begin_t;

typedef struct {
   unsigned char                    api_revision;         ///< The revision of this API.
   ctrlm_network_id_t               network_id;           ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t             network_type;         ///< Type of network on which the controller is bound
   ctrlm_controller_id_t            controller_id;        ///< A unique identifier of the remote
   unsigned long                    session_id;           ///< A unique id for the voice session.
   ctrlm_voice_session_end_reason_t reason;               ///< The reason for ending
   unsigned char                    is_voice_assistant;   ///< Boolean indicating if the device is a far-field device (1) as opposed to a hand-held remote (0).
} ctrlm_voice_iarm_event_session_end_t;

typedef struct {
   unsigned char                   api_revision;                                             ///< The revision of this API.
   ctrlm_network_id_t              network_id;                                               ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t            network_type;                                             ///< Type of network on which the controller is bound
   ctrlm_controller_id_t           controller_id;                                            ///< A unique identifier of the remote
   unsigned long                   session_id;                                               ///< A unique id for the voice session.
   ctrlm_voice_session_result_t    result;                                                   ///< The overall result of the voice command
   long                            return_code_http;                                         ///< HTTP request return code
   long                            return_code_curl;                                         ///< Curl's return code
   long                            return_code_vrex;                                         ///< Vrex server's return code
   long                            return_code_internal;                                     ///< Internally generated return code
   char                            vrex_session_id[CTRLM_VOICE_SESSION_ID_MAX_LENGTH];       ///< Unique identifier for the vrex session (null terminated string)
   char                            vrex_session_text[CTRLM_VOICE_SESSION_TEXT_MAX_LENGTH];   ///< Text field returned by the vrex server
   char                            vrex_session_message[CTRLM_VOICE_SESSION_MSG_MAX_LENGTH]; ///< Message field returned by the vrex server
   char                            session_uuid[CTRLM_VOICE_SESSION_ID_MAX_LENGTH];          ///< Local UUID generated for this session
   char                            curl_request_ip[CTRLM_VOICE_REQUEST_IP_MAX_LENGTH];       ///< cURL request primary IP address string
   double                          curl_request_dns_time;                                    ///< cURL request name lookup time (in seconds)
   double                          curl_request_connect_time;                                ///< cURL request connect time (in seconds)
} ctrlm_voice_iarm_event_session_result_t;

typedef struct {
   unsigned char               api_revision;  ///< The revision of this API.
   ctrlm_network_id_t          network_id;    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t        network_type;  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t       controller_id; ///< A unique identifier of the remote
   unsigned long               session_id;    ///< A unique id for the voice session.
   ctrlm_voice_stats_session_t session;       ///< Statistics for the session
   ctrlm_voice_stats_reboot_t  reboot;        ///< Data associated with a reboot
} ctrlm_voice_iarm_event_session_stats_t;

typedef struct {
   unsigned char                      api_revision;  ///< The revision of this API.
   ctrlm_network_id_t                 network_id;    ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t               network_type;  ///< Type of network on which the controller is bound
   ctrlm_controller_id_t              controller_id; ///< A unique identifier of the remote
   unsigned long                      session_id;    ///< A unique id for the voice session.
   ctrlm_voice_session_abort_reason_t reason;        ///< The reason that the voice session was aborted
} ctrlm_voice_iarm_event_session_abort_t;

typedef struct {
   unsigned char                    api_revision;         ///< The revision of this API.
   ctrlm_network_id_t               network_id;           ///< Identifier of network on which the controller is bound
   ctrlm_network_type_t             network_type;         ///< Type of network on which the controller is bound
   ctrlm_controller_id_t            controller_id;        ///< A unique identifier of the remote
   unsigned long                    session_id;           ///< A unique id for the voice session.
   ctrlm_voice_session_end_reason_t reason;               ///< The reason that the voice streaming ended
   long                             return_code_internal; ///< Internally generated return code
} ctrlm_voice_iarm_event_session_short_t;

typedef struct {
   unsigned char api_revision;            ///< The revision of this API
   char          media_service_url[2083]; ///< The url for the media service (null terminated string)
} ctrlm_voice_iarm_event_media_service_t;


// APIs for Thunder Plugin

// IARM Call JSON
// This structure is used for the following calls:
//   CTRLM_VOICE_IARM_CALL_CONFIGURE_VOICE
//   CTRLM_VOICE_IARM_CALL_SET_VOICE_INIT
//   CTRLM_VOICE_IARM_CALL_SEND_VOICE_MESSAGE
//
// The payload MUST be a NULL terminated JSON String.
typedef struct {
   unsigned char  api_revision;
   char           result[CTRLM_VOICE_IARM_CALL_RESULT_LEN_MAX];
   char           payload[];
} ctrlm_voice_iarm_call_json_t;

// IARM Event JSON
// This structure is used for the following calls:
//   CTRLM_VOICE_IARM_EVENT_SESSION_BEGIN_JSON 
//   CTRLM_VOICE_IARM_EVENT_STREAM_BEGIN_JSON  
//   CTRLM_VOICE_IARM_EVENT_SERVER_MESSAGE_JSON
//   CTRLM_VOICE_IARM_EVENT_STREAM_END_JSON    
//   CTRLM_VOICE_IARM_EVENT_SESSION_END_JSON   
//
// The payload MUST be a NULL terminated JSON String.
typedef struct {
   unsigned char  api_revision;
   char           payload[];
} ctrlm_voice_iarm_event_json_t;

// End APIs for Thunder Plugin

/// @}
///
/// @addtogroup CTRLM_IPC_VOICE_CALLS
/// @{
/// @brief Remote Calls accessible via IARM bus
/// @details IARM calls are synchronous calls to functions provided by other IARM bus members. Each bus member can register
/// calls using the IARM_Bus_RegisterCall() function. Other members can invoke the call using the IARM_Bus_Call() function.
///
/// -----------------
/// Call Registration
/// -----------------
///
/// The Voice component of Control Manager registers the following calls.
///
/// | Bus Name                 | Call Name                      | Argument                      | Description                                    |
/// | :-------                 | :--------                      | :-------                      | :----------                                    |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_VOICE_IARM_CALL_SETTINGS | ctrlm_voice_iarm_settings_t * | Sets one or more voice configuration settings. |
///
/// Examples:
///
/// Get a controller's status:
///
///     IARM_Result_t               result;
///     ctrlm_voice_iarm_settings_t settings;
///     settings.available = (CTRLM_VOICE_SETTINGS_VOICE_ENABLED | CTRLM_VOICE_SETTINGS_VREX_SAT_ENABLED);
///     settings.voice_control_enabled = 1;
///     settings.vrex_sat_enabled      = 1;
///
///     result = IARM_Bus_Call(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_CALL_SETTINGS, (void *)&settings, sizeof(settings));
///     if(IARM_RESULT_SUCCESS == result && CTRLM_IARM_CALL_RESULT_SUCCESS == settings.result) {
///         // Settings were successfully updated
///     }
///     }
///
/// @}
///
/// @addtogroup CTRLM_IPC_VOICE_EVENTS
/// @{
/// @brief Broadcast Events accessible via IARM bus
/// @details The IARM bus uses events to broadcast information to interested clients. An event is sent separately to each client. There are no return values for an event and no
/// guarantee that a client will receive the event.  Each event has a different argument structure according to the information being transferred to the clients.  The events that the
/// Remote Control component in Control Manager generates and subscribes to are detailed below.
///
/// ----------------------------
/// Event Generation (Broadcast)
/// ----------------------------
///
/// The Voice component generates events that can be received by other processes connected to the IARM bus. The following events
/// are registered during initialization:
///
/// | Bus Name                 | Event Name                            | Argument                                  | Description                                                      |
/// | :-------                 | :---------                            | :-------                                  | :----------                                                      |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_VOICE_IARM_EVENT_SESSION_BEGIN  | ctrlm_voice_iarm_event_session_begin_t *  | Generated at the beginning of a voice session                    |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_VOICE_IARM_EVENT_SESSION_END    | ctrlm_voice_iarm_event_session_end_t *    | Generated at the end of a voice session                          |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_VOICE_IARM_EVENT_SESSION_RESULT | ctrlm_voice_iarm_event_session_result_t * | Generated when the result of the voice session is available      |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_VOICE_IARM_EVENT_SESSION_STATS  | ctrlm_voice_iarm_event_session_stats_t *  | Generated when the statistics of the voice session are available |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_VOICE_IARM_EVENT_SESSION_ABORT  | ctrlm_voice_iarm_event_session_abort_t *  | Generated when a voice session is aborted (denied)               |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_VOICE_IARM_EVENT_SESSION_SHORT  | ctrlm_voice_iarm_event_session_short_t *  | Generated when a short voice session is detected                 |
/// | CTRLM_MAIN_IARM_BUS_NAME | CTRLM_VOICE_IARM_EVENT_MEDIA_SERVICE  | ctrlm_voice_iarm_event_media_service_t *  | Generated when a media service response is received              |
///
/// IARM events are available on a subscription basis. In order to receive an event, a client must explicitly register to receive the event by calling
/// IARM_Bus_RegisterEventHandler() with the bus name, event name and a @link IARM_EventHandler_t handler function@endlink. Events may be generated at any time by the
/// Voice component. All events are asynchronous.
///
/// Examples:
///
/// Register for a Voice event:
///
///     IARM_Result_t result;
///
///     result = IARM_Bus_RegisterEventHandler(CTRLM_MAIN_IARM_BUS_NAME, CTRLM_VOICE_IARM_EVENT_SESSION_RESULT, session_result_handler_func);
///     if(IARM_RESULT_SUCCESS == result) {
///         // Event registration was set successfully
///     }
///     }
///
/// @}
///
/// @addtogroup CTRLM_IPC_VOICE_COMMS
/// @{
/// @brief Communication Interfaces
/// @details The following diagrams detail the main communication paths for the Voice component.
/// ---------------
/// Voice Data Flow
/// ---------------
///
/// @dot
/// digraph CTRL_MGR_VREX {
///     rankdir=LR;
///     "VREX_IDLE"         [shape="ellipse", fontname=Helvetica, fontsize=10, label="IDLE"];
///     "VREX_OPEN"         [shape="ellipse", fontname=Helvetica, fontsize=10, label="OPEN"];
///     "VREX_CLOSE"        [shape="ellipse", fontname=Helvetica, fontsize=10, label="CLOSE"];
///     "VREX_IS_REM_VALID" [shape="diamond", fontname=Helvetica, fontsize=10, label="Is remote\nvalid?"];
///     "VREX_IS_SVR_AVAIL" [shape="diamond", fontname=Helvetica, fontsize=10, label="Is server\navailable?"];
///     "VREX_IS_SEND_OK"   [shape="diamond", fontname=Helvetica, fontsize=10, label="Is fragment transferred\nto the server?"];
///
///     "VREX_IDLE"         -> "VREX_IS_REM_VALID" [dir="forward", fontname=Helvetica, fontsize=10,label="  Voice Begin Event Rxd"];
///     "VREX_IDLE"         -> "VREX_IDLE"         [dir="forward", fontname=Helvetica, fontsize=10,label="  Voice Fragment/End Event Rxd"];
///     "VREX_IS_REM_VALID" -> "VREX_IS_SVR_AVAIL" [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes"];
///     "VREX_IS_REM_VALID" -> "VREX_IDLE"         [dir="forward", fontname=Helvetica, fontsize=10,label="  No"];
///     "VREX_IS_SVR_AVAIL" -> "VREX_OPEN"         [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes, open session"];
///     "VREX_IS_SVR_AVAIL" -> "VREX_IDLE"         [dir="forward", fontname=Helvetica, fontsize=10,label="  No"];
///     "VREX_OPEN"         -> "VREX_IS_SEND_OK"   [dir="forward", fontname=Helvetica, fontsize=10,label="  Voice Fragment\nEvent Rxd"];
///     "VREX_IS_SEND_OK"   -> "VREX_OPEN"         [dir="forward", fontname=Helvetica, fontsize=10,label="  Yes"];
///     "VREX_IS_SEND_OK"   -> "VREX_CLOSE"        [dir="forward", fontname=Helvetica, fontsize=10,label="  No, end session"];
///     "VREX_OPEN"         -> "VREX_CLOSE"        [dir="forward", fontname=Helvetica, fontsize=10,label="  Voice End Event Rxd.\nTransfer status\ncode to server."];
///     "VREX_CLOSE"        -> "VREX_IDLE"         [dir="forward", fontname=Helvetica, fontsize=10,label="  Close session."];
///     { rank=same; "VREX_IS_REM_VALID"; "VREX_IS_SVR_AVAIL"; }
///     { rank=same; "VREX_OPEN"; "VREX_IS_SEND_OK"; }
/// }
/// \enddot
/// @}
///
/// @}
#endif
