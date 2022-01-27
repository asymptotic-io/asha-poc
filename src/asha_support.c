#include "bluez.h"
#include "dbus.h"
#include "log.h"
#include "loop.h"
#include "streaming.h"
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>
#include <unistd.h>

#define EXECUTABLE_NAME "asha-support"
#define SOUND_FILE "sounds/ikea.g722"

static void print_help_message() {
  log_info("Usage: %s [BD_ADDR]\n", EXECUTABLE_NAME);
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    print_help_message();
    return 0;
  }

  char bd_addr[20];
  strncpy(bd_addr, argv[1], 20);

  int loop_fd = loop_init();
  int dbus_err = dbus_init(loop_fd);

  if (dbus_err != 0) {
    log_info("Failed to initialize dbus: Error %d\n", dbus_err);
    return 1;
  }

  log_info("Connecting to %s\n", bd_addr);

  sleep(1);
  dbus_connect_device(bd_addr);
  log_info("Connected to %s\n", bd_addr);
  sleep(1);

  struct ha_device **devices = find_devices();
  struct ha_device *device = *devices;

  // Assuming we want to start with the first device for now
  // if it exists
  if (device == NULL) {
    log_info("Could not find the device\n");
    return 1;
  }

  stream_init(loop_fd, bd_addr, device, SOUND_FILE);
  dbus_audio_control_point_start(*devices);
  sleep(1);
  stream_run(device);
  sleep(3);

  while (1) {
    sleep(1);
    loop_iterate(loop_fd);
  }

  return 0;
}
