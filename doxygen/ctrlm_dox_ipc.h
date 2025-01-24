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

/// @mainpage Control Manager IARM Interfaces
/// This document describes the interfaces that Control Manager uses to communicate with other components in the RDK.
///
/// This manual is divided into the following sections:
/// - @subpage IARM_BUS_IARM_CORE_API
/// - @subpage CTRLM_IPC
/// - @subpage CTRLM_IPC_MAIN
/// - @subpage CTRLM_IPC_RCU
/// - @subpage CTRLM_IPC_VOICE
/// - @subpage CTRLM_IPC_DEVICE_UPDATE
///
/// @addtogroup CTRLM_IPC Communication Interfaces
/// @{
/// @brief High level description with diagrams
/// 
/// @page CTRL_MGR_OPERATION Control Manager Operation
/// This document describes the operation of the Control Manager.
/// 
/// The Control Manager communicates with other processes in the system using the IARM bus. The communication paths to and from the 
/// Control Manager is summarized in the following diagram.
/// 
/// @dot
/// digraph CTRL_MGR_COMMS {
///     "IR_MGR"       [shape="ellipse", fontname=Helvetica, fontsize=10, label="IR Manager"];
///     "CTRL_MGR"     [shape="ellipse", fontname=Helvetica, fontsize=10, label="Control Manager"];
///     "IARM_BUS"     [shape="rectangle", fontname=Helvetica, fontsize=10, label="          IARM Bus          "];
///     "SVC_MGR"      [shape="ellipse", fontname=Helvetica, fontsize=10, label="Service Manager"];
///     "CTRL_MGR_HAL" [shape="rectangle", fontname=Helvetica, fontsize=10, label="Control Manager HAL"];
///     "RF4CE_DEV"    [shape="ellipse", fontname=Helvetica, fontsize=10, label="RF4CE Network"];
///     "BT_LE_DEV"    [shape="ellipse", fontname=Helvetica, fontsize=10, label="BT LE Network"];
///     "IP_DEV"       [shape="ellipse", fontname=Helvetica, fontsize=10, label="IP Network"];
///     "OTHER_DEV"    [shape="ellipse", fontname=Helvetica, fontsize=10, label="Other Network"];
///
///     "SVC_MGR"      -> "IARM_BUS"     [dir="both", fontname=Helvetica, fontsize=10,label="  Keys, Voice, Update"];
///     "IR_MGR"       -> "IARM_BUS"     [dir="both", fontname=Helvetica, fontsize=10,label="  Keys"];
///     "IARM_BUS"     -> "CTRL_MGR"     [dir="both", fontname=Helvetica, fontsize=10,label="  Keys, Voice, Update"];
///     "CTRL_MGR"     -> "CTRL_MGR_HAL" [dir="both", fontname=Helvetica, fontsize=10,label="  "];
///     "CTRL_MGR_HAL" -> "RF4CE_DEV"    [dir="both", fontname=Helvetica, fontsize=10,label="  "];
///     "CTRL_MGR_HAL" -> "BT_LE_DEV"    [dir="both", fontname=Helvetica, fontsize=10,label="  "];
///     "CTRL_MGR_HAL" -> "IP_DEV"       [dir="both", fontname=Helvetica, fontsize=10,label="  "];
///     "CTRL_MGR_HAL" -> "OTHER_DEV"    [dir="both", fontname=Helvetica, fontsize=10,label="  "];
/// }
/// \enddot
/// 
/// @}
