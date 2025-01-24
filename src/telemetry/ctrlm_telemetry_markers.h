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
#ifndef __CTRLM_TELEMETRY_MARKERS_H__
#define __CTRLM_TELEMETRY_MARKERS_H__

// This file contains all of the constant strings used for the controlMgr telemetry markers.
// Any time a new marker is added to the code base, it needs to be defined here. Additional
// comments must be added to describe format of the marker.

//
// Global Markers
//

//
// End Global Markers
//

//
// Voice Markers
//

#define MARKER_VOICE_SESSION_TOTAL   "ctrlm.voice.session.total"   // Count of total voice sessions
#define MARKER_VOICE_SESSION_SUCCESS "ctrlm.voice.session.success" // Count of successful voice sessions
#define MARKER_VOICE_SESSION_FAILURE "ctrlm.voice.session.fail"    // Count of failed voice sessions

// Voice Session End Reasons
// The Voice Session End Reason Marker names are generated at runtime. The name is the concatenation of the PREFIX and ERR_STR.
#define MARKER_VOICE_END_REASON_PREFIX "ctrlm.voice.end_reason."

// XRSR End Reason
// The XR-SPEECH-ROUTER End Reason Marker names are generated at runtime. The name is the concatenation of the PREFIX and ERR_STR
#define MARKER_VOICE_XRSR_END_REASON_PREFIX "ctrlm.voice.xrsr_end_reason."

// Voice Session Response Errors

// The Voice Session Response Error Marker names are generated at runtime. The name is the concatenation
// of the PREFIX, ERR_STR, and sub-field. This was designed this way so that all HAL error strings are reported to 
// telemetry seperately for tracking purposes. Also defined is the TOTAL string, which will be a collection
// of stats for all of the VSR fails.
//
// The three markers of interest as of 09/12/2022 are the following:
// - ctrlm.voice.vsrsperr.NO_ACK.*
// - ctrlm.voice.vsrsperr.NO_RESPONSE.*
// - ctrlm.voice.vsrsperr.TOTAL.*
//
// The format of each of these markers is a comma seperated string in the format below:
// <total>,<num errs w/ voice>,<num errs w/o voice>,<num errs in window>,<num errs out window>,<num errs 0 rsp>,<rsp_window>,<min rsp>,<max rsp>,<avg rsp>
//
// <total>               - Count of VSR errors
// <num errs w/ voice>   - Count of VSR errors in which audio data was received from the remote.
// <num errs w/o voice>  - Count of VSR errors in which audio data was not received from the remote.
// <num errs in window>  - Count of VSR errors in which the response was attempted to be transmitted within the window.
// <num errs out window> - Count of VSR errors in which the response was attempted to be transmitted outside the window.
// <num errs 0 rsp>      - Count of VSR errors in which the response was attempted to be transmitted immediately (0ms).
// <rsp_window>          - The current Voice Session Response window size in milliseconds(ms).
// <min rsp>             - The minimum amount of time in which the response was attempted to be transmitted and a VSR error occurred.
// <max rsp>             - The maximum amount of time in which the response was attempted to be transmitted and a VSR error occurred.
// <avg rsp>             - The average amount of time in which the response was attempted to be transmitted and a VSR error occurred.
#define MARKER_VOICE_VSR_FAIL_PREFIX            "ctrlm.voice.vsrsperr."
#define MARKER_VOICE_VSR_FAIL_TOTAL             "TOTAL"
#define MARKER_VOICE_VSR_FAIL_SUB_TOTAL         ".total"        // <total> 
#define MARKER_VOICE_VSR_FAIL_SUB_W_VOICE       ".w_voice"      // <num errs w/ voice>
#define MARKER_VOICE_VSR_FAIL_SUB_WO_VOICE      ".wo_voice"     // <num errs w/o voice>
#define MARKER_VOICE_VSR_FAIL_SUB_IN_WIN        ".in_win"       // <num errs in window>
#define MARKER_VOICE_VSR_FAIL_SUB_OUT_WIN       ".out_win"      // <num errs out window>
#define MARKER_VOICE_VSR_FAIL_SUB_RSPTIME_ZERO  ".rsptime_0"    // <num errs 0 rsp> 
#define MARKER_VOICE_VSR_FAIL_SUB_RSP_WINDOW    ".rsp_window"   // <rsp_window> 
#define MARKER_VOICE_VSR_FAIL_SUB_MIN_RSPTIME   ".rsptime_min"  // <min rsp>
#define MARKER_VOICE_VSR_FAIL_SUB_MAX_RSPTIME   ".rsptime_max"  // <max rsp>
#define MARKER_VOICE_VSR_FAIL_SUB_AVG_RSPTIME   ".rsptime_avg"  // <avg rsp>

// End Voice Session Response Errors

//
// End Voice Markers
//

#endif