#include "types.h"
#include <stdbool.h>

#ifndef ASHA_SUPPORT_STREAMING_H
#define ASHA_SUPPORT_STREAMING_H

bool stream_init(char *bd_addr_raw, struct ha_device *device);
bool stream_act(struct ha_device *device);

#endif
