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

/// @mainpage Control Manager HAL Interface
/// This document describes the interfaces that Control Manager uses to communicate with networks in the system.  Control Manager supports one or
/// more network devices via a Hardware Abstraction Layer (HAL).  The HAL API specifies the method by which a network device communications with the Control Manager.
///
/// This manual is divided into the following sections:
/// - @subpage CTRL_MGR_HAL
/// - @subpage CTRL_MGR_HAL_RF4CE
///

/// @defgroup CTRL_MGR_HAL Control Manager HAL API (common)
/// @{
///
/// @page HAL_Intro Introduction
/// The Control Manager provides a hardware abstraction layer (HAL) to allow it to support multiple network types.  The communication between
/// Control Manager and a HAL network is done exclusively via the HAL API.  The communication paths are summarized in the following high level block diagram.
///
/// @dot
/// digraph CTRL_MGR_COMMS_HAL {
///     "CTRL_MGR"     [shape="ellipse", fontname=Helvetica, fontsize=10, label="Control Manager"];
///     "CTRL_MGR_HAL" [shape="rectangle", fontname=Helvetica, fontsize=10, label="Control Manager HAL"];
///     "RF4CE_DEV"    [shape="ellipse", fontname=Helvetica, fontsize=10, label="RF4CE Network"];
///     "BT_LE_DEV"    [shape="ellipse", fontname=Helvetica, fontsize=10, label="BT LE Network"];
///     "IP_DEV"       [shape="ellipse", fontname=Helvetica, fontsize=10, label="IP Network"];
///     "OTHER_DEV"    [shape="ellipse", fontname=Helvetica, fontsize=10, label="Other Network"];
///
///     "CTRL_MGR"     -> "CTRL_MGR_HAL" [dir="both", fontname=Helvetica, fontsize=10,label="  "];
///     "CTRL_MGR_HAL" -> "RF4CE_DEV"    [dir="both", fontname=Helvetica, fontsize=10,label="  "];
///     "CTRL_MGR_HAL" -> "BT_LE_DEV"    [dir="both", fontname=Helvetica, fontsize=10,label="  "];
///     "CTRL_MGR_HAL" -> "IP_DEV"       [dir="both", fontname=Helvetica, fontsize=10,label="  "];
///     "CTRL_MGR_HAL" -> "OTHER_DEV"    [dir="both", fontname=Helvetica, fontsize=10,label="  "];
/// }
/// \enddot
///
///
/// Initialization
/// --------------
///
/// When the Control Manager is started, a thread will be launched to initialize each supported HAL network.  The HAL network device
/// should initialize it's network stack.  After initialization is complete, it must call control_hal_<net>_cfm_init to inform Control Manager of the results.
/// If an error occurs, the version string must still be returned for diagnostic purposes.  After a successful initialization, the HAL network must be
/// ready for immediate use by the Control Manager.
///
/// Termination
/// -----------
///
/// When the Control Manager is terminating, it must terminate all HAL networks that have been successfully initialized.  To terminate a
/// HAL network instance, the Control Manager will call the network's terminate function which must release all resources allocated by
/// the HAL network.
///
/// @}
