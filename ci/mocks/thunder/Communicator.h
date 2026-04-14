/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 RDK Management
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

#ifndef __COM_PROCESSLAUNCH_H
#define __COM_PROCESSLAUNCH_H

#include <string>
#include "Proxy.h"
#include "NodeId.h"
#include "IPCConnector.h"

using std::string;
const uint32_t CommunicationTimeOut = 3000;

class ICommunicatorClient {
public:
    virtual ~ICommunicatorClient() = default;
    virtual uint32_t Open(const uint32_t waitTime) = 0;
    virtual void* Open(const std::string& className, const uint32_t version, const uint32_t waitTime) = 0;
    virtual void* Acquire(const uint32_t waitTime, const std::string& className, const uint32_t versionId) = 0;
    virtual uint32_t Offer(void* offer, const uint32_t version, const uint32_t waitTime) = 0;
    virtual uint32_t Revoke(void* offer, const uint32_t version, const uint32_t waitTime) = 0;
    virtual uint32_t Close(const uint32_t waitTime) = 0;
	virtual void* Acquire(const std::string& className, const uint32_t interfaceId, const uint32_t versionId) = 0;
};

namespace WPEFramework {
namespace RPC {
    class Object {
    public:
        enum class HostType {
            LOCAL,
            DISTRIBUTED,
            CONTAINER
        };

        Object()
            : _locator()
            , _className()
            , _callsign()
            , _interface(~0)
            , _version(~0)
            , _user()
            , _group()
            , _threads()
            , _priority()
            , _type(HostType::LOCAL)
            , _systemRootPath()
            , _remoteAddress()
            , _configuration()
        {
        }
        Object(const Object& copy)
            : _locator(copy._locator)
            , _className(copy._className)
            , _callsign(copy._callsign)
            , _interface(copy._interface)
            , _version(copy._version)
            , _user(copy._user)
            , _group(copy._group)
            , _threads(copy._threads)
            , _priority(copy._priority)
            , _type(copy._type)
            , _systemRootPath(copy._systemRootPath)
            , _remoteAddress(copy._remoteAddress)
            , _configuration(copy._configuration)
        {
        }
        Object(const string& locator,
            const string& className,
            const string& callsign,
            const uint32_t interface,
            const uint32_t version,
            const string& user,
            const string& group,
            const uint8_t threads,
            const int8_t priority,
            const HostType type,
            const string& systemRootPath,
            const string& remoteAddress,
            const string& configuration)
            : _locator(locator)
            , _className(className)
            , _callsign(callsign)
            , _interface(interface)
            , _version(version)
            , _user(user)
            , _group(group)
            , _threads(threads)
            , _priority(priority)
            , _type(type)
            , _systemRootPath(systemRootPath)
            , _remoteAddress(remoteAddress)
            , _configuration(configuration)
        {
        }
        ~Object()
        {
        }

        Object& operator=(const Object& RHS)
        {
            _locator = RHS._locator;
            _className = RHS._className;
            _callsign = RHS._callsign;
            _interface = RHS._interface;
            _version = RHS._version;
            _user = RHS._user;
            _group = RHS._group;
            _threads = RHS._threads;
            _priority = RHS._priority;
            _systemRootPath = RHS._systemRootPath;
            _type = RHS._type;
            _remoteAddress = RHS._remoteAddress;
            _configuration = RHS._configuration;

            return (*this);
        }

    public:
        inline const string& Locator() const
        {
            return (_locator);
        }
        inline const string& ClassName() const
        {
            return (_className);
        }
        inline const string& Callsign() const
        {
            return (_callsign);
        }
        inline uint32_t Interface() const
        {
            return (_interface);
        }
        inline uint32_t Version() const
        {
            return (_version);
        }
        inline const string& User() const
        {
            return (_user);
        }
        inline const string& Group() const
        {
            return (_group);
        }
        inline uint8_t Threads() const
        {
            return (_threads);
        }
        inline int8_t Priority() const
        {
            return (_priority);
        }
        inline HostType Type() const
        {
            return (_type);
        }
        inline const string& SystemRootPath() const
        {
            return (_systemRootPath);
        }
        inline const Core::NodeId RemoteAddress() const
        {
            return (Core::NodeId(_remoteAddress.c_str()));
        }
        inline const string& Configuration() const
        {
            return (_configuration);
        }

    private:
        string _locator;
        string _className;
        string _callsign;
        uint32_t _interface;
        uint32_t _version;
        string _user;
        string _group;
        uint8_t _threads;
        int8_t _priority;
        HostType _type;
        string _systemRootPath;
        string _remoteAddress;
        string _configuration;
    };

    class CommunicatorClient{
    protected:
        static ICommunicatorClient* impl;

    public:
		class Source {
               bool IsOpen() const
               {
                  return true;
               }
		};
        CommunicatorClient() = delete;
        CommunicatorClient(CommunicatorClient&&) = delete;
        CommunicatorClient(const CommunicatorClient&) = delete;
        CommunicatorClient& operator=(CommunicatorClient&&) = delete;
        CommunicatorClient& operator=(const CommunicatorClient&) = delete;

        static void setImpl(ICommunicatorClient* newImpl);

        CommunicatorClient(const Core::NodeId& remoteNode) {
            //Mock CommunicatorClient constructor.
        }

        CommunicatorClient(const Core::NodeId& remoteNode, const Core::ProxyType<Core::IIPCServer>& handler) {
            //Mock CommunicatorClient constructor.
        }

        ~CommunicatorClient() {
        }

    public:
        inline uint32_t ConnectionId() const {
            return _connectionId;
        }

        // Open a communication channel with this process, no need for an initial exchange
        uint32_t Open(const uint32_t waitTime)
        {
            return impl->Open(waitTime);
        }
        template <typename INTERFACE>
        INTERFACE* Open(const std::string& className, const uint32_t version = static_cast<uint32_t>(~0), const uint32_t waitTime = CommunicationTimeOut)
        {
            return static_cast<INTERFACE*>(impl->Open(className, version, waitTime));
        }
        template <typename INTERFACE>
        INTERFACE* Acquire(const uint32_t waitTime, const std::string& className, const uint32_t versionId)
        {
            return static_cast<INTERFACE*>(impl->Acquire(waitTime, className, versionId));
        }
        template <typename INTERFACE>
        uint32_t Offer(INTERFACE* offer, const uint32_t version = static_cast<uint32_t>(~0), const uint32_t waitTime = CommunicationTimeOut)
        {
            return static_cast<INTERFACE*>(impl->Offer(static_cast<void*>(offer), version, CommunicationTimeOut));
        }
        template <typename INTERFACE>
        uint32_t Revoke(INTERFACE* offer, const uint32_t version = static_cast<uint32_t>(~0), const uint32_t waitTime = CommunicationTimeOut)
        {
            return static_cast<INTERFACE*>(impl->Revoke(static_cast<void*>(offer), version, CommunicationTimeOut));
        }
        uint32_t Close(const uint32_t waitTime)
        {
            if (impl)
                return impl->Close(waitTime);
            return Core::ERROR_GENERAL;
        }
        virtual void* Acquire(const std::string& className, const uint32_t interfaceId, const uint32_t versionId)
        {
            return impl->Acquire(className, interfaceId, versionId);
        }
    protected:
        void StateChange() {
            //Mock CommunicatorClient StateChange is calling.
			}
    private:
        uint32_t _connectionId;
    };
}
}
#endif // __COM_PROCESSLAUNCH_H

