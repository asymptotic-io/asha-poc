#include "bluez.h"
#include "types.h"
#include <stdbool.h>
#include <sys/types.h>

bool stream_init(struct ha_device *device) {
  if (device->connection_status != CONNECTED) {
    return false;
  }

  device->sequence_counter = 0;
  return true;
}
