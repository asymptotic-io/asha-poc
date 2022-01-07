#include "bluez.h"
#include "log.h"
#include "loop.h"
#include "types.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

void handle_stream_event(struct loop_data *loop_data) {
  void *buf = malloc(100);
  bzero(buf, 100);

  log_info("Received a stream event\n");
}

bool stream_init(char *bd_addr_raw, struct ha_device *device) {
  device->sequence_counter = 0;
  device->socket = l2cap_connect(bd_addr_raw, device->le_psm);
  device->source = open("/tmp/sample.g722", O_RDONLY);
  device->firstrun = 1;

  // 1 byte for seq_counter and 2 for sdulen in the first sdu
  device->sdulen = 167;
  device->buffer = malloc(device->sdulen);
  bzero(device->buffer, device->sdulen);

  log_info("socket: %d\n", device->socket);

  loop_add(device->socket, handle_stream_event, NULL);
  return true;
}

void stream_act(struct ha_device *device) {
  size_t datalen = device->sdulen - 3;
  size_t bytes_processed = 0;

  for (uint8_t i = 0; i < 200; i++) {
    if (device->firstrun) {
      memcpy(device->buffer, &device->sdulen, 2);
      memcpy(device->buffer + 2, &device->sequence_counter, 1);
      bytes_processed = read(device->source, device->buffer + 3, datalen);
      device->firstrun = 0;
    } else {
      memcpy(device->buffer, &device->sequence_counter, 1);
      bytes_processed = read(device->source, &device->buffer + 1, datalen);
    }
    if (bytes_processed < 0) {
      log_info("File read failed: %zu (%s)\n", bytes_processed,
               strerror(errno));
    }
    bytes_processed = write(device->socket, device->buffer, device->sdulen);
    if (bytes_processed < 0) {
      log_info("Write failed: %zu (%s)\n", bytes_processed, strerror(errno));
    } else {
      device->sequence_counter++;
    }
  }
}
