#include "bluez.h"
#include "log.h"
#include "loop.h"
#include "types.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
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
  log_info("RES: %d\n", res);
}

bool stream_init(char *bd_addr_raw, struct ha_device *device) {
  device->sequence_counter = 0;
  device->socket = l2cap_connect(bd_addr_raw, device->le_psm);
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
