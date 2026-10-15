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
#define CTRLM_VOICE_SERVER_URL_MAX_LENGTH          (2048) ///< Server url string maximum length
#define CTRLM_VOICE_GUIDE_LANGUAGE_MAX_LENGTH        (16) ///< Guide language string maximum length
#define CTRLM_VOICE_ASPECT_RATIO_MAX_LENGTH          (16) ///< Aspect ratio string maximum length
#define CTRLM_VOICE_SESSION_TEXT_MAX_LENGTH         (512) ///< Session text string maximum length
#define CTRLM_VOICE_QUERY_STRING_MAX_LENGTH         (128) ///< Query string maximum name or value length
#define CTRLM_VOICE_QUERY_STRING_MAX_PAIRS           (24) ///< Query string maximum number of name/value pairs

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

typedef enum {
   CTRLM_VOICE_SESSION_ABORT_REASON_BUSY                  =  0, ///< Session aborted because another session in progress
   CTRLM_VOICE_SESSION_ABORT_REASON_SERVER_NOT_READY      =  1, ///< Session aborted because the server cannot be reached
   CTRLM_VOICE_SESSION_ABORT_REASON_AUDIO_FORMAT          =  2, ///< Session aborted due to failure to negotiate an audio format
   CTRLM_VOICE_SESSION_ABORT_REASON_FAILURE               =  3, ///< Session aborted for any other reason
   CTRLM_VOICE_SESSION_ABORT_REASON_VOICE_DISABLED        =  4, ///< Session aborted because the voice feature is disabled
   CTRLM_VOICE_SESSION_ABORT_REASON_DEVICE_UPDATE         =  5, ///< Session aborted due to device update in progress
   CTRLM_VOICE_SESSION_ABORT_REASON_NO_AUTH_DATA          =  6, ///< Session aborted due to missing authorization data
   CTRLM_VOICE_SESSION_ABORT_REASON_NEW_SESSION           =  7, ///< Session aborted because the remote's previous session is still active
   CTRLM_VOICE_SESSION_ABORT_REASON_INVALID_CONTROLLER_ID =  8, ///< Session aborted because the controller id isn't valid
   CTRLM_VOICE_SESSION_ABORT_REASON_APPLICATION_RESTART   =  9, ///< Session aborted due to restarting controlMgr.
   CTRLM_VOICE_SESSION_ABORT_REASON_MAX                   = 10  ///< Session Abort Reason maximum value
} ctrlm_voice_session_abort_reason_t;

typedef enum {
   CTRLM_VOICE_RESET_TYPE_POWER_ON                   = 0, ///< Normal power up by inserting batteries
   CTRLM_VOICE_RESET_TYPE_EXTERNAL                   = 1, ///< Reset due to an external condition
   CTRLM_VOICE_RESET_TYPE_WATCHDOG                   = 2, ///< Reset due to watchdog timer expiration
   CTRLM_VOICE_RESET_TYPE_CLOCK_LOSS                 = 3, ///< Reset due to loss of main clock
   CTRLM_VOICE_RESET_TYPE_BROWN_OUT                  = 4, ///< Reset due to a low voltage condition
   CTRLM_VOICE_RESET_TYPE_OTHER                      = 5, ///< Reset due to any other reason
   CTRLM_VOICE_RESET_TYPE_MAX                        = 6  ///< Reset type maximum value
} ctrlm_voice_reset_type_t;

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
   signed long   voice_key_held_ms;///< The amount of time (in milliseconds) that the voice key was held down
} ctrlm_voice_stats_session_t;

typedef struct {
   unsigned char            available;             ///< Boolean value indicating that a remote control reset was detected (1) or not (0)
   ctrlm_voice_reset_type_t reset_type;            ///< The type of reset that occurred
   unsigned char            voltage;               ///< RCU's voltage from 0.0 V (0) to 4.0 V (0xFF).  The value 0xFF indicates that the voltage is not available.
   unsigned char            battery_percentage;    ///< RCU's battery percentage from 0-100.
} ctrlm_voice_stats_reboot_t;


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

#endif
