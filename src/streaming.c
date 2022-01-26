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
#include <time.h>
#include <unistd.h>

/*
 * This file handles the audio data being streamed across to the
 * hearing aid
 *
 */

// 1 byte for seq_counter and 160 for the buffer
#define SDULEN 161

void handle_stream_event(struct loop_data *loop_data) {
  // This is currently doing nothing since we're doing blocking writes right now
  log_info("Received a stream event\n");
}

int stream_init(char *bd_addr_raw, struct ha_device *device, char *sound_path) {
  int ret = 0;

  device->sequence_counter = 0;
  ret = l2cap_connect(bd_addr_raw, device->le_psm);

  if (ret < 0) {
    return ret;
  }

  device->socket = ret;
  device->source = open(sound_path, O_RDONLY);
  device->sdulen = SDULEN;

  log_info("socket: %d\n", device->socket);

  memset(device->buffer, '\0', device->sdulen + 2);

  loop_add(device->socket, handle_stream_event, NULL);

  return 1;
}

bool act_once(struct ha_device *device) {
  ssize_t bytes_processed = 0;
  struct timespec t;
  int res = 0;

  bytes_processed = read(device->source, device->buffer, 160);

  if (bytes_processed < 0) {
    log_info("File read failed: %zd (%s)\n", bytes_processed, strerror(errno));
    return false;
  }

  memcpy(device->buffer, &device->sequence_counter, 1);
  memcpy(device->buffer + 1, device->buffer, sizeof(device->buffer));
  bytes_processed = send(device->socket, device->buffer, device->sdulen, 0);
  res = clock_gettime(CLOCK_MONOTONIC, &t);
  long long nanos = t.tv_nsec + (t.tv_sec * 1000000000);

  if (res < 0) {
    log_info("Could not gettime: %d (%s)\n", res, strerror(errno));
  }

  if (bytes_processed < 0) {
    log_info("%6d | %lld | Write failed: %zd (%s)\n", device->iteration, nanos,
             bytes_processed, strerror(errno));
    return false;
  } else {
    log_info("%6d | %lld | Wrote %zd bytes\n", device->iteration, nanos,
             bytes_processed);
  }

  device->sequence_counter++;
  device->iteration++;

  return true;
}

static long int micros(struct timespec t) {
  return t.tv_sec * 1000000 + (t.tv_nsec / 1000);
}

#define NANOS_20MS (20 * 1000 * 1000)
#define NANOS_1S (1000 * 1000 * 1000)

static struct timespec next_tick(struct timespec t) {
  struct timespec ret = t;

  if (t.tv_nsec + NANOS_20MS >= NANOS_1S) {
    ret.tv_sec = t.tv_sec + 1;
    ret.tv_nsec = t.tv_nsec - NANOS_1S + NANOS_20MS;
  } else {
    ret.tv_nsec = t.tv_nsec + NANOS_20MS;
  }

  return ret;
}

void stream_act(struct ha_device *device) {
  int res = 0;
  long int delay = 0;
  struct timespec this_run;
  struct timespec next_run;

  res = clock_gettime(CLOCK_MONOTONIC, &this_run);
  if (res < 0) {
    log_info("Could not gettime: %d (%s)\n", res, strerror(errno));
  }

  next_run = this_run;
  next_run.tv_nsec += NANOS_20MS;

  for (int i = 0; i < 5000; i++) {
    if (!act_once(device))
      return;

    res = clock_gettime(CLOCK_MONOTONIC, &this_run);

    if (res < 0) {
      log_info("Could not gettime: %d (%s)\n", res, strerror(errno));
    }

    delay = micros(next_run) - micros(this_run);
    log_info("sleep(%ld μs) | ", delay);
    // this_run is now here
    usleep(delay);

    // Make this_run the next iteration's this_run time
    this_run = next_run;
    // Make next_run the next iteration's next_run time
    next_run = next_tick(this_run);
  }
}
