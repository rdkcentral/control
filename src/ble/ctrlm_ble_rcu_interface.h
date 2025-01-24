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

#ifndef __CTRLM_BLE_RCU_INTERFACE_H__
#define __CTRLM_BLE_RCU_INTERFACE_H__

#include "bluez/blercuadapter_p.h"
#include "configsettings/configsettings.h"
#include "blercu/bleservices/blercuservicesfactory.h"
#include "blercu/blercucontroller_p.h"
#include "blercu/blercuerror.h"
#include "blercu/blercudevice.h"
#include "utils/bleaddress.h"
#include "utils/slot.h"

#include "ctrlm_hal.h"
#include "ctrlm_hal_ble.h"
#include "ctrlm_utils.h"
#include <xr_mq.h>

#include <memory>
#include <map>
#include <semaphore.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <dirent.h>

#include <libevdev-1.0/libevdev/libevdev.h>


class RcuMetadata
{
public:
    RcuMetadata()
        : m_inputDeviceFd(-1)
        , m_deviceMinorId(-1)
    { }

    ~RcuMetadata() = default;

public:
    int m_inputDeviceFd;
    int m_deviceMinorId;
    std::shared_ptr<BleRcuDevice> m_rcuDevice;
};

/**
 * This class is used within ControlMgr to interact with the ctrlm_ble_rcu component.
 */
class ctrlm_ble_rcu_interface_t 
{
public:

    ctrlm_ble_rcu_interface_t(json_t *jsonConfig = NULL);
    ~ctrlm_ble_rcu_interface_t();

    void shutdown();
    void initialize();
    void setGMainLoop(GMainLoop* main_loop);

    std::vector<uint64_t> registerDevices();
    void startKeyMonitorThread();

    bool handleAddedDevice(const BleAddress &address);
    
    void handleDeepsleep(bool wakingUp);

    bool pairWithCode(unsigned int code);
    bool pairWithMacHash(unsigned int code);
    bool startScanning(int timeoutMs);
    bool unpairDevice(uint64_t ieee_address);

    bool findMe(uint64_t ieee_address, ctrlm_fmr_alarm_level_t level);

    bool getUnpairReason(uint64_t ieee_address, ctrlm_ble_RcuUnpairReason_t &reason);
    bool getRebootReason(uint64_t ieee_address, ctrlm_ble_RcuRebootReason_t &reason);
    bool sendRcuAction(uint64_t ieee_address, ctrlm_ble_RcuAction_t action, bool waitForReply = true);
    bool writeAdvertisingConfig(uint64_t ieee_address,
                                ctrlm_rcu_wakeup_config_t config,
                                int *customList,
                                int customListSize);

    bool getAudioFormat(uint64_t ieee_address, ctrlm_hal_ble_VoiceEncoding_t encoding, AudioFormat &format);
    bool startAudioStreaming(uint64_t ieee_address, ctrlm_hal_ble_VoiceEncoding_t encoding, ctrlm_hal_ble_VoiceStreamEnd_t streamEnd, int &fd);
    bool stopAudioStreaming(uint64_t ieee_address, uint32_t audioDuration = 0);
    bool getAudioStatus(uint64_t ieee_address, uint32_t &lastError, uint32_t &expectedPackets, uint32_t &actualPackets);
    bool getFirstAudioDataTime(uint64_t ieee_address, ctrlm_timestamp_t &time);

    bool setIrControl(uint64_t ieee_address, ctrlm_irdb_vendor_t vendor);
    bool programIrSignalWaveforms(uint64_t ieee_address, ctrlm_irdb_ir_codes_t &&irWaveforms, ctrlm_irdb_vendor_t vendor);
    bool eraseIrSignals(uint64_t ieee_address);

    bool startUpgrade(uint64_t ieee_address, const std::string &fwFile);
    bool cancelUpgrade(uint64_t ieee_address);

    bool setBleConnectionParams(unsigned long long ieee_address, ctrlm_hal_ble_connection_params_t &connParams);

    std::shared_ptr<ConfigSettings> getConfigSettings();

    void tickleKeyMonitorThread(ctrlm_hal_network_property_thread_monitor_t *value);
    std::vector<uint64_t> getManagedDevices();
    ctrlm_hal_ble_rcu_data_t getAllDeviceProperties(uint64_t ieee_address);


    inline void addRcuStatusChangedHandler(const Slot<ctrlm_hal_ble_RcuStatusData_t*> &func)
    {
        m_rcuStatusChangedSlots.addSlot(func);
    }
    inline void addRcuPairedHandler(const Slot<ctrlm_hal_ble_IndPaired_params_t*> &func)
    {
        m_rcuPairedSlots.addSlot(func);
    }
    inline void addRcuUnpairedHandler(const Slot<ctrlm_hal_ble_IndUnPaired_params_t*> &func)
    {
        m_rcuUnpairedSlots.addSlot(func);
    }
    inline void addRcuKeypressHandler(const Slot<ctrlm_hal_ble_IndKeypress_params_t*> &func)
    {
        m_rcuKeypressSlots.addSlot(func);
    }

    
    Slots<ctrlm_hal_ble_RcuStatusData_t*> m_rcuStatusChangedSlots;
    Slots<ctrlm_hal_ble_IndPaired_params_t*> m_rcuPairedSlots;
    Slots<ctrlm_hal_ble_IndUnPaired_params_t*> m_rcuUnpairedSlots;
    Slots<ctrlm_hal_ble_IndKeypress_params_t*> m_rcuKeypressSlots;

private:
    std::shared_ptr<bool> m_isAlive;
    GMainLoop* m_GMainLoop;
    uint32_t m_parVoiceEosTimeout;

public:
    xr_mq_t m_keyThreadMsgQ;
    sem_t m_keyThreadSem;

private:

    ctrlm_thread_t  m_keyMonitorThread;

    GDBusConnection *m_dbusConnection;
    std::shared_ptr<BleRcuAdapterBluez> m_adapter;
    std::shared_ptr<ConfigSettings> m_config;
    std::shared_ptr<BleRcuServicesFactory> m_servicesFactory;
    std::shared_ptr<BleRcuController> m_controller;

    void addNewDeviceKeyMonitorThread(BleAddress address);
};

#endif //__CTRLM_BLE_RCU_INTERFACE_H__