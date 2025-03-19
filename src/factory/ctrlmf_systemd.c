#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctrlm_log.h>
#include <rdkx_logger.h>
#include <dbus/dbus.h>
#include <ctrlm_fta_lib.h>
#include <ctrlmf_utils.h>

#define SYSTEMD_DESTINATION ("org.freedesktop.systemd1")
#define SYSTEMD_PATH        ("/org/freedesktop/systemd1")
#define SYSTEMD_IFACE_MGR   ("org.freedesktop.systemd1.Manager")
#define SYSTEMD_IFACE_UNIT  ("org.freedesktop.systemd1.Unit")
#define SYSTEMD_IFACE_PROP  ("org.freedesktop.DBus.Properties")

static DBusMessage *ctrlmf_systemd_service_get_unit(DBusConnection *connection, const char *unit_name);
static DBusMessage *ctrlmf_systemd_unit_property_get(DBusConnection *connection, const char *unit_path, const char *property);
static bool         ctrlmf_systemd_unit_property_equal(DBusConnection *connection, const char *unit_path, const char *property, const char *value);

bool ctrlmf_systemd_service_exec(const char *unit_name, const char *method) {
   bool success = false;
   DBusError error;
   dbus_error_init(&error);

   DBusConnection *connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
   if(connection == NULL) {
      XLOGD_ERROR("Failed to connect to the D-BUS daemon <%s>", error.message);
   } else {
      DBusMessage *message = dbus_message_new_method_call(SYSTEMD_DESTINATION, SYSTEMD_PATH, SYSTEMD_IFACE_MGR, method);
      if(message == NULL) {
         XLOGD_ERROR("Failed to create message for systemd1.Manager");
      } else {
         const char *mode = "replace"; // needed for StartUnit and RestartUnit. Does nothing for StopUnit
         if(!dbus_message_append_args(message, DBUS_TYPE_STRING, &unit_name, DBUS_TYPE_STRING, &mode, DBUS_TYPE_INVALID)) {
            XLOGD_ERROR("Failed to append message for systemd1.Manager");
         } else {
            DBusMessage *reply = dbus_connection_send_with_reply_and_block(connection, message, DBUS_TIMEOUT_USE_DEFAULT, &error);
            if(reply == NULL) {
               XLOGD_ERROR("Failed to send message <%s>", error.message);
            } else {
               dbus_message_unref(reply);
               success = true;
            }
         }
         dbus_message_unref(message);
      }
      dbus_connection_flush(connection);
      dbus_connection_unref(connection);
   }
   dbus_error_free(&error);
   return(success);
}

bool ctrlmf_systemd_is_service_active(const char *unit_name) {
   DBusError error;
   dbus_error_init(&error);

   DBusConnection *connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
   if(connection == NULL) {
      XLOGD_ERROR("Failed to connect to the D-BUS daemon <%s>", error.message);
      dbus_error_free(&error);
      return(false);
   }
   bool result = false;
   DBusMessage *reply = ctrlmf_systemd_service_get_unit(connection, unit_name);

   if(reply == NULL) {
      XLOGD_ERROR("Failed to get unit for <%s>", unit_name);
   } else {
      DBusMessageIter root_iter;
      dbus_message_iter_init(reply, &root_iter);

      if(DBUS_TYPE_OBJECT_PATH != dbus_message_iter_get_arg_type(&root_iter)) {
         XLOGD_INFO("Failed to get object path");
      } else {
         char *unit_path = NULL;
         dbus_message_iter_get_basic(&root_iter, &unit_path);

         result = ctrlmf_systemd_unit_property_equal(connection, unit_path, "ActiveState", "active");

         if(result) {
            result = ctrlmf_systemd_unit_property_equal(connection, unit_path, "SubState", "running");
         }
      }

      dbus_message_unref(reply);
   }

   dbus_connection_flush(connection);
   dbus_connection_unref(connection);

   dbus_error_free(&error);
   return(result);
}

DBusMessage *ctrlmf_systemd_service_get_unit(DBusConnection *connection, const char *unit_name) {
   DBusMessage *message = dbus_message_new_method_call(SYSTEMD_DESTINATION, SYSTEMD_PATH, SYSTEMD_IFACE_MGR, "GetUnit");
   if(message == NULL) {
      XLOGD_ERROR("Failed to create message");
      return(NULL);
   }

   if(!dbus_message_append_args(message, DBUS_TYPE_STRING, &unit_name, DBUS_TYPE_INVALID)) {
      XLOGD_ERROR("Failed to append message");
      dbus_message_unref(message);
      return(NULL);
   }
   DBusError error;
   dbus_error_init(&error);

   DBusMessage *reply = dbus_connection_send_with_reply_and_block(connection, message, DBUS_TIMEOUT_USE_DEFAULT, &error);
   if(reply == NULL) {
      XLOGD_ERROR("Failed to send message <%s>", error.message);
   }

   dbus_message_unref(message);
   dbus_error_free(&error);
   return(reply);
}

DBusMessage *ctrlmf_systemd_unit_property_get(DBusConnection *connection, const char *unit_path, const char *property) {
   DBusMessage *message = dbus_message_new_method_call(SYSTEMD_DESTINATION, unit_path, SYSTEMD_IFACE_PROP, "Get");
   if(message == NULL) {
      XLOGD_ERROR("Failed to create message for unit <%s>", unit_path);
      return(NULL);
   }

   char *interface_name = SYSTEMD_IFACE_UNIT;
   if(!dbus_message_append_args(message, DBUS_TYPE_STRING, &interface_name, DBUS_TYPE_STRING, &property, DBUS_TYPE_INVALID)) {
      XLOGD_ERROR("Failed to append message for unit <%s>", unit_path);
      dbus_message_unref(message);
      return(NULL);
   }
   DBusError error;
   dbus_error_init(&error);

   DBusMessage *reply = dbus_connection_send_with_reply_and_block(connection, message, DBUS_TIMEOUT_USE_DEFAULT, &error);
   if(reply == NULL) {
      XLOGD_ERROR("Failed to send message <%s> interface <%s> property <%s>", error.message, interface_name, property);
   }

   dbus_message_unref(message);
   dbus_error_free(&error);
   return(reply);
}

bool ctrlmf_systemd_unit_property_equal(DBusConnection *connection, const char *unit_path, const char *property, const char *value) {
   bool result = false;
   DBusMessage *prop = ctrlmf_systemd_unit_property_get(connection, unit_path, property);

   if(prop == NULL) {
      XLOGD_ERROR("Failed to get property - unit path <%s> property <%s>", unit_path, property);
      return(result);
   }

   DBusMessageIter iter;
   dbus_message_iter_init(prop, &iter);

   int type = dbus_message_iter_get_arg_type(&iter);
   if(DBUS_TYPE_VARIANT != type) {
      XLOGD_ERROR("Invalid reply - unit path <%s> property <%s> type <%d>", unit_path, property, type);
   } else {
      DBusMessageIter sub_iter;
      dbus_message_iter_recurse(&iter, &sub_iter);

      type = dbus_message_iter_get_arg_type(&sub_iter);
      if(DBUS_TYPE_STRING != type) {
         XLOGD_ERROR("Invalid reply - unit path <%s> property <%s> sub type <%d>", unit_path, property, type);
      } else {
         char* str = NULL;
         dbus_message_iter_get_basic(&sub_iter, &str);

         if(str == NULL || strcmp(str, value) != 0) {
            XLOGD_ERROR("value not equal - unit path <%s> property <%s> value <%s> <%s>", unit_path, property, value, str ? str : "NULL");
         } else {
            result = true;
         }
      }
   }
   dbus_message_unref(prop);
   return(result);
}
