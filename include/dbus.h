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

struct ha_device **find_devices();

enum AudioControlPoint { START = 0x01, STOP = 0x02, STATUS = 0x03 };
enum Codec { G722_16K_HZ = 0x01 };
enum AudioType { UNKNOWN = 0, RINGTONE = 1, PHONECALL = 2, MEDIA = 3 };
enum OtherState { OTHER_DISCONNECTED = 0, OTHER_CONNECTED = 1 };

void dbus_audio_control_point_start(struct ha_device *device);

#else
#endif
