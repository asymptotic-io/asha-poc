#include "dbus.h"
#include "log.h"
#include "loop.h"
#include "types.h"
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
  event.data.fd = 0; // Unsure as to whether this needs to even be anything

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

void debug_read_only_properties(
    struct asha_read_only_properties *ro_properties) {
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
  log_info("Device Capabilities: %u\n", ro_properties->device_capabilities);
  log_info("HiSync ID: %lu\n", ro_properties->hi_sync_id);
  log_info("Feature map: %u\n", ro_properties->feature_map);
  log_info("Render delay: %u\n", ro_properties->render_delay);
  log_info("Reserved: %u\n", ro_properties->reserved);
  log_info("Supported Codecs: %u\n", ro_properties->supported_codecs);
}

void dbus_read_characteristic(char *bd_addr) {
  char *bus_name = "org.bluez";
  char path[100] = "/org/bluez/hci0/dev_";
  strncat(path, bd_addr, 17);
  strcat(path, "/service0021/char0022");

  char *interface = "org.bluez.GattCharacteristic1";

  struct asha_read_only_properties *ro_properties;

  DBusMessage *m = dbus_message_new_method_call(bus_name, path, interface,
                                                "ReadValue"),
              *reply;
  DBusMessageIter iter;
  int size;

  dbus_error_init(&err);
  dbus_message_iter_init_append(m, &iter);
  create_read_characteristic_container(&iter);

  reply = dbus_connection_send_with_reply_and_block(conn, m, 500, &err);
  if (dbus_error_is_set(&err)) {
    log_info("Error: failed to read ReadOnlyProperties\n");
    return;
  }

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
    log_info("Error: failed to read ReadOnlyProperties reply\n");
    return;
  }

  dbus_message_get_args(reply, &err, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
                        &ro_properties, &size, DBUS_TYPE_INVALID);
  log_info("Size: %d\n", size);

  if (dbus_error_is_set(&err)) {
    log_info("Error: failed to parse ReadOnlyProperties. %s (%s)\n", err.name,
             err.message);
    return;
  }

  debug_read_only_properties(ro_properties);
  dbus_message_unref(reply);

  log_info("Sent\n");
}

uint16_t dbus_read_psm(char *bd_addr) {
  // Un-free'd strings abound
  char *bus_name = "org.bluez";
  char path[100] = "/org/bluez/hci0/dev_";
  strncat(path, bd_addr, 17);
  strcat(path, "/service0021/char002f");
  char *interface = "org.bluez.GattCharacteristic1";
  uint16_t *spsm;

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
    return 0;
  }

  if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
    log_info("Error: failed to read ReadOnlyProperties reply\n");
    return 0;
  }

  dbus_message_get_args(reply, &err, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &spsm,
                        &size, DBUS_TYPE_INVALID);
  log_info("Size: %d\n", size);

  if (dbus_error_is_set(&err)) {
    log_info("Error: failed to parse PSM. %s (%s)\n", err.name, err.message);
    return 0;
  }

  dbus_message_unref(m);

  log_info("Sent\n");
  return *spsm;
}
