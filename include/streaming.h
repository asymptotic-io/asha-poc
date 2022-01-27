#include "types.h"
#include <stdbool.h>

#ifndef ASHA_SUPPORT_STREAMING_H
#define ASHA_SUPPORT_STREAMING_H

bool stream_init(int loop_fd, char *bd_addr_raw, struct ha_device *device,
                 char *sound_path);
bool stream_run(struct ha_device *device);

#endif
