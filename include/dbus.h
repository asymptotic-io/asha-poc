#include <dbus/dbus.h>
#include <stdbool.h>

int dbus_init(int loop_fd);

bool add_watch(DBusWatch *watch, void *data);
void remove_watch(DBusWatch *watch, void *data);
void toggle_watch(DBusWatch *watch, void *data);
void free_watch(void *mem);
int dbus_connect_device(const char *bd_addr);
