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
  int res = 0;
  void *buf = malloc(100);
  bzero(buf, 100);

  log_info("Received a stream event\n");

  res = fcntl(loop_data->fd, F_GETFD);
  res = write(loop_data->fd, buf, 50);

  if (res < 0)
    log_info("Write failed: %d (%s)\n", res, strerror(errno));
}

bool stream_init(char *bd_addr_raw, struct ha_device *device) {
  int res = 0;
  void *buf = malloc(200);
  bzero(buf, 200);

  device->sequence_counter = 0;
  device->socket = l2cap_connect(bd_addr_raw, device->le_psm);

  int audio_src = open("/tmp/sample.g722", O_RDONLY);

  uint16_t sdulen = 160;
  for (uint8_t i = 0; i < 200; i++) {
    if (i == 0) {
      memcpy(buf, &sdulen, 2);
      memcpy(buf + 2, &i, 1);
      res = read(audio_src, buf + 3, 160);
    } else {
      memcpy(buf, &i, 1);
      res = read(audio_src, buf + 1, 160);
    }
    if (res < 0) {
      log_info("File read failed: %d (%s)\n", res, strerror(errno));
    }
    res = write(device->socket, buf, 160);
    if (res < 0) {
      log_info("Write failed: %d (%s)\n", res, strerror(errno));
    }
  }

  log_info("socket: %d\n", device->socket);

  loop_add(device->socket, handle_stream_event, NULL);
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
