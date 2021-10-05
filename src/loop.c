#include <sys/epoll.h>
#include "log.h"

#define MAX_EVENTS_PER_ITERATION 10
#define ITERATION_TIMEOUT_MS 1000

static int loop_fd;

void loop_init() {
  log_info("loop: Initializing\n");
  loop_fd = epoll_create1(0);
}

int loop_iterate() {
  struct epoll_event events[MAX_EVENTS_PER_ITERATION];

  log_info("loop: Iteration\n");

  int received_event_count = epoll_wait(loop_fd, events, MAX_EVENTS_PER_ITERATION, ITERATION_TIMEOUT_MS);

  for (int i=0; i < received_event_count; i++) {
    log_info("loop: Received an event\n");
  }

  return 0;
}
