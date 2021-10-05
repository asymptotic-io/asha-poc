#include "log.h"
#include "loop.h"
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
  struct epoll_event event;
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
  struct epoll_event event;

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
                                      toggle_watch, &l, free_watch);

  return 0;
}

int dbus_connect_device(const char *bd_addr) {
  char path[100];
  snprintf(path, 100, "/org/bluez/hci0/dev_%s", bd_addr);
  log_debug("Connecting to path: %s\n", path);
  DBusMessage *message = dbus_message_new_method_call("org.bluez", path, "org.bluez.Device1",
      "Connect");
  dbus_connection_send(conn, message, NULL);

  return 0;
}
