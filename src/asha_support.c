#include "bluez.h"
#include "dbus.h"
#include "log.h"
#include "loop.h"
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXECUTABLE_NAME "asha-support"

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
  uint16_t psm = 0;

  if (dbus_err != 0) {
    log_info("Failed to initialize dbus: Error %d\n", dbus_err);
    return 1;
  }

  log_info("Connecting to %s\n", bd_addr);

  sleep(1);
  dbus_connect_device(bd_addr);
  log_info("Connected to %s\n", bd_addr);

  psm = dbus_read_psm(bd_addr);
  dbus_read_characteristic(bd_addr);
  if (psm == 0) {
    log_info("Unable to read PSM: %u\n", psm);
    return 1;
  }

  log_info("Received PSM: %u\n", psm);

  log_info("L2CAP: Connecting to %s:%u\n", bd_addr, psm);
  l2cap_connect(bd_addr, psm);

  while (1)
    loop_iterate();

  return 0;
}
