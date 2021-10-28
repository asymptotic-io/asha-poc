#include "bluez.h"
#include "log.h"
#include "loop.h"
#include "types.h"
#include <stdbool.h>
#include <sys/types.h>

bool stream_init(char *bd_addr_raw, struct ha_device *device) {

  device->sequence_counter = 0;
  device->socket = l2cap_connect(bd_addr_raw, device->le_psm);
  log_info("socket: %d\n", device->socket);
  loop_add(device->socket);
  return true;
}

bool stream_act(struct ha_device *device) {
  if (device->connection_status != CONNECTED) {
    return false;
  }

  while (true) {
  }

  return true;
}
