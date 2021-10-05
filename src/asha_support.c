#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "dbus.h"
#include "log.h"

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

  dbus_init();
  log_info("Connecting to %s\n", bd_addr);

  return 0;
}
