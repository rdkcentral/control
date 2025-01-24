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
//  blercudevice.h
//

#ifndef BLERCUDEVICE_H
#define BLERCUDEVICE_H


#include "utils/bleaddress.h"
#include "utils/slot.h"
#include "utils/pendingreply.h"
#include "utils/audioformat.h"

#include <string>
#include <memory>
#include <map>
#include "ctrlm_hal.h"

class BleRcuAudioService;
class BleRcuBatteryService;
class BleRcuDeviceInfoService;
class BleRcuFindMeService;
class BleRcuInfraredService;
class BleRcuUpgradeService;
class BleRcuRemoteControlService;




class BleRcuDevice
{

protected:
    BleRcuDevice() = default;

public:
    virtual ~BleRcuDevice() = default;

public:
    enum Encoding {
        PCM16,
        ADPCM_FRAME,
        InvalidEncoding
    };

public:
    virtual bool isValid() const = 0;
    virtual bool isConnected() const = 0;
    virtual bool isPairing() const = 0;
    virtual bool isPaired() const = 0;
    virtual bool isReady() const = 0;

    virtual double msecsSinceReady() const = 0;
    virtual void shutdown() = 0;

    virtual BleAddress address() const = 0;
    virtual std::string name() const = 0;

    // virtual Future<qint16> rssi() const = 0;

    virtual int deviceId() const
    {
        return -1;
    }
    

public:
    virtual std::shared_ptr<BleRcuAudioService> audioService() const = 0;
    virtual std::shared_ptr<BleRcuBatteryService> batteryService() const = 0;
    virtual std::shared_ptr<BleRcuDeviceInfoService> deviceInfoService() const = 0;
    virtual std::shared_ptr<BleRcuFindMeService> findMeService() const = 0;
    virtual std::shared_ptr<BleRcuInfraredService> infraredService() const = 0;
    virtual std::shared_ptr<BleRcuUpgradeService> upgradeService() const = 0;
    virtual std::shared_ptr<BleRcuRemoteControlService> remoteControlService() const = 0;

    // Device Info Service
    virtual std::string firmwareRevision() const = 0;
    virtual std::string hardwareRevision() const = 0;
    virtual std::string softwareRevision() const = 0;
    virtual std::string manufacturer() const = 0;
    virtual std::string model() const = 0;
    virtual std::string serialNumber() const = 0;
    virtual void addManufacturerNameChangedSlot(const Slot<const std::string &> &func) = 0;
    virtual void addModelNumberChangedSlot(const Slot<const std::string &> &func) = 0;
    virtual void addSerialNumberChangedSlot(const Slot<const std::string &> &func) = 0;
    virtual void addHardwareRevisionChangedSlot(const Slot<const std::string &> &func) = 0;
    virtual void addFirmwareVersionChangedSlot(const Slot<const std::string &> &func) = 0;
    virtual void addSoftwareVersionChangedSlot(const Slot<const std::string &> &func) = 0;
    
    // Battery Service
    virtual uint8_t batteryLevel() const = 0;
    virtual void addBatteryLevelChangedSlot(const Slot<int> &func) = 0;

    // FindMe Service
    virtual void findMe(uint8_t level, PendingReply<> &&reply) const = 0;
    
    // Remote Control Service
    virtual uint8_t unpairReason() const = 0;
    virtual uint8_t rebootReason() const = 0;
    virtual uint8_t lastKeypress() const = 0;
    virtual uint8_t advConfig() const = 0;
    virtual std::vector<uint8_t> advConfigCustomList() const = 0;
    virtual void sendRcuAction(uint8_t action, PendingReply<> &&reply) = 0;
    virtual void writeAdvertisingConfig(uint8_t config, const std::vector<uint8_t> &customList, PendingReply<> &&reply) = 0;
    virtual void addUnpairReasonChangedSlot(const Slot<uint8_t> &func) = 0;
    virtual void addRebootReasonChangedSlot(const Slot<uint8_t, std::string> &func) = 0;
    virtual void addLastKeypressChangedSlot(const Slot<uint8_t> &func) = 0;
    virtual void addAdvConfigChangedSlot(const Slot<uint8_t> &func) = 0;
    virtual void addAdvConfigCustomListChangedSlot(const Slot<const std::vector<uint8_t> &> &func) = 0;

    // Voice Service
    virtual uint8_t audioGainLevel() const = 0;
    virtual void setAudioGainLevel(uint8_t value) = 0;
    virtual uint32_t audioCodecs() const = 0;
    virtual bool audioStreaming() const = 0;

    virtual bool getAudioFormat(Encoding encoding, AudioFormat &format) const = 0;
    virtual void startAudioStreaming(uint32_t encoding, PendingReply<int> &&reply, uint32_t durationMax = 0) = 0;
    virtual void stopAudioStreaming(uint32_t audioDuration, PendingReply<> &&reply) = 0;
    virtual void getAudioStatus(uint32_t &lastError, uint32_t &expectedPackets, uint32_t &actualPackets) = 0;
    virtual bool getFirstAudioDataTime(ctrlm_timestamp_t &time) = 0;

    virtual void addStreamingChangedSlot(const Slot<bool> &func) = 0;
    virtual void addGainLevelChangedSlot(const Slot<uint8_t> &func) = 0;
    virtual void addAudioCodecsChangedSlot(const Slot<uint32_t> &func) = 0;

    // IR Service
    virtual int32_t irCode() const = 0;
    virtual uint8_t irSupport() const = 0;
    virtual void setIrControl(const uint8_t irControl, PendingReply<> &&reply) = 0;
    virtual void programIrSignalWaveforms(const std::map<uint32_t, std::vector<uint8_t>> &irWaveforms,
                                  const uint8_t irControl, PendingReply<> &&reply) = 0;
    virtual void eraseIrSignals(PendingReply<> &&reply) = 0;
    virtual void emitIrSignal(uint32_t keyCode, PendingReply<> &&reply) = 0;
    virtual void addCodeIdChangedSlot(const Slot<int32_t> &func) = 0;
    virtual void addIrSupportChangedSlot(const Slot<uint8_t> &func) = 0;

    // Upgrade Service
    virtual void startUpgrade(const std::string &fwFile, PendingReply<> &&reply) = 0;
    virtual void cancelUpgrade(PendingReply<> &&reply) = 0;
    virtual void addUpgradingChangedSlot(const Slot<bool> &func) = 0;
    virtual void addUpgradeProgressChangedSlot(const Slot<int> &func) = 0;
    virtual void addUpgradeErrorSlot(const Slot<std::string> &func) = 0;

public:
    template<typename T>
    std::shared_ptr<T> service() const;

    inline void addConnectedChangedSlot(const Slot<bool> &func)
    {
        m_connectedChangedSlots.addSlot(func);
    }
    inline void addPairedChangedSlot(const Slot<bool> &func)
    {
        m_pairedChangedSlots.addSlot(func);
    }
    inline void addNameChangedSlot(const Slot<const std::string&> &func)
    {
        m_nameChangedSlots.addSlot(func);
    }
    inline void addReadyChangedSlot(const Slot<bool> &func)
    {
        m_readyChangedSlots.addSlot(func);
    }

protected:
    Slots<bool>                  m_connectedChangedSlots;
    Slots<bool>                  m_pairedChangedSlots;
    Slots<const std::string&>    m_nameChangedSlots;
    Slots<bool>                  m_readyChangedSlots;
};


template<>
inline std::shared_ptr<BleRcuAudioService> BleRcuDevice::service() const
{ return audioService(); }

template<>
inline std::shared_ptr<BleRcuBatteryService> BleRcuDevice::service() const
{ return batteryService(); }

template<>
inline std::shared_ptr<BleRcuDeviceInfoService> BleRcuDevice::service() const
{ return deviceInfoService(); }

template<>
inline std::shared_ptr<BleRcuFindMeService> BleRcuDevice::service() const
{ return findMeService(); }

template<>
inline std::shared_ptr<BleRcuInfraredService> BleRcuDevice::service() const
{ return infraredService(); }

template<>
inline std::shared_ptr<BleRcuUpgradeService> BleRcuDevice::service() const
{ return upgradeService(); }

template<>
inline std::shared_ptr<BleRcuRemoteControlService> BleRcuDevice::service() const
{ return remoteControlService(); }


#endif // !defined(BLERCUDEVICE_H)
