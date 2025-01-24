##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

set(project "ctrlm-main")

target_sources(controlMgr PRIVATE
   ctrlm_main.cpp
   ctrlm_main_iarm.cpp
   ctrlm_utils.cpp
   ctrlm_network.cpp
   ctrlm_controller.cpp
   ctrlm_ir_controller.cpp
   ctrlm_rcu.cpp
   ctrlm_rcu_iarm.cpp
   attributes/ctrlm_attr.cpp
   attributes/ctrlm_attr_general.cpp
   attributes/ctrlm_attr_voice.cpp
   attributes/ctrlm_version.cpp
   ctrlm_config_default.c
   config/ctrlm_config.cpp
   config/ctrlm_config_attr.cpp
   config/ctrlm_config_types.cpp
   ctrlm_controller.cpp
   ctrlm_device_update.cpp
   ctrlm_device_update_iarm.cpp
   ctrlm_ir_controller.cpp
   ctrlm_main.cpp
   ctrlm_main_iarm.cpp
   ctrlm_network.cpp
   ctrlm_rcu.cpp
   ctrlm_rcu_iarm.cpp
   ctrlm_recovery.cpp
   ctrlm_tr181.cpp
   ctrlm_utils.cpp
   ctrlm_validation.cpp
   ctrlm_vendor_network_factory.cpp
   database/ctrlm_database.cpp
   database/ctrlm_db_attr.cpp
   database/ctrlm_db_types.cpp
   input_event/ctrlm_input_event_writer.cpp
   ipc/ctrlm_ipc_iarm.cpp
   ipc/ctrlm_rcp_ipc_event.cpp
   irdb/ctrlm_irdb.cpp
   irdb/ctrlm_irdb_stub.cpp
   irdb/ipc/ctrlm_irdb_ipc_iarm_thunder.cpp
   json_config.cpp
   network/ctrlm_ir_rf_db.cpp
   network/ctrlm_ir_rf_db_entry.cpp
   rfc/ctrlm_rfc.cpp
   rfc/ctrlm_rfc_attr.cpp
   shared_memory/ctrlm_shared_memory.cpp
   voice/ctrlm_voice_obj.cpp
   voice/ctrlm_voice_obj_generic.cpp
   voice/endpoints/ctrlm_voice_endpoint.cpp
   voice/endpoints/ctrlm_voice_endpoint_ws_nextgen.cpp
   voice/endpoints/ctrlm_voice_endpoint_ws_nsp.cpp
   voice/ipc/ctrlm_voice_ipc_iarm_all.cpp
   voice/ipc/ctrlm_voice_ipc_iarm_legacy.cpp
   voice/ipc/ctrlm_voice_ipc_request_type.c
)

if(BLE_ENABLED)
   target_sources(controlMgr PRIVATE
      ble/ctrlm_ble_controller.cpp 
      ble/ctrlm_ble_iarm.cpp 
      ble/ctrlm_ble_network.cpp 
      ble/ctrlm_ble_utils.cpp 
      ble/ctrlm_ble_controller_attr_version.cpp 
      ble/ctrlm_ble_rcu_interface.cpp 
      ble/hal/blercu/blercucontroller.cpp 
      ble/hal/blercu/blercuerror.cpp 
      ble/hal/blercu/blercupairingstatemachine.cpp 
      ble/hal/blercu/blercuscannerstatemachine.cpp 
      ble/hal/blercu/btrmgradapter.cpp 
      ble/hal/blercu/bleservices/blercuservicesfactory.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_audiopipe.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_audioservice.cpp
      ble/hal/blercu/bleservices/gatt/gatt_audioservice_rdk.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_batteryservice.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_deviceinfoservice.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_findmeservice.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_infraredservice.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_infraredsignal.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_remotecontrolservice.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_services.cpp 
      ble/hal/blercu/bleservices/gatt/gatt_upgradeservice.cpp 
      ble/hal/blercu/bluez/blegattcharacteristic.cpp 
      ble/hal/blercu/bluez/blegattdescriptor.cpp 
      ble/hal/blercu/bluez/blegattnotifypipe.cpp 
      ble/hal/blercu/bluez/blegattprofile.cpp 
      ble/hal/blercu/bluez/blegattservice.cpp 
      ble/hal/blercu/bluez/blercuadapter.cpp 
      ble/hal/blercu/bluez/blercudevice.cpp 
      ble/hal/configsettings/configmodelsettings.cpp 
      ble/hal/configsettings/configsettings.cpp 
      ble/hal/dbus/dbusabstractinterface.cpp 
      ble/hal/dbus/dbusobjectmanager.cpp 
      ble/hal/dbus/dbusservicewatcher.cpp 
      ble/hal/dbus/dbusutils.cpp 
      ble/hal/dbus/dbusvariant.cpp 
      ble/hal/dbus/gdbusabstractinterface.cpp 
      ble/hal/utils/bleaddress.cpp 
      ble/hal/utils/bleconnectionparameters.cpp 
      ble/hal/utils/bleuuid.cpp 
      ble/hal/utils/crc32.cpp 
      ble/hal/utils/filedescriptor.cpp 
      ble/hal/utils/audioformat.cpp 
      ble/hal/utils/futureaggregator.cpp 
      ble/hal/utils/fwimagefile.cpp 
      ble/hal/utils/hcisocket.cpp 
      ble/hal/utils/statemachine.cpp
      ctrlm_config_default.c
   )
endif()

if(RF4CE_ENABLED)
   target_sources(controlMgr PRIVATE
      rf4ce/controller/attributes/ctrlm_rf4ce_controller_attr_battery.cpp
      rf4ce/controller/attributes/ctrlm_rf4ce_controller_attr_general.cpp
      rf4ce/controller/attributes/ctrlm_rf4ce_controller_attr_irdb.cpp
      rf4ce/controller/attributes/ctrlm_rf4ce_controller_attr_version.cpp
      rf4ce/controller/attributes/ctrlm_rf4ce_controller_attr_voice.cpp
      rf4ce/ctrlm_rf4ce_battery.cpp
      rf4ce/ctrlm_rf4ce_controller.cpp
      rf4ce/ctrlm_rf4ce_device_update.cpp
      rf4ce/ctrlm_rf4ce_discovery.cpp
      rf4ce/ctrlm_rf4ce_indication.cpp
      rf4ce/ctrlm_rf4ce_network.cpp
      rf4ce/ctrlm_rf4ce_pairing.cpp
      rf4ce/ctrlm_rf4ce_rcu.cpp
      rf4ce/ctrlm_rf4ce_rib.cpp
      rf4ce/ctrlm_rf4ce_utils.cpp
      rf4ce/ctrlm_rf4ce_validation.cpp
      rf4ce/network/attributes/ctrlm_rf4ce_network_attr_config.cpp
      rf4ce/rib/ctrlm_rf4ce_rib.cpp
      rf4ce/rib/ctrlm_rf4ce_rib_attr.cpp
   )

   if(RF4CE_PACKET_ANALYSIS)
      target_sources(controlMgr PRIVATE
         ctrlm_rf4ce_voice_packet_analysis.cpp
      )
   else()
      target_sources(controlMgr PRIVATE
         stubs/stubs_voice_packet_analysis.cpp
      )
   endif()
endif()

if(THUNDER)
   target_sources(controlMgr PRIVATE
      thunder/ctrlm_thunder_controller.cpp
      thunder/ctrlm_thunder_plugin.cpp
      thunder/ctrlm_thunder_plugin_system_audio_player.cpp
      thunder/helpers/ctrlm_thunder_helpers.cpp
      thunder/plugins/ctrlm_thunder_plugin_cec.cpp
      thunder/plugins/ctrlm_thunder_plugin_cec_sink.cpp
      thunder/plugins/ctrlm_thunder_plugin_cec_source.cpp
      thunder/plugins/ctrlm_thunder_plugin_display_settings.cpp
      thunder/plugins/ctrlm_thunder_plugin_device_info.cpp 
      thunder/plugins/ctrlm_thunder_plugin_hdmi_input.cpp
      thunder/plugins/ctrlm_thunder_plugin_system.cpp
      ipc/ctrlm_rcp_ipc_iarm_thunder.cpp
      voice/ipc/ctrlm_voice_ipc_iarm_thunder.cpp
   )
endif()

if(NOT IRDB)
   target_sources(controlMgr PRIVATE
      irdb/ctrlm_irdb_factory.cpp
   )
endif()

if(TELEMETRY_SUPPORT)
   target_sources(controlMgr PRIVATE
      telemetry/ctrlm_telemetry.cpp
      telemetry/ctrlm_telemetry_event.cpp
      voice/telemetry/ctrlm_voice_telemetry_events.cpp
   )
endif()

if(XRSR_SDT)
   target_sources(controlMgr PRIVATE
      voice/endpoints/ctrlm_voice_endpoint_sdt.cpp
   )
endif()

if(XRSR_HTTP)
   target_sources(controlMgr PRIVATE
      voice/endpoints/ctrlm_voice_endpoint_http.cpp
   )
endif()
