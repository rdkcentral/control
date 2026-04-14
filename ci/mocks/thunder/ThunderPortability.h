/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
**/

#pragma once

#include <core/core.h>
#include <plugins/plugins.h>

#ifdef USE_THUNDER_R4
#define EVENT_SUBSCRIBE(__A__, __B__, __C__, __D__) \
    { \
        auto ret = plugin->Subscribe(__A__, __B__, __C__); \
        EXPECT_EQ(Core::ERROR_NONE, ret); \
        (void)__D__; \
    }

#define EVENT_UNSUBSCRIBE(__A__, __B__, __C__, __D__) \
    { \
        auto ret = plugin->Unsubscribe(__A__, __B__, __C__); \
        EXPECT_EQ(Core::ERROR_NONE, ret); \
        (void)__D__; \
    }

#define DECL_CORE_JSONRPC_CONX                        Core::JSONRPC::Context
#define INIT_CONX(__X__, __Y__)                       connection(__X__, __Y__, "")
#define PLUGINHOST_DISPATCHER                         PluginHost::ILocalDispatcher
#define PLUGINHOST_DISPATCHER_ID                      PluginHost::ILocalDispatcher::ID
#else
#define EVENT_SUBSCRIBE(__A__, __B__, __C__, __D__)   { handler.Subscribe(__A__, __B__, __C__, __D__); }
#define EVENT_UNSUBSCRIBE(__A__, __B__, __C__, __D__) { handler.Unsubscribe(__A__, __B__, __C__, __D__); }

#define DECL_CORE_JSONRPC_CONX                        Core::JSONRPC::Connection
#define INIT_CONX(__X__, __Y__)                       connection(__X__, __Y__)
#define PLUGINHOST_DISPATCHER                         PluginHost::IDispatcher
#define PLUGINHOST_DISPATCHER_ID                      PluginHost::IDispatcher::ID
#endif
