#include "dbus.h"
#include "types.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HA_DEVICES 20
struct ha_pairs *device_manager_init() {
  struct ha_pairs *pairs = malloc(sizeof(struct ha_pairs));
  return pairs;
}

static void load_dbus_paths(struct ha_device *path) {
  // Search through OManager
  // for characteristics of service fdf0
}

bool add_device(struct ha_pairs *pairs, struct ha_device *device) {
  // FIXME
  // assuming the hi sync id matched device is not already added
  // and that we can just add this device without any further validations

  if (pairs->length == MAX_HA_PAIRS)
    return false;

  struct ha_device *slot = &pairs->pairs[pairs->length].left;

  if (!is_left(device->properties)) {
    slot = &pairs->pairs[pairs->length].right;
  }

  memcpy(slot, device, sizeof(struct ha_device));
  pairs->length++;

  return true;
  /*
   * If is left:
   *   If matching right is found:
   *        If it already has a left:
   *            Log error, fail
   *        else:
   *            Add as left in the same ha_device
   *    else:
   *        Add as left in a new ha_device
   * else:
   *   Same as above, but right
   */
}
