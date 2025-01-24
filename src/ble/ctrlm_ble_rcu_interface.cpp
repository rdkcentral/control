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


#include "ctrlm_ble_rcu_interface.h"
#include "ctrlm_ble_utils.h"
#include "ctrlm_voice_obj.h"


#define CTRLM_BLE_KEY_MSG_QUEUE_MSG_MAX         (10)
#define CTRLM_BLE_KEY_MSG_QUEUE_MSG_SIZE_MAX    (sizeof(ctrlm_ble_key_queue_device_changed_msg_t))

#define KEY_INPUT_DEVICE_BASE_DIR    "/dev/input/"
#define KEY_INPUT_DEVICE_BASE_FILE   "event"


using namespace std;


void *KeyMonitorThread(void *data);
static int HandleKeypress(ctrlm_ble_rcu_interface_t *metadata, struct input_event *event, const BleAddress &address);
static int OpenKeyInputDevice(uint64_t ieee_address);
static void FindRcuInputDevices(ctrlm_ble_rcu_interface_t *metadata, 
                                std::map <BleAddress, int> &rcuKeypressFds, 
                                fd_set &rfds, 
                                int &nfds);





ctrlm_ble_rcu_interface_t::ctrlm_ble_rcu_interface_t(json_t *jsonConfig)
    : m_isAlive(make_shared<bool>(true))
    , m_GMainLoop(NULL)
    , m_dbusConnection(NULL)
{
    m_keyMonitorThread.name = "";
    m_keyMonitorThread.id = 0;
    m_keyMonitorThread.running = false;
    
    m_keyThreadMsgQ = XR_MQ_INVALID;


    GError *error = NULL;
    m_dbusConnection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (m_dbusConnection == NULL) {
        XLOGD_ERROR("Failed to create dbus connection, error = <%s>", (error == NULL) ? "" : error->message);
        g_clear_error (&error);
        return;
    }

    if(jsonConfig != NULL) {
        XLOGD_INFO("using config from json");
        m_config = ConfigSettings::fromJsonObj(jsonConfig);
    } else {
        XLOGD_INFO("using default config");
        m_config = ConfigSettings::defaults();
    }

    // the factory for creating the BleRcu services for each device
    m_servicesFactory = std::make_shared<BleRcuServicesFactory>(m_config);
    if (!m_servicesFactory) {
        XLOGD_ERROR("failed to setup the BLE services factory");
    }
}

ctrlm_ble_rcu_interface_t::~ctrlm_ble_rcu_interface_t()
{
    *m_isAlive = false;

    shutdown();

    XLOGD_INFO ("waiting for BLE key monitor thread to exit...");

    if (m_keyMonitorThread.running) {
        ctrlm_ble_key_queue_msg_header_t msg;
        msg.type = CTRLM_BLE_KEY_QUEUE_MSG_TYPE_TERMINATE;
        ctrlm_utils_queue_msg_push(m_keyThreadMsgQ, (const char *)&msg, sizeof(msg));
        ctrlm_utils_thread_join(&m_keyMonitorThread, 2);
    }
    ctrlm_utils_message_queue_close(&m_keyThreadMsgQ);

    if (m_dbusConnection) { g_object_unref(m_dbusConnection); }
}

void ctrlm_ble_rcu_interface_t::shutdown()
{
    if (m_controller && m_adapter) {
        XLOGD_INFO("Cancelling pairing and scanning state machines...");
        m_controller->cancelPairing();
        m_controller->cancelScanning();

        XLOGD_INFO("Checking if bluez adapter is trying to discover...");
        if (m_adapter->isDiscovering()) {
            XLOGD_WARN("Stopping discovery session");
            m_adapter->stopDiscovery();
        }
        XLOGD_INFO("Checking if bluez adapter is pairable...");
        if (m_adapter->isPairable()) {
            XLOGD_WARN("Disabling bluez adapter pairability");
            m_adapter->disablePairable();
        }

        XLOGD_INFO("Shutdown all services to exit gracefully...");
        m_controller->shutdown();
    }

    // delete the adapter and controller when going into deepsleep so that we don't
    // get notified of the remote disconnection that occurs when going to sleep.
    // Upon waking, these will be re-initialized.
    m_controller.reset();
    m_adapter.reset();
}

void ctrlm_ble_rcu_interface_t::initialize()
{
    if (m_adapter == nullptr) {
        m_adapter = std::make_shared<BleRcuAdapterBluez> (m_config, m_servicesFactory, m_dbusConnection, m_GMainLoop);
    }

    if (m_controller == nullptr) {
        // create the m_controller that manages the adapter and paired devices
        m_controller = make_shared<BleRcuControllerImpl>(m_config, m_adapter);
    }

    if (m_controller) {
        auto stateChangedSlot = [this](BleRcuController::State state)
            {
                XLOGD_DEBUG("BLE RCU state = <%s>", ctrlm_rf_pair_state_str((ctrlm_rf_pair_state_t)state));

                ctrlm_hal_ble_RcuStatusData_t params;
                params.property_updated = CTRLM_HAL_BLE_PROPERTY_STATE;
                params.state = (ctrlm_rf_pair_state_t)state;
                m_rcuStatusChangedSlots.invoke(&params);
            };

        m_controller->addStateChangedSlot(Slot<BleRcuController::State>(m_isAlive, stateChangedSlot));

        auto deviceAddedSlot = [this](const BleAddress &address)
            {
                XLOGD_DEBUG("deviceAddedSlot %s", address.toString().c_str());

                if (true == handleAddedDevice(address)) {
                    ctrlm_hal_ble_rcu_data_t rcuData = getAllDeviceProperties(address.toUInt64());
                    rcuData.ieee_address = address.toUInt64();

                    ctrlm_hal_ble_IndPaired_params_t params;
                    params.rcu_data = rcuData;

                    // Indicate up to the ControlMgr BLE network
                    m_rcuPairedSlots.invoke(&params);
                    addNewDeviceKeyMonitorThread(address);
                }
            };
        m_controller->addManagedDeviceAddedSlot(Slot<const BleAddress &>(m_isAlive, deviceAddedSlot));

        auto deviceRemovedSlot = [this](const BleAddress &address)
            {
                XLOGD_DEBUG("deviceRemovedSlot %s", address.toString().c_str());

                ctrlm_ble_key_queue_device_changed_msg_t msg;
                msg.header.type = CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEVICE_REMOVED;
                msg.address = address;
                ctrlm_utils_queue_msg_push(m_keyThreadMsgQ, (const char *)&msg, sizeof(msg));

                ctrlm_hal_ble_IndUnPaired_params_t params;
                params.ieee_address = address.toUInt64();

                // Indicate up to the ControlMgr BLE network
                m_rcuUnpairedSlots.invoke(&params);
            };
        m_controller->addManagedDeviceRemovedSlot(Slot<const BleAddress &>(m_isAlive, deviceRemovedSlot));
    }

    voice_params_par_t params;
    ctrlm_get_voice_obj()->voice_params_par_get(&params);
    m_parVoiceEosTimeout = params.par_voice_eos_timeout;
}

void ctrlm_ble_rcu_interface_t::setGMainLoop(GMainLoop* main_loop)
{
    m_GMainLoop = main_loop;
}


std::vector<uint64_t> ctrlm_ble_rcu_interface_t::registerDevices()
{
    XLOGD_INFO("Get list of currently connected devices, and register RCU Device interface DBus signal handlers...");
    auto devices = m_controller->managedDevices();
    vector<uint64_t> ret;

    for (auto const &device : devices) {
        XLOGD_INFO("Setting up BLE interface for %s", device.toString().c_str());
        if (true == handleAddedDevice(device)) {
            addNewDeviceKeyMonitorThread(device);
        }
        ret.push_back(device.toUInt64());
    }
    return ret;
}

void ctrlm_ble_rcu_interface_t::startKeyMonitorThread()
{
    // Create an asynchronous queue to receive incoming messages
    if (false == ctrlm_utils_message_queue_open(&m_keyThreadMsgQ, CTRLM_BLE_KEY_MSG_QUEUE_MSG_MAX, CTRLM_BLE_KEY_MSG_QUEUE_MSG_SIZE_MAX)) {
        XLOGD_ERROR("failed to create message queue to key monitor thread");
        return;
    }
   
    // Start key monitor thread.  This has to occur after the network as already added all the controllers
    // because this thread will indicate up the device node ID when it finds it.
    m_keyMonitorThread.name = "ctrlm_ble_key_mon";
    sem_init(&m_keyThreadSem, 0, 0);
    if (false == ctrlm_utils_thread_create(&m_keyMonitorThread, KeyMonitorThread, this)) {
        sem_destroy(&m_keyThreadSem);
    } else  {
        // Block until initialization is complete or a timeout occurs
        XLOGD_INFO("Waiting for %s thread initialization...", m_keyMonitorThread.name);
        sem_wait(&m_keyThreadSem);
        sem_destroy(&m_keyThreadSem);
    }
}

void ctrlm_ble_rcu_interface_t::tickleKeyMonitorThread(ctrlm_hal_network_property_thread_monitor_t *value) 
{
    if (value == 0) {
        XLOGD_ERROR("Param is NULL");
        return;
    }
    XLOGD_DEBUG("ctrlm checks whether we are still alive");

    ctrlm_ble_key_thread_monitor_msg_t msg;
    msg.header.type = CTRLM_BLE_KEY_QUEUE_MSG_TYPE_TICKLE;
    msg.response = value->response;
    ctrlm_utils_queue_msg_push(m_keyThreadMsgQ, (const char *)&msg, sizeof(msg));
}

bool ctrlm_ble_rcu_interface_t::handleAddedDevice(const BleAddress &address)
{
    XLOGD_INFO("device <%s>", address.toString().c_str());
    if (!m_controller) {
        XLOGD_ERROR("m_controller is NULL!!!");
        return false;
    }

    shared_ptr<BleRcuDevice> device = m_controller->managedDevice(address);

    if (!device || !device->isValid()) {
        XLOGD_ERROR("BleRcuDevice is invalid");
        return false;
    }

    //add slots to BleRcuDevice
    auto connectedChangedSlot = [this, address](bool connected)
        {
            XLOGD_INFO("BLE RCU %s connected changed to <%s>", address.toString().c_str(), connected ? "TRUE" : "FALSE");

            if (connected) {
                if (m_keyMonitorThread.running) {
                    addNewDeviceKeyMonitorThread(address);
                }
            }

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_CONNECTED;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.connected = connected;
            m_rcuStatusChangedSlots.invoke(&params);

        };
    // We use READY signal from the device as a proxy for connected
    device->addReadyChangedSlot(Slot<bool>(m_isAlive, connectedChangedSlot));

    auto nameChangedSlot = [this, address](const string &name)
        {
            XLOGD_DEBUG("BLE RCU name changed to <%s>", name.c_str());

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_NAME;
            params.rcu_data.ieee_address = address.toUInt64();
            errno_t safec_rc = strcpy_s(params.rcu_data.name, sizeof(params.rcu_data.name), name.c_str());
            ERR_CHK(safec_rc);
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addNameChangedSlot(Slot<const string&>(m_isAlive, nameChangedSlot));


    auto manufacturerChangedSlot = [this, address](const string &name)
        {
            XLOGD_DEBUG("BLE RCU manufacturer name changed to <%s>", name.c_str());

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_MANUFACTURER;
            params.rcu_data.ieee_address = address.toUInt64();
            errno_t safec_rc = strcpy_s(params.rcu_data.manufacturer, sizeof(params.rcu_data.manufacturer), name.c_str());
            ERR_CHK(safec_rc);
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addManufacturerNameChangedSlot(Slot<const string&>(m_isAlive, manufacturerChangedSlot));

    auto modelChangedSlot = [this, address](const string &name)
        {
            XLOGD_DEBUG("BLE RCU model number changed to <%s>", name.c_str());

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_MODEL;
            params.rcu_data.ieee_address = address.toUInt64();
            errno_t safec_rc = strcpy_s(params.rcu_data.model, sizeof(params.rcu_data.model), name.c_str());
            ERR_CHK(safec_rc);
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addModelNumberChangedSlot(Slot<const string&>(m_isAlive, modelChangedSlot));

    auto serialChangedSlot = [this, address](const string &name)
        {
            XLOGD_DEBUG("BLE RCU serial number changed to <%s>", name.c_str());

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_SERIAL_NUMBER;
            params.rcu_data.ieee_address = address.toUInt64();
            errno_t safec_rc = strcpy_s(params.rcu_data.serial_number, sizeof(params.rcu_data.serial_number), name.c_str());
            ERR_CHK(safec_rc);
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addSerialNumberChangedSlot(Slot<const string&>(m_isAlive, serialChangedSlot));

    auto hardwareChangedSlot = [this, address](const string &name)
        {
            XLOGD_DEBUG("BLE RCU hardware revision changed to <%s>", name.c_str());

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_HW_REVISION;
            params.rcu_data.ieee_address = address.toUInt64();
            errno_t safec_rc = strcpy_s(params.rcu_data.hw_revision, sizeof(params.rcu_data.hw_revision), name.c_str());
            ERR_CHK(safec_rc);
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addHardwareRevisionChangedSlot(Slot<const string&>(m_isAlive, hardwareChangedSlot));

    auto firmwareChangedSlot = [this, address](const string &name)
        {
            XLOGD_DEBUG("BLE RCU firmware version changed to <%s>", name.c_str());

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_FW_REVISION;
            params.rcu_data.ieee_address = address.toUInt64();
            errno_t safec_rc = strcpy_s(params.rcu_data.fw_revision, sizeof(params.rcu_data.fw_revision), name.c_str());
            ERR_CHK(safec_rc);
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addFirmwareVersionChangedSlot(Slot<const string&>(m_isAlive, firmwareChangedSlot));

    auto softwareChangedSlot = [this, address](const string &name)
        {
            XLOGD_INFO("BLE RCU %s software version changed to <%s>", address.toString().c_str(), name.c_str());

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_SW_REVISION;
            params.rcu_data.ieee_address = address.toUInt64();
            errno_t safec_rc = strcpy_s(params.rcu_data.sw_revision, sizeof(params.rcu_data.sw_revision), name.c_str());
            ERR_CHK(safec_rc);
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addSoftwareVersionChangedSlot(Slot<const string&>(m_isAlive, softwareChangedSlot));

    auto batteryChangedSlot = [this, address](int level)
        {
            XLOGD_DEBUG("BLE RCU battery level changed to <%d%%>", level);

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_BATTERY_LEVEL;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.battery_level = (unsigned int)level;
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addBatteryLevelChangedSlot(Slot<int>(m_isAlive, batteryChangedSlot));


    auto unpairReasonChangedSlot = [this, address](uint8_t reason)
        {
            XLOGD_DEBUG("BLE RCU unpair reason changed to <%u>", reason);

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_UNPAIR_REASON;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.unpair_reason = (ctrlm_ble_RcuUnpairReason_t)reason;
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addUnpairReasonChangedSlot(Slot<uint8_t>(m_isAlive, unpairReasonChangedSlot));

    auto rebootReasonChangedSlot = [this, address](uint8_t reason, std::string assertReport)
        {
            XLOGD_DEBUG("BLE RCU reboot reason changed to <%u>", reason);

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_REBOOT_REASON;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.reboot_reason = (ctrlm_ble_RcuRebootReason_t)reason;
            errno_t safec_rc = strcpy_s(params.rcu_data.assert_report, sizeof(params.rcu_data.assert_report), assertReport.c_str());
            ERR_CHK(safec_rc);
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addRebootReasonChangedSlot(Slot<uint8_t, std::string>(m_isAlive, rebootReasonChangedSlot));

    auto lastKeypressChangedSlot = [this, address](uint8_t key)
        {
            XLOGD_DEBUG("BLE RCU last key press changed to <%u>", key);

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_LAST_WAKEUP_KEY;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.last_wakeup_key = ctrlm_ble_utils_ConvertUsbKbdCodeToLinux(key);
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addLastKeypressChangedSlot(Slot<uint8_t>(m_isAlive, lastKeypressChangedSlot));

    auto advConfigChangedSlot = [this, address](uint8_t config)
        {
            XLOGD_DEBUG("BLE RCU advertising config changed to <%u>", config);

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_WAKEUP_CONFIG;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.wakeup_config = config;
            m_rcuStatusChangedSlots.invoke(&params);

        };
    device->addAdvConfigChangedSlot(Slot<uint8_t>(m_isAlive, advConfigChangedSlot));

    auto advConfigCustomListChangedSlot = [this, address](const std::vector<uint8_t> &list)
        {
            XLOGD_DEBUG("BLE RCU advertising config custom list changed.");

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_WAKEUP_CUSTOM_LIST;
            params.rcu_data.ieee_address = address.toUInt64();

            if (list.size() <= (sizeof(params.rcu_data.wakeup_custom_list) / sizeof(params.rcu_data.wakeup_custom_list[0]))) {
                int idx = 0;
                for (unsigned int i = 0; i < list.size(); i += 2) {
                    if (list[i] != 0) {
                        // Format of this list is the keycode followed by config byte.  We only care about the keycodes (even bytes)
                        params.rcu_data.wakeup_custom_list[idx] = ctrlm_ble_utils_ConvertUsbKbdCodeToLinux(list[i]);
                        idx++;
                    }
                }
                params.rcu_data.wakeup_custom_list_size = idx;
            }
            m_rcuStatusChangedSlots.invoke(&params);
        };
    device->addAdvConfigCustomListChangedSlot(Slot<const std::vector<uint8_t> &>(m_isAlive, advConfigCustomListChangedSlot));


    auto audioStreamingChangedSlot = [this, address](bool streaming)
        {
            XLOGD_DEBUG("BLE RCU audio streaming changed to %s", streaming ? "TRUE" : "FALSE");

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_AUDIO_STREAMING;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.audio_streaming = streaming;

            m_rcuStatusChangedSlots.invoke(&params);
        };
    device->addStreamingChangedSlot(Slot<bool>(m_isAlive, audioStreamingChangedSlot));


    auto audioGainChangedSlot = [this, address](uint8_t gain)
        {
            XLOGD_DEBUG("BLE RCU gain level changed to %u", gain);

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_AUDIO_GAIN_LEVEL;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.audio_gain_level = gain;

            m_rcuStatusChangedSlots.invoke(&params);
        };
    device->addGainLevelChangedSlot(Slot<uint8_t>(m_isAlive, audioGainChangedSlot));


    auto audioCodecsChangedSlot = [this, address](uint32_t codecs)
        {
            XLOGD_DEBUG("BLE RCU audio codecs changed to 0x%X", codecs);

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_AUDIO_CODECS;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.audio_codecs = codecs;

            m_rcuStatusChangedSlots.invoke(&params);
        };
    device->addAudioCodecsChangedSlot(Slot<uint32_t>(m_isAlive, audioCodecsChangedSlot));

    auto codeIdChangedSlot = [this, address](int32_t codeId)
        {
            XLOGD_DEBUG("BLE RCU IR code ID changed to %d", codeId);

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_IR_CODE;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.ir_code = codeId;

            m_rcuStatusChangedSlots.invoke(&params);
        };
    device->addCodeIdChangedSlot(Slot<int32_t>(m_isAlive, codeIdChangedSlot));

    auto irSupportChangedSlot = [this, address](uint8_t irSupport)
        {
            XLOGD_INFO("BLE RCU IR Support bitmap changed to 0x%X", irSupport);

            ctrlm_hal_ble_RcuStatusData_t params;

            // Get supported vendor types from IR protocol bitmask value
            int num_irdbs_supported = 0;
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (irSupport & (0x1 << bit)) {
                    params.rcu_data.irdbs_supported[num_irdbs_supported] = ctrlm_ble_utils_IrControlToVendor(0x1 << bit);
                    num_irdbs_supported++;
                }
            }

            params.property_updated = CTRLM_HAL_BLE_PROPERTY_IRDBS_SUPPORTED;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.num_irdbs_supported = num_irdbs_supported;

            m_rcuStatusChangedSlots.invoke(&params);
        };
    device->addIrSupportChangedSlot(Slot<uint8_t>(m_isAlive, irSupportChangedSlot));

    auto upgradingChangedSlot = [this, address](bool upgrading)
        {
            ctrlm_hal_ble_RcuStatusData_t params;

            params.property_updated = CTRLM_HAL_BLE_PROPERTY_IS_UPGRADING;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.is_upgrading = upgrading;

            m_rcuStatusChangedSlots.invoke(&params);
        };
    device->addUpgradingChangedSlot(Slot<bool>(m_isAlive, upgradingChangedSlot));

    auto upgradeProgressChangedSlot = [this, address](int progress)
        {
            ctrlm_hal_ble_RcuStatusData_t params;

            params.property_updated = CTRLM_HAL_BLE_PROPERTY_UPGRADE_PROGRESS;
            params.rcu_data.ieee_address = address.toUInt64();
            params.rcu_data.upgrade_progress = progress;

            m_rcuStatusChangedSlots.invoke(&params);
        };
    device->addUpgradeProgressChangedSlot(Slot<int>(m_isAlive, upgradeProgressChangedSlot));

    auto upgradeErrorSlot = [this, address](string errorMessage)
        {
            ctrlm_hal_ble_RcuStatusData_t params;

            params.property_updated = CTRLM_HAL_BLE_PROPERTY_UPGRADE_ERROR;
            params.rcu_data.ieee_address = address.toUInt64();
            errno_t safec_rc = strcpy_s(params.rcu_data.upgrade_error, sizeof(params.rcu_data.upgrade_error), errorMessage.c_str());
            ERR_CHK(safec_rc);

            m_rcuStatusChangedSlots.invoke(&params);
        };
    device->addUpgradeErrorSlot(Slot<string>(m_isAlive, upgradeErrorSlot));


    return true;   
}


void ctrlm_ble_rcu_interface_t::handleDeepsleep(bool wakingUp)
{
    XLOGD_WARN("Deepsleep transitioning, waking up = %s", wakingUp ? "TRUE" : "FALSE");
    
    if (wakingUp) {

        initialize();

        if (m_adapter) {
            m_adapter->reconnectAllDevices();
        }


        ctrlm_ble_key_queue_msg_header_t msg;
        msg.type = CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEEPSLEEP_WAKEUP;
        ctrlm_utils_queue_msg_push(m_keyThreadMsgQ, (const char *)&msg, sizeof(msg));

    } else {

        // We are going into deepsleep, clean up
        shutdown();
    }
}


ctrlm_hal_ble_rcu_data_t ctrlm_ble_rcu_interface_t::getAllDeviceProperties(uint64_t ieee_address)
{
    ctrlm_hal_ble_rcu_data_t ret;
    errno_t safec_rc = memset_s(&ret, sizeof(ret), 0, sizeof(ret)); ERR_CHK(safec_rc);

    ret.ieee_address = ieee_address;
    
    shared_ptr<BleRcuDevice> device = m_controller->managedDevice(ieee_address);

    safec_rc = strcpy_s(ret.name, sizeof(ret.name), device->name().c_str()); ERR_CHK(safec_rc);

    ret.connected = device->isReady();

    //Battery Service
    ret.battery_level = device->batteryLevel();
    
    //Device Info Service
    safec_rc = strcpy_s(ret.manufacturer, sizeof(ret.manufacturer), device->manufacturer().c_str()); ERR_CHK(safec_rc);
    safec_rc = strcpy_s(ret.model, sizeof(ret.model), device->model().c_str()); ERR_CHK(safec_rc);
    safec_rc = strcpy_s(ret.fw_revision, sizeof(ret.fw_revision), device->firmwareRevision().c_str()); ERR_CHK(safec_rc);
    safec_rc = strcpy_s(ret.hw_revision, sizeof(ret.hw_revision), device->hardwareRevision().c_str()); ERR_CHK(safec_rc);
    safec_rc = strcpy_s(ret.sw_revision, sizeof(ret.sw_revision), device->softwareRevision().c_str()); ERR_CHK(safec_rc);
    safec_rc = strcpy_s(ret.serial_number, sizeof(ret.serial_number), device->serialNumber().c_str()); ERR_CHK(safec_rc);

    //Remote Control Service
    ret.unpair_reason = (ctrlm_ble_RcuUnpairReason_t)device->unpairReason();
    ret.reboot_reason = (ctrlm_ble_RcuRebootReason_t)device->rebootReason();
    ret.last_wakeup_key = ctrlm_ble_utils_ConvertUsbKbdCodeToLinux(device->lastKeypress());
    ret.wakeup_config = device->advConfig();

    vector<uint8_t> list = device->advConfigCustomList();

    if (list.size() <= (sizeof(ret.wakeup_custom_list) / sizeof(ret.wakeup_custom_list[0]))) {
        // Format of this list is the keycode followed by config byte.  We only care about the keycodes (even bytes)
        int idx = 0;
        for (unsigned int i = 0; i < list.size(); i += 2) {
            if (list[i] != 0) {
                ret.wakeup_custom_list[idx] = ctrlm_ble_utils_ConvertUsbKbdCodeToLinux(list[i]);
                idx++;
            }
        }
        ret.wakeup_custom_list_size = idx;
    }

    // IR Service
    uint8_t irSupport = device->irSupport();
    int num_irdbs_supported = 0;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (irSupport & (0x1 << bit)) {
            ret.irdbs_supported[num_irdbs_supported] = ctrlm_ble_utils_IrControlToVendor(0x1 << bit);
            num_irdbs_supported++;
        }
    }
    ret.num_irdbs_supported = num_irdbs_supported;

    // Voice Service
    ret.audio_codecs = device->audioCodecs();
    ret.audio_gain_level = device->audioGainLevel();


    return ret;
}

bool ctrlm_ble_rcu_interface_t::pairWithCode(unsigned int code)
{
    if (!m_controller->startPairing(0, (uint8_t) code)) {
        BleRcuError error = m_controller->lastError();

        // Remote will continually send out IR pairing signals until the BLE pair request
        // has been received.  This means that the "Already in pairing state" error is normal.
        // Let's omit this error print because it only serves to confuse those analyzing the logs.
        if (error.message() != "Already in pairing state") {
            XLOGD_ERROR("controller failed to start pairing, %s: %s", error.name().c_str(), error.message().c_str());
        }
        return false;
    } else {
        XLOGD_INFO("started pairing with code %hhu", code);
    }
    return true;
}

bool ctrlm_ble_rcu_interface_t::pairWithMacHash(unsigned int code)
{
    if (!m_controller->startPairingMacHash(0, (uint8_t) code)) {
        BleRcuError error = m_controller->lastError();

        // Remote will continually send out IR pairing signals until the BLE pair request
        // has been received.  This means that the "Already in pairing state" error is normal.
        // Let's omit this error print because it only serves to confuse those analyzing the logs.
        if (error.message() != "Already in pairing state") {
            XLOGD_ERROR("controller failed to start pairing, %s: %s", error.name().c_str(), error.message().c_str());
        }
        return false;
    } else {
        XLOGD_INFO("started pairing with MAC hash 0x%X", code);
    }
    return true;
}

bool ctrlm_ble_rcu_interface_t::startScanning(int timeoutMs)
{
    XLOGD_INFO("starting BLE scan with timeout %dms", timeoutMs);
    if (m_controller) {
        if (!m_controller->startScanning(timeoutMs)) {
            BleRcuError error = m_controller->lastError();
            XLOGD_ERROR("controller failed to start scan, %s: %s", error.name().c_str(), error.message().c_str());
            return false;
        }
        return true;
    }
    return false;
}

bool ctrlm_ble_rcu_interface_t::unpairDevice(uint64_t ieee_address)
{
    BleAddress address(ieee_address);

    XLOGD_INFO("unpairing device %s", address.toString().c_str());
    if (m_controller) {
        // This method currently doesn't wait for a success or failure response from bluez so
        // error message isn't captured in lastError()
        return m_controller->unpairDevice(address);
    }
    return false;
}

bool ctrlm_ble_rcu_interface_t::findMe(uint64_t ieee_address, ctrlm_fmr_alarm_level_t level)
{
    // This method will wait for the operation to complete, so init a semaphore
    sem_t semaphore;
    
    bool success = false;

    // lambda invoked when the request returns
    auto replyHandler = [this, &semaphore, &success](PendingReply<> *reply) mutable
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                XLOGD_ERROR("findMe failed due to <%s>", reply->errorMessage().c_str());
                success = false;
            } else {
                XLOGD_DEBUG("findMe succeeded");
                success = true;
            }
            sem_post(&semaphore);
        };


    BleAddress address(ieee_address);
    XLOGD_INFO("triggering \"find me\" operation on remote %s", address.toString().c_str());
    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {
            sem_init(&semaphore, 0, 0);
            device->findMe((uint8_t)level, PendingReply<>(m_isAlive, replyHandler));
        } else {
            return false;
        }
    } else {
        return false;
    }

    // Wait for the result semaphore to be signaled
    sem_wait(&semaphore);
    sem_destroy(&semaphore);

    return success;
}

bool ctrlm_ble_rcu_interface_t::sendRcuAction(uint64_t ieee_address, ctrlm_ble_RcuAction_t action, bool waitForReply)
{
    // This method will wait for the operation to complete, so init a semaphore
    sem_t semaphore;

    bool success = false;

    BleAddress address(ieee_address);

    // lambda invoked when the request returns
    auto replyHandler = [this, &semaphore, &success, waitForReply, action, address](PendingReply<> *reply) mutable
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                XLOGD_ERROR("sendRcuAction failed due to <%s>", reply->errorMessage().c_str());
                success = false;
            } else {
                XLOGD_INFO("RCU action %s successfully sent to remote %s", 
                        ctrlm_ble_rcu_action_str(action), address.toString().c_str());
                success = true;
            }
            if (waitForReply) { sem_post(&semaphore); }
        };



    XLOGD_INFO("triggering RCU action %s on remote %s", 
            ctrlm_ble_rcu_action_str(action), address.toString().c_str());

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {
            if (waitForReply) { sem_init(&semaphore, 0, 0); }
            success = true;
            device->sendRcuAction((uint8_t)action, PendingReply<>(m_isAlive, replyHandler));
        } else {
            return false;
        }
    } else {
        return false;
    }

    // Wait for the result semaphore to be signaled
    if (waitForReply) { 
        sem_wait(&semaphore);
        sem_destroy(&semaphore);
    }

    return success;
}

bool ctrlm_ble_rcu_interface_t::writeAdvertisingConfig(uint64_t ieee_address, 
                                                       ctrlm_rcu_wakeup_config_t config, 
                                                       int *customList,
                                                       int customListSize)
{
    BleAddress address(ieee_address);

    // lambda invoked when the request returns
    auto replyHandler = [this, address](PendingReply<> *reply) mutable
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                XLOGD_ERROR("%s: writeAdvertisingConfig failed due to <%s>", 
                        address.toString().c_str(), reply->errorMessage().c_str());
            } else {
                XLOGD_INFO("successfully wrote RCU advertising config on remote %s", address.toString().c_str());
            }
        };


    XLOGD_INFO("writing RCU advertising config on remote %s", address.toString().c_str());

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {

            vector<uint8_t> listConverted;
            if (config == CTRLM_RCU_WAKEUP_CONFIG_CUSTOM && customList != NULL) {
                for (int i = 0; i < customListSize; i++) {
                    listConverted.push_back(ctrlm_ble_utils_ConvertLinuxCodeToUsbKdb(customList[i]));
                    listConverted.push_back(0);     // 0 to send directed advertisment for the key
                }
            }
            device->writeAdvertisingConfig((uint8_t)config, listConverted, PendingReply<>(m_isAlive, replyHandler));

            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}


bool ctrlm_ble_rcu_interface_t::getUnpairReason(uint64_t ieee_address, ctrlm_ble_RcuUnpairReason_t &reason)
{
    BleAddress address(ieee_address);

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {
            reason = (ctrlm_ble_RcuUnpairReason_t)device->unpairReason();
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

bool ctrlm_ble_rcu_interface_t::getRebootReason(uint64_t ieee_address, ctrlm_ble_RcuRebootReason_t &reason)
{
    BleAddress address(ieee_address);

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {
            reason = (ctrlm_ble_RcuRebootReason_t)device->rebootReason();
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
}


bool ctrlm_ble_rcu_interface_t::getAudioFormat(uint64_t ieee_address, ctrlm_hal_ble_VoiceEncoding_t encoding, AudioFormat &format) {
    BleAddress address(ieee_address);
    XLOGD_INFO("getting RCU audio format on remote %s", address.toString().c_str());

    BleRcuDevice::Encoding device_encoding = BleRcuDevice::Encoding::InvalidEncoding;

    if(encoding == CTRLM_HAL_BLE_ENCODING_ADPCM) {
        device_encoding = BleRcuDevice::Encoding::ADPCM_FRAME;
    } else if(encoding == CTRLM_HAL_BLE_ENCODING_PCM) {
        device_encoding = BleRcuDevice::Encoding::PCM16;
    } else {
        XLOGD_ERROR("getAudioFormat failed due to invalid encoding");
        return(false);
    }

    if(m_controller) {
        const auto device = m_controller->managedDevice(address);
        if(device) {
            return(device->getAudioFormat(device_encoding, format));
        }
    }

    return(false);
}

bool ctrlm_ble_rcu_interface_t::startAudioStreaming(uint64_t ieee_address, ctrlm_hal_ble_VoiceEncoding_t encoding, ctrlm_hal_ble_VoiceStreamEnd_t streamEnd, int &fd)
{
    // This method will wait for the operation to complete, so init a semaphore
    sem_t semaphore;
    
    bool success = false;

    // lambda invoked when the request returns
    auto replyHandler = [this, &semaphore, &success, &fd](PendingReply<int> *reply) mutable
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("startAudioStreaming failed due to <%s>", reply->errorMessage().c_str());
                success = false;
            } else {
                success = true;
                fd = reply->result();
                XLOGD_DEBUG("startAudioStreaming succeeded, fd = %d", fd);
            }
            sem_post(&semaphore);
        };


    BleAddress address(ieee_address);
    XLOGD_INFO("starting RCU audio streaming on remote %s", address.toString().c_str());

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {
            
            sem_init(&semaphore, 0, 0);
            
            // encoding options from startAudioStreaming API:
            // enum Encoding {
            //     ADPCM,
            //     PCM16,
            //     InvalidEncoding
            // };

            uint32_t durationMax = 0;
            if(streamEnd == CTRLM_HAL_BLE_VOICE_STREAM_END_ON_AUDIO_DURATION) {
                durationMax = m_parVoiceEosTimeout;
            }

            device->startAudioStreaming( (encoding == CTRLM_HAL_BLE_ENCODING_PCM) ? 1 : 0, PendingReply<int>(m_isAlive, replyHandler), durationMax );

        } else {
            return false;
        }
    } else {
        return false;
    }

    // Wait for the result semaphore to be signaled
    sem_wait(&semaphore);
    sem_destroy(&semaphore);

    return success;
}
bool ctrlm_ble_rcu_interface_t::stopAudioStreaming(uint64_t ieee_address, uint32_t audioDuration)
{
    // This method will wait for the operation to complete, so init a semaphore
    sem_t semaphore;
    
    bool success = false;

    // lambda invoked when the request returns
    auto replyHandler = [this, &semaphore, &success](PendingReply<> *reply) mutable
        {
            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("stopAudioStreaming failed due to <%s>", reply->errorMessage().c_str());
                success = false;
            } else {
                XLOGD_DEBUG("stopAudioStreaming succeeded");
                success = true;
            }
            sem_post(&semaphore);
        };


    BleAddress address(ieee_address);
    XLOGD_INFO("stopping RCU audio streaming on remote %s", address.toString().c_str());

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {

            sem_init(&semaphore, 0, 0);

            device->stopAudioStreaming( audioDuration, PendingReply<>(m_isAlive, replyHandler) );
            
        } else {
            return false;
        }
    } else {
        return false;
    }

    // Wait for the result semaphore to be signaled
    sem_wait(&semaphore);
    sem_destroy(&semaphore);

    return success;
}

bool ctrlm_ble_rcu_interface_t::getAudioStatus(uint64_t ieee_address, 
                                               uint32_t &lastError, 
                                               uint32_t &expectedPackets, 
                                               uint32_t &actualPackets)
{
    BleAddress address(ieee_address);

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {
            device->getAudioStatus( lastError, expectedPackets, actualPackets );
            return true;
        }
    }

    return false;
}

bool ctrlm_ble_rcu_interface_t::getFirstAudioDataTime(uint64_t ieee_address, ctrlm_timestamp_t &time)
{
    BleAddress address(ieee_address);

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {
            device->getFirstAudioDataTime( time );
            return true;
        }
    }

    return false;
}

bool ctrlm_ble_rcu_interface_t::setIrControl(uint64_t ieee_address, ctrlm_irdb_vendor_t vendor)
{
    // This method will wait for the operation to complete, so init a semaphore
    sem_t semaphore;

    bool success = false;
    // lambda invoked when the request returns
    auto replyHandler = [this, &semaphore, &success](PendingReply<> *reply) mutable
        {
            // check for errors (only for logging)
            if (reply->isError()) {
                XLOGD_ERROR("setIrControl failed due to <%s>", reply->errorMessage().c_str());
                success = false;
            } else {
                XLOGD_DEBUG("setIrControl succeeded");
                success = true;
            }
            sem_post(&semaphore);
        };

    BleAddress address(ieee_address);
    XLOGD_INFO("writing IR Control characteristic on remote %s", address.toString().c_str());

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        uint8_t irControl = ctrlm_ble_utils_VendorToIrControlBitmask(vendor);
        if (device) {
            sem_init(&semaphore, 0, 0);
            device->setIrControl(irControl, PendingReply<>(m_isAlive, replyHandler));
        } else {
            return false;
        }
    } else {
        return false;
    }

    // Wait for the result semaphore to be signaled
    sem_wait(&semaphore);
    sem_destroy(&semaphore);

    return success;
}

bool ctrlm_ble_rcu_interface_t::programIrSignalWaveforms(uint64_t ieee_address, 
                                                         ctrlm_irdb_ir_codes_t &&irWaveforms, 
                                                         ctrlm_irdb_vendor_t vendor)
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply) mutable
        {
            bool success = false;

            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("programIrSignalWaveforms failed due to <%s>", reply->errorMessage().c_str());
                success = false;
            } else {
                XLOGD_DEBUG("programIrSignalWaveforms succeeded");
                success = true;
            }

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_IR_STATE;
            params.ir_state = success ? CTRLM_IR_STATE_COMPLETE : CTRLM_IR_STATE_FAILED;
            m_rcuStatusChangedSlots.invoke(&params);
        };


    BleAddress address(ieee_address);
    XLOGD_INFO("programming IR waveforms on remote %s", address.toString().c_str());

    bool success = true;
    uint8_t irControl = ctrlm_ble_utils_VendorToIrControlBitmask(vendor);

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {

            // This enum needs to be in sync with GattInfraredSignal::Key enum in ctrlm-ble-rcu component
            enum BleRcuKey {
                BleRcuKey_PowerPrimary = 0,
                BleRcuKey_PowerSecondary,
                BleRcuKey_VolUp,
                BleRcuKey_VolDown,
                BleRcuKey_Mute,
                BleRcuKey_Input,
                BleRcuKey_INVALID
            };

            map<uint32_t, vector<uint8_t>> bleRcuWaveforms;

            for (auto &waveform : irWaveforms) {
                BleRcuKey key_code;
                switch (waveform.first) {
                    case CTRLM_KEY_CODE_POWER_TOGGLE:       //0x6B
                        key_code = BleRcuKey_PowerPrimary;
                        break;
                    case CTRLM_KEY_CODE_AVR_POWER_TOGGLE:   //0x68
                        key_code = BleRcuKey_PowerSecondary;
                        break;
                    case CTRLM_KEY_CODE_VOL_UP:             //0x41
                        key_code = BleRcuKey_VolUp;
                        break;
                    case CTRLM_KEY_CODE_VOL_DOWN:           //0x42
                        key_code = BleRcuKey_VolDown;
                        break;
                    case CTRLM_KEY_CODE_MUTE:               //0x43
                        key_code = BleRcuKey_Mute;
                        break;
                    case CTRLM_KEY_CODE_INPUT_SELECT:       //0x34
                        key_code = BleRcuKey_Input;
                        break;
                    default:
                        XLOGD_WARN("Unhandled key received <0x%X>, ignoring...", waveform.first);
                        continue;
                }

                bleRcuWaveforms[key_code] = std::move(waveform.second);
            }

            device->programIrSignalWaveforms(std::move(bleRcuWaveforms), irControl, 
                    PendingReply<>(m_isAlive, replyHandler) );
            
        } else {
            success = false;
        }
    } else {
        success = false;
    }

    ctrlm_hal_ble_RcuStatusData_t params;
    params.property_updated = CTRLM_HAL_BLE_PROPERTY_IR_STATE;
    params.ir_state = success ? CTRLM_IR_STATE_WAITING : CTRLM_IR_STATE_FAILED;
    m_rcuStatusChangedSlots.invoke(&params);

    return success;
}

bool ctrlm_ble_rcu_interface_t::eraseIrSignals(uint64_t ieee_address)
{
    // lambda invoked when the request returns
    auto replyHandler = [this](PendingReply<> *reply) mutable
        {
            bool success = false;

            // check for errors
            if (reply->isError()) {
                XLOGD_ERROR("eraseIrSignals failed due to <%s>", reply->errorMessage().c_str());
                success = false;
            } else {
                XLOGD_DEBUG("eraseIrSignals succeeded");
                success = true;
            }

            ctrlm_hal_ble_RcuStatusData_t params;
            params.property_updated = CTRLM_HAL_BLE_PROPERTY_IR_STATE;
            params.ir_state = success ? CTRLM_IR_STATE_COMPLETE : CTRLM_IR_STATE_FAILED;
            m_rcuStatusChangedSlots.invoke(&params);
        };


    BleAddress address(ieee_address);
    XLOGD_INFO("erasing programmed IR waveforms on remote %s", address.toString().c_str());

    bool success = true;
    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {

            device->eraseIrSignals( PendingReply<>(m_isAlive, replyHandler) );
            
        } else {
            success = false;
        }
    } else {
        success = false;
    }

    ctrlm_hal_ble_RcuStatusData_t params;
    params.property_updated = CTRLM_HAL_BLE_PROPERTY_IR_STATE;
    params.ir_state = success ? CTRLM_IR_STATE_WAITING : CTRLM_IR_STATE_FAILED;
    m_rcuStatusChangedSlots.invoke(&params);

    return success;
}


bool ctrlm_ble_rcu_interface_t::startUpgrade(uint64_t ieee_address, const std::string &fwFile)
{
    // lambda invoked when the request returns
    auto replyHandler = [this, ieee_address](PendingReply<> *reply) mutable
        {
            if (reply->isError()) {
                XLOGD_ERROR("startUpgrade failed due to <%s>", reply->errorMessage().c_str());

                ctrlm_hal_ble_RcuStatusData_t params;
                params.property_updated = CTRLM_HAL_BLE_PROPERTY_UPGRADE_ERROR;
                params.rcu_data.ieee_address = ieee_address;
                errno_t safec_rc = strcpy_s(params.rcu_data.upgrade_error, sizeof(params.rcu_data.upgrade_error), reply->errorMessage().c_str());
                ERR_CHK(safec_rc);

                m_rcuStatusChangedSlots.invoke(&params);

            } else {
                XLOGD_DEBUG("startUpgrade request succeeded");
            }
        };


    BleAddress address(ieee_address);
    XLOGD_INFO("starting firmware upgrade on remote %s with file %s", address.toString().c_str(), fwFile.c_str());

    bool success = true;
    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {

            device->startUpgrade( fwFile, PendingReply<>(m_isAlive, replyHandler) );
            
        } else {
            success = false;
        }
    } else {
        success = false;
    }

    return success;
}

bool ctrlm_ble_rcu_interface_t::cancelUpgrade(uint64_t ieee_address)
{
    // This method will wait for the operation to complete, so init a semaphore
    sem_t semaphore;
    
    bool success = false;
    
    BleAddress address(ieee_address);


    // lambda invoked when the request returns
    auto replyHandler = [this, &semaphore, &success, address](PendingReply<> *reply)
        {
            if (reply->isError()) {
                XLOGD_ERROR("cancelUpgrade failed due to <%s>", reply->errorMessage().c_str());
                success = false;
            } else {
                XLOGD_INFO("successfully canceled firmware upgrade on remote %s", address.toString().c_str());
                success = true;
            }
            sem_post(&semaphore);
        };


    XLOGD_INFO("cancelling firmware upgrade on remote %s", address.toString().c_str());

    if (m_controller) {
        const auto device = m_controller->managedDevice(address);
        if (device) {

            sem_init(&semaphore, 0, 0);

            device->cancelUpgrade( PendingReply<>(m_isAlive, replyHandler) );
            
        } else {
            return false;
        }
    } else {
        return false;
    }

    // Wait for the result semaphore to be signaled
    sem_wait(&semaphore);
    sem_destroy(&semaphore);

    return success;
}

std::vector<uint64_t> ctrlm_ble_rcu_interface_t::getManagedDevices()
{
    XLOGD_INFO("Get list of currently managed devices");
    auto devices = m_controller->managedDevices();
    vector<uint64_t> ret;

    for (auto const &device : devices) {
        ret.push_back(device.toUInt64());
    }
    return ret;
}



bool ctrlm_ble_rcu_interface_t::setBleConnectionParams(unsigned long long ieee_address, ctrlm_hal_ble_connection_params_t &connParams)
{
    if (m_adapter) {
        return m_adapter->setConnectionParams(BleAddress(ieee_address), 
                                                connParams.minInterval,
                                                connParams.maxInterval,
                                                connParams.latency,
                                                connParams.supvTimeout);
    } else {
        XLOGD_ERROR("m_adapter is NULL");
    }
    return false;
}

std::shared_ptr<ConfigSettings> ctrlm_ble_rcu_interface_t::getConfigSettings() {
    return(m_config);
}

void ctrlm_ble_rcu_interface_t::addNewDeviceKeyMonitorThread(BleAddress address){
    ctrlm_ble_key_queue_device_changed_msg_t msg;
    msg.header.type = CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEVICE_ADDED;
    msg.address = address;
    ctrlm_utils_queue_msg_push(m_keyThreadMsgQ, (const char *)&msg, sizeof(msg));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BEGIN - Key Monitor Thread
// -------------------------------------------------------------------------------------------------------------
static int HandleKeypress(ctrlm_ble_rcu_interface_t *metadata, struct input_event *event, const BleAddress &address)
{
    // These are 'seperator' events.  Ignore for now.
    if (event->code == 0) {
        return 0;
    }

    if (event->value >= 0 && event->value < 3) {
        // Send the keypress up to the BLE network
        ctrlm_hal_ble_IndKeypress_params_t params;
        params.ieee_address = address.toUInt64();
        params.event = *event;

        metadata->m_rcuKeypressSlots.invoke(&params);
    }
    return 0;
}


static int OpenKeyInputDevice(uint64_t ieee_address)
{
    int retry = 0;
    do {
        string keyInputBaseDir(KEY_INPUT_DEVICE_BASE_DIR);
        DIR *dir_p = opendir(keyInputBaseDir.c_str());
        if (NULL == dir_p) {
            return -1;
        }
        dirent *file_p;
        while ((file_p = readdir(dir_p)) != NULL) {
            if(strstr(file_p->d_name, KEY_INPUT_DEVICE_BASE_FILE) != NULL) {
                //this is one of the event devices, open it and see if it belongs to this MAC
                string keyInputFilename = keyInputBaseDir + file_p->d_name;
                int input_fd = open(keyInputFilename.c_str(), O_RDONLY|O_NONBLOCK);
                if (input_fd >= 0) {
                    struct libevdev *evdev = NULL;
                    int rc = libevdev_new_from_fd(input_fd, &evdev);
                    if (rc < 0) {
                        XLOGD_ERROR("Failed to init libevdev (%s)", strerror(-rc));   //on failure, rc is negative errno
                    } else if (evdev != NULL) {
                        XLOGD_DEBUG("Input device <%s> name: <%s> ID: bus %#x vendor %#x product %#x, phys = <%s>, unique = <%s>",
                                keyInputFilename.c_str(),
                                libevdev_get_name(evdev),libevdev_get_id_bustype(evdev),libevdev_get_id_vendor(evdev),
                                libevdev_get_id_product(evdev),libevdev_get_phys(evdev),libevdev_get_uniq(evdev));

                        uint64_t evdev_macaddr = ctrlm_convert_mac_string_to_long(libevdev_get_uniq(evdev));
                        if (evdev_macaddr == ieee_address) {
                            XLOGD_INFO("Input Dev Node (%s) for device (0x%llX) FOUND, returning file descriptor: <%d>", 
                                    keyInputFilename.c_str(), ieee_address, input_fd);

                            libevdev_free(evdev);
                            evdev = NULL;
                            closedir(dir_p);
                            return input_fd;
                        }
                    }
                    close(input_fd);
                    if (NULL != evdev) {
                        libevdev_free(evdev);
                        evdev = NULL;
                    }
                }
            }
        }
        closedir(dir_p);
        retry++;
    } while (retry < 1);

    return -1;
}

static void FindRcuInputDevices(ctrlm_ble_rcu_interface_t *metadata, 
                                std::map <BleAddress, int> &rcuKeypressFds, 
                                fd_set &rfds, 
                                int &nfds)
{   
    // Since the metadata map can be updated outside this thread, we need to lock with a mutex and then make a copy.
    // But only after getting the fd and device minor ID which need to be written to the metadata.
    // Otherwise, removing/adding an entry to the map while iterating will invalidate the iterator
    for (auto &rcu : rcuKeypressFds) {
        if (rcu.second < 0) {
            // We have an rcu in the metadata without an input device opened.
            int input_device_fd = -1;
            // Loop through the linux input device nodes to find the one corresponding to this ieee_address
            input_device_fd = OpenKeyInputDevice(rcu.first.toUInt64());

            if (input_device_fd < 0) {
                // XLOGD_DEBUG("Did not find input device for RCU 0x%llX", rcu.first);
            } else {
                // add this fd to select
                FD_SET(input_device_fd,  &rfds);
                nfds = MAX(nfds, input_device_fd);
                rcu.second = input_device_fd;

                // Get the device minor ID, which gets reported up to the app as deviceid
                struct stat sb;
                if (-1 == fstat(rcu.second, &sb)) {
                    int errsv = errno;
                    XLOGD_ERROR("fstat() failed: error = <%d>, <%s>", errsv, strerror(errsv));
                } else {
                    int deviceMinorId = minor(sb.st_rdev);

                    XLOGD_DEBUG("%s device minor ID = <%d>, reporting status change up to the network", 
                            rcu.first.toString().c_str(), deviceMinorId);

                    // send deviceid up to the network
                    ctrlm_hal_ble_RcuStatusData_t params;
                    params.property_updated = CTRLM_HAL_BLE_PROPERTY_DEVICE_ID;
                    params.rcu_data.ieee_address = rcu.first.toUInt64();
                    params.rcu_data.device_minor_id = deviceMinorId;
                    metadata->m_rcuStatusChangedSlots.invoke(&params);
                }
            }
        } else {
            FD_SET(rcu.second,  &rfds);
            nfds = MAX(nfds, rcu.second);
        }
    }
}


void *KeyMonitorThread(void *data)
{
    ctrlm_ble_rcu_interface_t *metadata = (ctrlm_ble_rcu_interface_t *)data;
    // std::map<BleAddress, RcuMetadata> rcu_metadata;

    std::map <BleAddress, int> rcuKeypressFds;

    struct input_event event;
    fd_set rfds;
    int nfds = -1;
    errno_t safec_rc = -1;
    bool running = true;
    char msg[CTRLM_BLE_KEY_MSG_QUEUE_MSG_SIZE_MAX];

    // Unblock the caller that launched this thread
    sem_post(&metadata->m_keyThreadSem);

    XLOGD_INFO("Enter main loop for new key monitor thread");
    do {
        // Needs to be reinitialized before each call to select() because select() will modify these variables
        FD_ZERO(&rfds);
        FD_SET(metadata->m_keyThreadMsgQ, &rfds);
        nfds = metadata->m_keyThreadMsgQ;
        FindRcuInputDevices(metadata, rcuKeypressFds, rfds, nfds);
        nfds++;

        int ret = select(nfds, &rfds, NULL, NULL, NULL);
        if (ret < 0) {
            int errsv = errno;
            XLOGD_DEBUG("select() failed: error = <%d>, <%s>", errsv, strerror(errsv));
            continue;
        }

        if(FD_ISSET(metadata->m_keyThreadMsgQ, &rfds)) {
            ssize_t bytes_read = xr_mq_pop(metadata->m_keyThreadMsgQ, msg, sizeof(msg));
            if(bytes_read <= 0) {
                XLOGD_ERROR("mq_receive failed, rc <%d>", bytes_read);
            } else {
                ctrlm_ble_key_queue_msg_header_t *hdr = (ctrlm_ble_key_queue_msg_header_t *) msg;
                switch(hdr->type) {
                    case CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEVICE_ADDED: {
                        ctrlm_ble_key_queue_device_changed_msg_t *device_changed_msg = (ctrlm_ble_key_queue_device_changed_msg_t *) msg;
                        XLOGD_INFO("message type CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEVICE_ADDED");

                        if (rcuKeypressFds.end() == rcuKeypressFds.find(device_changed_msg->address)) {
                            // device not found in our map, create an entry and init the fd to -1
                            rcuKeypressFds[device_changed_msg->address] = -1;
                        } else {
                            if (rcuKeypressFds[device_changed_msg->address] >= 0) {
                                XLOGD_TELEMETRY("RCU <%s> RE-CONNECTED, closing key input device so key monitor thread can reopen...", 
                                        device_changed_msg->address.toString().c_str());
                                close(rcuKeypressFds[device_changed_msg->address]);
                                rcuKeypressFds[device_changed_msg->address] = -1;
                            }
                        }
                        break;
                    }
                    case CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEVICE_REMOVED: {
                        ctrlm_ble_key_queue_device_changed_msg_t *device_changed_msg = (ctrlm_ble_key_queue_device_changed_msg_t *) msg;
                        XLOGD_INFO("message type CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEVICE_REMOVED");

                        if (rcuKeypressFds.end() != rcuKeypressFds.find(device_changed_msg->address)) {
                            if (rcuKeypressFds[device_changed_msg->address] >= 0) {
                                close(rcuKeypressFds[device_changed_msg->address]);
                            }
                            rcuKeypressFds.erase(device_changed_msg->address);
                        }
                        break;
                    }
                    case CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEEPSLEEP_WAKEUP: {
                        XLOGD_INFO("message type CTRLM_BLE_KEY_QUEUE_MSG_TYPE_DEEPSLEEP_WAKEUP");

                        for (auto &rcu : rcuKeypressFds) {
                            if (rcu.second >= 0) {
                                XLOGD_INFO("Closing key input device for RCU <%s> so key monitor thread can reopen...", 
                                        rcu.first.toString().c_str());
                                close(rcu.second);
                                rcu.second = -1;
                            }
                        }
                        break;
                    }
                    case CTRLM_BLE_KEY_QUEUE_MSG_TYPE_TICKLE: {
                        XLOGD_DEBUG("message type CTRLM_BLE_KEY_QUEUE_MSG_TYPE_TICKLE");
                        ctrlm_ble_key_thread_monitor_msg_t *thread_monitor_msg = (ctrlm_ble_key_thread_monitor_msg_t *) msg;
                        *thread_monitor_msg->response = CTRLM_HAL_THREAD_MONITOR_RESPONSE_ALIVE;
                        break;
                    }
                    case CTRLM_BLE_KEY_QUEUE_MSG_TYPE_TERMINATE: {
                        XLOGD_INFO("message type CTRLM_BLE_KEY_QUEUE_MSG_TYPE_TERMINATE");
                        running = false;

                        for (auto &rcu : rcuKeypressFds) {
                            if (rcu.second >= 0) {
                                close(rcu.second);
                                rcu.second = -1;
                            }
                        }
                        break;
                    }
                    default: {
                        XLOGD_DEBUG("Unknown message type %u", hdr->type);
                        break;
                    }
                }
            }
        }

        // loop the rcu fds to see if any has data to read
        for (auto const &rcu : rcuKeypressFds) {
            if (rcu.second >= 0) {
                if (FD_ISSET(rcu.second, &rfds)) {
                    safec_rc = memset_s ((void*) &event, sizeof(event), 0, sizeof(event));
                    ERR_CHK(safec_rc);
                    ret = read(rcu.second, (void*)&event, sizeof(event));
                    if (ret < 0) {
                        // int errsv = errno;
                        // XLOGD_ERROR("Error reading event: error = <%d>, <%s>", errsv, strerror(errsv));
                    } else {
                        HandleKeypress(metadata, &event, rcu.first);
                    }
                }
            }
        }
    } while (running);

    if (running) {
        XLOGD_ERROR("key monitor thread broke out of loop without being told, an error occurred...");
    } else {
        XLOGD_INFO("thread told to exit...");
    }

    return NULL;
}
