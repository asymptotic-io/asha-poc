#include "log.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#define MAX_EVENTS_PER_ITERATION 10
#define ITERATION_TIMEOUT_MS 5000

static int loop_fd, stream_fd;

int loop_init() {
  log_info("loop: Initializing\n");
  loop_fd = epoll_create1(0);
  return loop_fd;
}

int loop_iterate() {
  struct epoll_event events[MAX_EVENTS_PER_ITERATION];

  log_info("loop: Iteration\n");

  int received_event_count = epoll_wait(
      loop_fd, events, MAX_EVENTS_PER_ITERATION, ITERATION_TIMEOUT_MS);

  int res = 0;

  void *buf = malloc(100);
  for (int i = 0; i < received_event_count; i++) {
    log_info("loop: Received an event\n");
    if (events[i].data.fd == stream_fd) {
      res = write(events[i].data.fd, buf, 100);
      log_info("RES: %d\n", res);
    }
  }

  return 0;
}

void loop_add(int fd) {
  struct epoll_event evt = {0};
  evt.events = EPOLLOUT;
  evt.data.fd = fd;
  stream_fd = fd;

  epoll_ctl(loop_fd, EPOLL_CTL_ADD, fd, &evt);
}
