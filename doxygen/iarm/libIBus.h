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

/**
* @file libIBus.h
*
* @brief RDK IARM-Bus API Declarations.
*
* Application should use the APIs declared in this file to access
* services provided by IARM-Bus. Basically services provided by
* these APIs include:
* <br> 1) Library Initialization and termination.
* <br> 2) Connection to IARM-Bus.
* <br> 3) Send and Receive Events.
* <br> 4) Declared and Invoke RPC Methods.
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
* - BE:       ig-Endian.
* - cb:       allback function (suffix).
* - DS:      Device Settings.
* - FPD:     Front-Panel Display.
* - HAL:     Hardware Abstraction Layer.
* - LE:      Little-Endian.
* - LS:      Least Significant.
* - MBZ:     Must be zero.
* - MS:      Most Significant.
* - RDK:     Reference Design Kit.
* - _t:      Type (suffix).
*
* @par Implementation Notes
* -# None
*
*/

/** @defgroup IARM_BUS IARM_BUS
*    @ingroup IARM_BUS
*
*  IARM-Bus is a platform agnostic Inter-process communication (IPC) interface. It allows
*  applications to communicate with each other by sending Events or invoking Remote
*  Procedure Calls. The common programming APIs offered by the RDK IARM-Bus interface is
*  independent of the operating system or the underlying IPC mechanism.
*
*  Two applications connected to the same instance of IARM-Bus are able to exchange events
*  or RPC calls. On a typical system, only one instance of IARM-Bus instance is needed. If
*  desired, it is possible to have multiple IARM-Bus instances. However, applications
*  connected to different buses will not be able to communicate with each other.
*/

/** @addtogroup IARM_BUS_IARM_CORE_API IARM-Core library.
*  @ingroup IARM_BUS
*
*  Described herein are the functions that are part of the
*  IARM Core library.
*
*  @{
*/


#ifndef _LIB_IARM_BUS_H
#define _LIB_IARM_BUS_H


#ifdef __cplusplus
extern "C" 
{
#endif

#include "libIARM.h"
#include "libIBus.h"

#ifdef __cplusplus
extern "C" {
#endif
IARM_Result_t IARM_Bus_DaemonStart(int argc, char *argv[]);
IARM_Result_t IARM_Bus_DaemonStop(void);
#ifdef __cplusplus
}
#endif


/**
 * @brief Function signature for RPC Methods.
 *
 * All IARM RPC Methods must use <tt>IARM_BusCall_t</tt> as their function signature.
 * Important Note: The argument structure cannot have pointers. The sizeof() operator applied
 * on the @p arg must equal to the actual memory allocated.  Internally an equivalent of
 * memcpy() is used to dispatch parameters its target. If a pointer is used in the parameters,
 * the pointer, not the content it points to, is sent to the destination.
 *
 *  @param arg is the composite carrying all input and output parameters of the RPC Method.
 *
 */
typedef IARM_Result_t (*IARM_BusCall_t) (void *arg);
/**
 * @brief Function signature for event handlers.
 *
 * All IARM Event handlers must use <tt>IARM_EventHandler_t</tt> as their function signature.
 * Important Note: The event data structure cannot have pointers. The sizeof() operator applied
 * on the @p data must equal to the actual memory allocated.  Internally an equivalent of
 * memcpy() is used to dispatch event data to its target. If a pointer is used in the event data,
 * the pointer, not the content it points to, is sent to the destination.
 *
 * @param owner is well-known name of the application sending the event.
 * @param eventID is the integer uniquely identifying the event within the sending application.
 * @param data is the composite carrying all input and output parameters of event.
 * @param len is the result of sizeof() applied on the event data data structure.
 * 
 * @return None
 */
typedef void (*IARM_EventHandler_t)(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

/**
 * @brief Initialize the IARM-Bus library.
 *
 * This function initialize the IARM-Bus library and register the application to the bus.
 * The @p name is the <b>well-known</b> name of the application.  Once concatenated with the
 * IARM-Bus name, it becomes an universally unique name for the application.  This name can
 * be thought of as the namespace of the events or RPC methods exported by the application.
 *
 * After the library is initialized, the application is ready to access events and RPC methods
 * on the bus.
 *
 * @param name well-known application name..
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_Bus_Init(const char *name);
/**
 * @brief Terminate the IARM-Bus library.
 *
 * This function releases resources allocated by the IARM Bus Library. After it is called,
 * the library returns to the state prior to <tt>IARM_Bus_Init</tt> is called.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_Bus_Term(void);
/**
 * @brief Connected to the IARM-Bus Daemon.
 *
 * The bus, when connected to the IARM-Bus, is not known to the <b>Bus-Daemon</b> until this API
 * is called.  The <tt>connect</tt> function register the application with the Bus daemon.
 * Bus Daemon only coordinate resource sharing among the applications connected to the bus.
 * Normally this API should be called right after <tt>IARM_Bus_Init()</tt>.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_Bus_Connect(void);
/**
 * @brief Disconnect application from the IARM-Bus Daemon.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_Bus_Disconnect(void);

/**
 * @brief Returns group context of the calling member
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */

IARM_Result_t IARM_Bus_GetContext(void **context);
/**
 * @brief Send an event onto the bus.
 *
 * Event is always broadcast onto the bus. Only listening applications will be awakened.
 * The registered event handlers (via <tt>IARM_Bus_RegisterEventHandler</tt>) are invoked
 * asynchronously.
 *
 * @p ownerName is the well-known name of the application sending the event. It can be thought
 * of as the namespace of the event being sent. @p data holds the event data.  This data structure
 * is allocated internally by IARM and need not be freed by application.  The event data stored
 * at @p data is no longer available after the event handler is returned. <b>Application should make
 * a copy of the event data</b> if it wishes to access the data after event handler is returned.
 *
 * Normally the application use strcmp() to dispatch events based on ownerName and then a
 * switch block to dispatch events based on eventId.
 *
 * @param [in] ownerName is the source of the event.
 * @param [in] eventId is the enum value of the event.
 * @param [in] data is the data structure holding event data.
 * @param [in] len is the sizeof the event data. 
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_Bus_BroadcastEvent(const char *ownerName, IARM_EventId_t eventId, void *data, size_t len);
/**
 * @brief Check if current process is registered with UI Manager.
 *
 * In the event of a crash at UI Manager, the manager may have lost memory of registered process.
 * A member process can call this function regularly to see if it is still registered.
 *
 * @param [in] memberName 
 * @param [out] isRegistered True if still registered.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_Bus_IsConnected(const char *memberName, int *isRegistered);
/**
 * @brief Register Event Handler to handle events.
 *
 * Each event can only be associated with 1 event handler. If an event handler is already registered, the old one
 * will be replaced with the new one.
 *
 * @param [in] ownerName well-known name of the application sending the event.
 * @param [in] eventId is the enum value of the event.
 * @param [in] handler callback function to process the event.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 *
 * @see IARM_Bus_BroadcastEvent
 * @see IARM_EventHandler_t
 */
IARM_Result_t IARM_Bus_RegisterEventHandler(const char *ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler);

/**
 * @brief Unregister to remove from listeners of the event.
 *
 * This API remove the process from the listeners of the event.
 
 * @param [in] ownerName well-known name of the application sending the event.
 * @param [in] eventId The event whose listener to remove.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 */
IARM_Result_t IARM_Bus_UnRegisterEventHandler(const char *ownerName, IARM_EventId_t eventId);
/**
 * @brief Register an RPC Method.
 *
 * Register an RPC method that can be invoked by other applications.  The @p methodName is the string name
 * used to invoke the RPC method. @p handler is the implementation of the RPC method. When other application
 * invoke the method via its string name, the function pointed to by the handler is executed.
 *
 * @param [in] methodName well-known name of the RPC Method. This name lives within the name sapce of the application name.
 * @param [in] handler implementation of the RPC method..
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 *
 * @see IARM_BusCall_t
 */
IARM_Result_t IARM_Bus_RegisterCall(const char *methodName, IARM_BusCall_t handler);
/**
 * @brief Invoke an RPC Method.
 *
 * Invoke RPC method by its application name and method name.  The @p arg is data structure holding all input
 * and output parameters of the invocation. @p argLen is result of sizeof() applied on the @p arg data structure.
 *
 * @param [in] ownerName well-known name of the application sending the event.
 * @param [in] methodName well-known name of the RPC Method. 
 * @param [in] arg is data structure holding all input and output parameters of the invocation.
 * @param [in] argLen is the sizeof arg.
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 *
 * @see IARM_BusCall_t
 */
IARM_Result_t IARM_Bus_Call(const char *ownerName,  const char *methodName, void *arg, size_t argLen);
/**
 * @brief Register Events.
 *
 * Register all the events that are published by the application.  An application can publish multipel events.
 * These events must each have an enumeration value starting 0. The @p maxEventId is the enum value of the last
 * event + 1.  This API register all events whose enum value is less than @p maxEventId at once.
 *
 * Once an event is registered, it can be broadcast on to the bus and received by listening applications.
 *
 * @param [in] maxEventId is the enum value of the last event + 1
 *
 * @return IARM_Result_t Error Code.
 * @retval IARM_RESULT_SUCCESS on success
 *
 * @see IARM_Bus_BroadcastEvent
 */
IARM_Result_t IARM_Bus_RegisterEvent(IARM_EventId_t maxEventId);

#ifdef __cplusplus
}
#endif
#endif

/* End of IARM_BUS_IARM_CORE_API doxygen group */
/**
 * @}
 */

