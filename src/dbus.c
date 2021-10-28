#include "dbus.h"
#include "log.h"
#include "loop.h"
#include "types.h"
#include <assert.h>
#include <byteswap.h>
#include <dbus/dbus.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

static DBusError err;
static DBusConnection *conn;

struct watch_data {
  bool active;
};

void free_watch(void *mem) {
  log_info("dbus: Freeing some watch memory");
  free(mem);
}

void handle_dbus_event(void *p) { log_info("Received a DBus event\n"); }

dbus_bool_t add_watch(DBusWatch *watch, void *data) {
  struct loop_data *l = data;
  struct epoll_event event = {0};
  struct watch_data *wd = dbus_watch_get_data(watch);

  if ((wd != NULL) && wd->active) {
    log_info("dbus: Avoided adding a watch that was already active");
    return TRUE;
  }
  struct watch_data *new_wd = malloc(sizeof(struct watch_data));
  new_wd->active = TRUE;

  event.events = EPOLLIN;
  event.data.ptr = &data; // Unsure as to whether this needs to even be anything

  epoll_ctl(l->loop_fd, EPOLL_CTL_ADD, dbus_watch_get_unix_fd(watch), &event);
  dbus_watch_set_data(watch, (void *)new_wd, free_watch);

  return TRUE;
}

void remove_watch(DBusWatch *watch, void *data) {
  struct loop_data *l = data;
  struct epoll_event event;

  struct watch_data *wd = dbus_watch_get_data(watch);

  if ((wd == NULL) || !wd->active) {
    log_info("dbus: Avoided removing a watch that was not active");
    return;
  }

  event.events = EPOLLIN;
  event.data.fd = 0; // Unsure as to whether this needs to even be anything
  epoll_ctl(l->loop_fd, EPOLL_CTL_DEL, dbus_watch_get_unix_fd(watch), &event);
}

void toggle_watch(DBusWatch *watch, void *data) {
  struct loop_data *l = data;
  struct epoll_event event = {0};

  if (dbus_watch_get_enabled(watch)) {
    struct watch_data *wd = (struct watch_data *)dbus_watch_get_data(watch);
    wd->active = TRUE;

    event.events = EPOLLIN;
    event.data.fd = 0; // Unsure as to whether this needs to even be anything

    epoll_ctl(l->loop_fd, EPOLL_CTL_DEL, dbus_watch_get_unix_fd(watch), &event);
    log_info("dbus: Watch toggled on");
  } else {
    log_info("dbus: Watch toggled off");
  }
}

int dbus_init(int loop_fd) {
  struct loop_data *l = malloc(sizeof(struct loop_data));
  l->loop_fd = loop_fd;
  dbus_error_init(&err);

  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

  if (dbus_error_is_set(&err)) {
    log_info("DBus Connection Error (%s)\n", err.message);
    dbus_error_free(&err);
  }

  if (NULL == conn) {
    log_info("Missing DBus Connection\n");
    return 1;
  }

  dbus_connection_set_watch_functions(conn, add_watch, remove_watch,
                                      toggle_watch, l, free_watch);

  return 0;
}

int dbus_connect_device(const char *bd_addr) {
  char path[100];
  snprintf(path, 100, "/org/bluez/hci0/dev_%s", bd_addr);
  log_debug("Connecting to path: %s\n", path);
  DBusMessage *message = dbus_message_new_method_call(
      "org.bluez", path, "org.bluez.Device1", "Connect");
  dbus_connection_send(conn, message, NULL);

  return 0;
}

static void create_read_characteristic_container(DBusMessageIter *iter) {
  DBusMessageIter dict;

  dbus_message_iter_open_container(
      iter, DBUS_TYPE_ARRAY,
      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
          DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
      &dict);

  dbus_message_iter_close_container(iter, &dict);
}

static void create_write_characteristic_container(DBusMessageIter *iter,
                                                  uint8_t *data,
                                                  unsigned int len) {
  DBusMessageIter dict, data_iter;

  dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                                   DBUS_TYPE_BYTE_AS_STRING, &data_iter);
  for (int i = 0; i < len; i++)
    dbus_message_iter_append_basic(&data_iter, DBUS_TYPE_BYTE, data + i);

  dbus_message_iter_close_container(iter, &data_iter);

  dbus_message_iter_open_container(
      iter, DBUS_TYPE_ARRAY,
      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
          DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
      &dict);

  dbus_message_iter_close_container(iter, &dict);
}

void debug_properties(struct ha_properties *ro_properties) {
  uint8_t ro_properties_bytes[17];
  memcpy(ro_properties_bytes, ro_properties, 17);
  log_info("Data (uint8): ");
  for (int i = 0; i < 17; i++)
    log_info("%u ", ro_properties_bytes[i]);
  log_info("\n");

  log_info("Data (hex): ");
  for (int i = 0; i < 17; i++)
    log_info("%X ", ro_properties_bytes[i]);
  log_info("\n");
  log_info("Version: %u\n", ro_properties->version);
  log_info("Device Capabilities(side): %u\n",
           ro_properties->device_capabilities.side);
  log_info("Device Capabilities(type): %u\n",
           ro_properties->device_capabilities.type);
  log_info("HiSync ID: %lu\n", ro_properties->hi_sync_id);
  log_info("Feature map: %u\n",
           ro_properties->feature_map.coc_streaming_supported);
  log_info("Render delay: %u\n", ro_properties->render_delay);
  log_info("Reserved: %u\n", ro_properties->reserved);
  log_info("Supported Codecs: %u\n", ro_properties->supported_codecs.g722);
}

static void dbus_read_ro_properties(char *path,
                                    struct ha_properties *properties,
                                    DBusError *error) {
  char *bus_name = "org.bluez";
  char *interface = "org.bluez.GattCharacteristic1";

  struct ha_properties *properties_ref;

  DBusMessage *m = dbus_message_new_method_call(bus_name, path, interface,
                                                "ReadValue"),
              *reply;
  DBusMessageIter iter;
  int size;

  dbus_error_init(error);
  dbus_message_iter_init_append(m, &iter);
  create_read_characteristic_container(&iter);

  reply = dbus_connection_send_with_reply_and_block(conn, m, 500, error);
  if (dbus_error_is_set(error)) {
    log_info("Error: failed to read ReadOnlyProperties\n");
    return;
  }

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
    log_info("Error: failed to read ReadOnlyProperties reply\n");
    return;
  }

  dbus_message_get_args(reply, error, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
                        &properties_ref, &size, DBUS_TYPE_INVALID);
  log_info("Size: %d\n", size);

  if (dbus_error_is_set(error)) {
    log_info("Error: failed to parse ReadOnlyProperties. %s (%s)\n",
             error->name, error->message);
    return;
  }

  memcpy(&properties, properties_ref, sizeof(struct ha_properties));
  debug_properties(properties_ref);
  dbus_message_unref(reply);

  log_info("Sent\n");
}

void dbus_read_psm(char *path, uint16_t *spsm, DBusError *error) {
  char *bus_name = "org.bluez";
  char *interface = "org.bluez.GattCharacteristic1";
  uint16_t *spsm_ref;

  DBusMessage *m = dbus_message_new_method_call(bus_name, path, interface,
                                                "ReadValue"),
              *reply;
  DBusMessageIter iter;
  int size = 0;

  dbus_error_init(&err);
  dbus_message_iter_init_append(m, &iter);
  create_read_characteristic_container(&iter);

  reply = dbus_connection_send_with_reply_and_block(conn, m, 500, &err);
  if (dbus_error_is_set(&err)) {
    log_info("Error: failed to read PSM\n");
    return;
  }

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
    log_info("Error: failed to read ReadOnlyProperties reply\n");
    return;
  }

  dbus_message_get_args(reply, &err, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &spsm_ref,
                        &size, DBUS_TYPE_INVALID);
  log_info("Size: %d\n", size);

  if (dbus_error_is_set(&err)) {
    log_info("Error: failed to parse PSM. %s (%s)\n", err.name, err.message);
    return;
  }

  dbus_message_unref(m);
  // FIXME: Do we need to free the reply here?

  log_info("Sent\n");
  memcpy(spsm, spsm_ref, sizeof(uint16_t));
}

#define BLUEZ_SERVICE_NAME "org.bluez"
#define OBJECTMANAGER_INTERFACE "org.freedesktop.DBus.ObjectManager"

#define READ_ONLY_PROPERTIES_UUID "6333651e-c481-4a3e-9169-7c902aad37bb"
#define AUDIO_CONTROL_POINT_UUID "f0d4de7e-4a88-476c-9d9f-1937b0996cc0"
#define AUDIO_STATUS_UUID "38663f1a-e711-4cac-b641-326b56404837"
#define VOLUME_UUID "00e4ca9e-ab14-41e4-8823-f9e70c7e91df"
#define LE_PSM_OUT_UUID "2d410339-82b6-42aa-b34e-e2e01df8cc1a"

struct ha_device *is_ha_service(DBusMessageIter *iter) {
  char *interface, *key, *value;
  bool has_ha_uuid = FALSE;
  struct ha_device *device = NULL;

  DBusMessageIter dict_entries, dict_entry, sub;

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING);
  // get interface. eg. 'org.bluez.GattService1'
  dbus_message_iter_get_basic(iter, &interface);

  if (0 != strcmp(interface, "org.bluez.GattService1"))
    return NULL;

  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_recurse(iter, &dict_entries);
  // recurse into a{sv}

  while (
      (dbus_message_iter_get_arg_type(&dict_entries) == DBUS_TYPE_DICT_ENTRY)) {
    dbus_message_iter_recurse(&dict_entries, &dict_entry);
    // into the {sv}

    assert(dbus_message_iter_get_arg_type(&dict_entry) == DBUS_TYPE_STRING);
    dbus_message_iter_get_basic(&dict_entry, &key);
    // get the key
    if (strcmp(key, "UUID") == 0) {
      assert(dbus_message_iter_has_next(&dict_entry));
      dbus_message_iter_next(&dict_entry);
      assert(dbus_message_iter_get_arg_type(&dict_entry) == DBUS_TYPE_VARIANT);
      dbus_message_iter_recurse(&dict_entry, &sub);
      assert(dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING);
      dbus_message_iter_get_basic(&sub, &value);

      if (strncmp(value, "0000fdf0-", 9) == 0)
        has_ha_uuid = TRUE;
    } else if (strcmp(key, "Device") == 0) {
      assert(dbus_message_iter_has_next(&dict_entry));
      dbus_message_iter_next(&dict_entry);
      assert(dbus_message_iter_get_arg_type(&dict_entry) == DBUS_TYPE_VARIANT);
      dbus_message_iter_recurse(&dict_entry, &sub);
      assert(dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_OBJECT_PATH);
      dbus_message_iter_get_basic(&sub, &value);

      device = malloc(sizeof(struct ha_device));
      // Presumably, this is true if we can discover the service
      device->connection_status = CONNECTED;
      bzero(device, sizeof(struct ha_device));
      strncpy(device->dbus_paths.device_path, value, 100);
    }

    if (!dbus_message_iter_has_next(&dict_entries))
      break;

    dbus_message_iter_next(&dict_entries);
  }

  if (!has_ha_uuid) {
    return NULL;
  } else {
    return device;
  }
}

DBusMessage *get_objects();

static void populate_characteristic(DBusMessageIter *iter,
                                    char *characteristic_path,
                                    struct ha_device **devices) {
  char *interface, *key, *value, *uuid;
  struct ha_device *device = NULL, **device_iter = NULL;

  DBusMessageIter dict_entries, dict_entry, sub;

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING);
  // get interface. eg. 'org.bluez.GattCharacteristic1'
  dbus_message_iter_get_basic(iter, &interface);

  if (0 != strcmp(interface, "org.bluez.GattCharacteristic1"))
    return;

  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_recurse(iter, &dict_entries);
  // recurse into a{sv}

  while (
      (dbus_message_iter_get_arg_type(&dict_entries) == DBUS_TYPE_DICT_ENTRY)) {
    dbus_message_iter_recurse(&dict_entries, &dict_entry);
    // into the {sv}

    assert(dbus_message_iter_get_arg_type(&dict_entry) == DBUS_TYPE_STRING);
    dbus_message_iter_get_basic(&dict_entry, &key);
    // get the key
    if (strcmp(key, "UUID") == 0) {
      assert(dbus_message_iter_has_next(&dict_entry));
      dbus_message_iter_next(&dict_entry);
      assert(dbus_message_iter_get_arg_type(&dict_entry) == DBUS_TYPE_VARIANT);
      dbus_message_iter_recurse(&dict_entry, &sub);
      assert(dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING);
      dbus_message_iter_get_basic(&sub, &uuid);
    } else if (strcmp(key, "Service") == 0) {
      assert(dbus_message_iter_has_next(&dict_entry));
      dbus_message_iter_next(&dict_entry);
      assert(dbus_message_iter_get_arg_type(&dict_entry) == DBUS_TYPE_VARIANT);
      dbus_message_iter_recurse(&dict_entry, &sub);
      assert(dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_OBJECT_PATH);
      dbus_message_iter_get_basic(&sub, &value);

      for (device_iter = devices; (*device_iter != NULL); device_iter++) {
        if (strncmp((*device_iter)->dbus_paths.service_path, value, 100) == 0)
          device = *device_iter;
      }
    }

    if (!dbus_message_iter_has_next(&dict_entries))
      break;

    dbus_message_iter_next(&dict_entries);
  }

  if (device == NULL)
    return;

  if (strcmp(uuid, LE_PSM_OUT_UUID) == 0) {
    strncpy(device->dbus_paths.le_psm_out_path, characteristic_path, 100);
    dbus_read_psm(characteristic_path, &device->le_psm, &err);
  } else if (strcmp(uuid, READ_ONLY_PROPERTIES_UUID) == 0) {
    strncpy(device->dbus_paths.read_only_properties_path, characteristic_path,
            100);
    dbus_read_ro_properties(characteristic_path, &device->properties, &err);
  } else if (strcmp(uuid, AUDIO_CONTROL_POINT_UUID) == 0) {
    strncpy(device->dbus_paths.audio_control_point_path, characteristic_path,
            100);
  } else if (strcmp(uuid, AUDIO_STATUS_UUID) == 0) {
    strncpy(device->dbus_paths.audio_status_path, characteristic_path, 100);
  } else if (strcmp(uuid, VOLUME_UUID) == 0) {
    strncpy(device->dbus_paths.volume_path, characteristic_path, 100);
  }
}

static void handle_object_if_characteristic(DBusMessageIter *iter,
                                            struct ha_device **devices) {
  // getting the oa{sa{sv}}
  DBusMessageIter dict_entries, dict_entry;
  char *object_path;

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_OBJECT_PATH);
  dbus_message_iter_get_basic(iter, &object_path);
  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_recurse(iter, &dict_entries);

  while (
      (dbus_message_iter_get_arg_type(&dict_entries) == DBUS_TYPE_DICT_ENTRY)) {
    dbus_message_iter_recurse(&dict_entries, &dict_entry);

    populate_characteristic(&dict_entry, object_path, devices);

    if (!dbus_message_iter_has_next(&dict_entries))
      break;
    dbus_message_iter_next(&dict_entries);
  }
}

static void
fetch_and_populate_characteristic_paths(struct ha_device **devices) {
  DBusMessage *reply = get_objects();

  DBusMessageIter iter, dict_entries, dict_entry;

  if (reply == NULL)
    return;

  dbus_message_iter_init(reply, &iter);
  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_recurse(&iter, &dict_entries);

  while (dbus_message_iter_get_arg_type(&dict_entries) ==
         DBUS_TYPE_DICT_ENTRY) {
    // iterate over each oa{sa{sv}}
    dbus_message_iter_recurse(&dict_entries, &dict_entry);
    handle_object_if_characteristic(&dict_entry, devices);

    if (!dbus_message_iter_has_next(&dict_entries))
      break;
    dbus_message_iter_next(&dict_entries);
  }
}

int add_if_ha_service(DBusMessageIter *iter, struct ha_device **devices) {
  DBusMessageIter dict_entries, dict_entry;
  char *object_path;
  struct ha_device *device, **device_iter = devices;
  // device_iter points to the first device pointer pointer

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_OBJECT_PATH);
  dbus_message_iter_get_basic(iter, &object_path);
  dbus_message_iter_next(iter);

  assert(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_recurse(iter, &dict_entries);

  while (
      (dbus_message_iter_get_arg_type(&dict_entries) == DBUS_TYPE_DICT_ENTRY)) {
    dbus_message_iter_recurse(&dict_entries, &dict_entry);

    device = is_ha_service(&dict_entry);
    if (device != NULL) {
      *device_iter = device;
      device_iter++;
      strncpy(device->dbus_paths.service_path, object_path, 100);
      log_info("Device: %s. %s\n", device->dbus_paths.device_path,
               device->dbus_paths.service_path);
    }

    if (!dbus_message_iter_has_next(&dict_entries))
      break;
    dbus_message_iter_next(&dict_entries);
  }
  return 0;
}

DBusMessage *get_objects() {
  DBusMessage *m = dbus_message_new_method_call(
      BLUEZ_SERVICE_NAME, "/", OBJECTMANAGER_INTERFACE, "GetManagedObjects");

  DBusMessage *reply;

  reply = dbus_connection_send_with_reply_and_block(conn, m, 500, &err);

  if (dbus_error_is_set(&err)) {
    log_info("Error: failed to list objects with ObjectManager\n");
    return NULL;
  }

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
    log_info("Error: failed to read GetManagedObjects reply\n");
    return NULL;
  }

  if (0 != strncmp(dbus_message_get_signature(reply), "a{oa{sa{sv}}}", 15)) {
    log_info("Unexpected response from ObjectManager\n");
    return NULL;
  }

  return reply;
}

void dbus_audio_control_point_start(struct ha_device *device) {
  log_info("Writing to %s\n", device->dbus_paths.audio_control_point_path);
  char *bus_name = "org.bluez";
  char *interface = "org.bluez.GattCharacteristic1";

  uint8_t data[] = {START, G722_16K_HZ, MEDIA, 0, OTHER_DISCONNECTED};

  DBusMessage *m = dbus_message_new_method_call(
                  bus_name, device->dbus_paths.audio_control_point_path,
                  interface, "WriteValue"),
              *reply;
  DBusMessageIter iter;

  dbus_error_init(&err);
  dbus_message_iter_init_append(m, &iter);
  create_write_characteristic_container(&iter, data, 5);

  reply = dbus_connection_send_with_reply_and_block(conn, m, 500, &err);
  if (dbus_error_is_set(&err)) {
    log_info("Error: failed to write AudioControlPoint: %s\n",
             dbus_message_get_error_name(reply));
    return;
  }

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
    log_info("Error: error writing AudioControlPoint\n");
    return;
  }

  log_info("AudioControlPoint Written\n");
}

struct ha_device **find_devices() {
  /*
   * Send a message to ObjectManager
   * GetManagedObjects
   * Read dict
   * Per entry:
   *   1. Scan dict for org.bluez.GattService1 or org.bluez.GattCharacteristic1
   *   2. When a Service (0x0000fdf0) is found, accumulate the key, i.e.
   * /org/bluez/hciX/dev_pp_qq_rr_ss/serviceNN under the device 'side'
   *   3. When a Characteristic of a service we have accumulated is found,
   * record the path under the device
   */
  DBusMessageIter iter, dict_entries, dict_entry;
  DBusMessage *reply = get_objects();
  struct ha_device **devices = calloc(sizeof(struct ha_device *), 20);
  bzero(devices, sizeof(struct ha_device *) * 20);

  if (reply == NULL)
    return devices;

  dbus_message_iter_init(reply, &iter);
  assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY);
  dbus_message_iter_recurse(&iter, &dict_entries);

  while (dbus_message_iter_get_arg_type(&dict_entries) ==
         DBUS_TYPE_DICT_ENTRY) {
    dbus_message_iter_recurse(&dict_entries, &dict_entry);

    add_if_ha_service(&dict_entry, devices);

    if (!dbus_message_iter_has_next(&dict_entries))
      break;
    dbus_message_iter_next(&dict_entries);
  }

  fetch_and_populate_characteristic_paths(devices);
  log_info("Done.\n");
  return devices;
}

/*
 * On load:
 *  1. Find all GattServices (0xfdf0)
 *  2. Find all GattCharacteristics for those services.
 *  3. Query the readable GattCharacteristics.
 *  4. Pair devices by hisync id
 *
 * Updates:
 *
 * When a new 0xfdf0 service is added
 *    1. Check if hisync id matches existing device, pair them together
 *    2. Else, create new device
 * When a 0xfdf0 service is removed
 *    1. Check if other device in pair exists, change 'stream' to mux into
 * single channel
 *    2. Else, remove 'stream'
 *
 */
