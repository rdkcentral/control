/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2017-2020 Sky UK
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

/*
 * Changes made by Comcast
 * Copyright 2024 Comcast Cable Communications Management, LLC
 * Licensed under the Apache License, Version 2.0
 */

//
//  hcisocket_p.h
//

#ifndef HCISOCKET_P_H
#define HCISOCKET_P_H

#include "hcisocket.h"

#include "ctrlm_utils.h"
#include <semaphore.h>


struct evt_le_connection_complete;
struct evt_disconn_complete;
struct evt_le_connection_update_complete;


class HciSocketImpl : public HciSocket
{
public:
    HciSocketImpl();
    HciSocketImpl(uint hciDeviceId, int netNsFd);
    ~HciSocketImpl();

public:
    bool isValid() const override;

    bool requestConnectionUpdate(uint16_t connHandle,
                                 const BleConnectionParameters &params) override;

    std::vector<ConnectedDeviceInfo> getConnectedDevices() const override;

    bool sendIncreaseDataCapability(uint16_t connHandle) override;
    
    void onSocketActivated();

private:
    bool setSocketFilter(int socketFd) const;

    bool bindSocket(int socketFd, uint hciDeviceId) const;

    bool sendCommand(uint16_t ogf, uint16_t ocf, void *data, uint8_t dataLen);

    bool checkConnectionParams(uint16_t minInterval, uint16_t maxInterval,
                               uint16_t latency, uint16_t supervisionTimeout) const;

    void onConnectionCompleteEvent(const evt_le_connection_complete *event);
    void onDisconnectionCompleteEvent(const evt_disconn_complete *event);
    void onUpdateCompleteEvent(const evt_le_connection_update_complete *event);

    const char* hciErrorString(uint8_t code) const;

public:
    std::shared_ptr<bool> m_isAlive;
    int m_hciSocket;
    sem_t m_socketThreadSem;
    int m_exitEventFds[2] = {-1,-1};

private:
    const uint m_hciDeviceId;
    ctrlm_thread_t m_socketThread;

};


#endif // !defined(HCISOCKET_P_H)
