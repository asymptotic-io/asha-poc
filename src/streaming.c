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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void handle_stream_event(struct loop_data *loop_data) {
  void *buf = malloc(100);
  bzero(buf, 100);

  log_info("Received a stream event\n");
}

bool stream_init(char *bd_addr_raw, struct ha_device *device) {
  ssize_t bytes_processed = 0;

  device->sequence_counter = 0;
  device->socket = l2cap_connect(bd_addr_raw, device->le_psm);
  device->source = open("/tmp/sample.g722", O_RDONLY);

  // 1 byte for seq_counter and 2 for sdulen in the first sdu
  device->sdulen = 161;
  device->buffer = malloc(device->sdulen + 2);
  bzero(device->buffer, device->sdulen + 2);
  bytes_processed = read(device->source, device->sample, 160);

  if (bytes_processed < 0) {
    log_info("File read failed: %zd (%s)\n", bytes_processed, strerror(errno));
    return false;
  }
  log_info("socket: %d\n", device->socket);

  loop_add(device->socket, handle_stream_event, NULL);
  return true;
}

void stream_act(struct ha_device *device) {
  ssize_t bytes_processed = 0;

  for (uint8_t i = 0; i < 2000; i++) {
    memcpy(device->buffer, &device->sequence_counter, 1);
    memcpy(device->buffer + 1, device->sample, sizeof(device->sample));
    bytes_processed = send(device->socket, device->buffer, device->sdulen, 0);

    if (bytes_processed < 0) {
      log_info("Write failed: %zd (%s)\n", bytes_processed, strerror(errno));
      return;
    } else {
      log_info("Wrote %zd bytes\n", bytes_processed);
    }

    device->sequence_counter++;
    usleep(20000);
  }
}
