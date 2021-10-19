#include <dbus/dbus.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef ASHA_SUPPORT_DBUS_H
#define ASHA_SUPPORT_DBUS_H

int dbus_init(int loop_fd);

dbus_bool_t add_watch(DBusWatch *watch, void *data);
void remove_watch(DBusWatch *watch, void *data);
void toggle_watch(DBusWatch *watch, void *data);
void free_watch(void *mem);
int dbus_connect_device(const char *bd_addr);

enum control_point_operation { START, STOP, STATUS };

struct audio_control_point {
  enum control_point_operation operation;
  union control_point_args {
    struct control_point_start {
      uint8_t codec;
      uint8_t audiotype;
      uint8_t volume;
      uint8_t otherstate;
    } start;
    struct control_point_status {
      uint8_t connected;
    } status;
  } args;
};

void dbus_write_audio_control_point(struct audio_control_point control_point);

void find_devices(uint32_t service_uuid_prefix);

#else
#endif
