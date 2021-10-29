#include "loop.h"
#include "log.h"
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#define MAX_EVENTS_PER_ITERATION 10
#define ITERATION_TIMEOUT_MS 5000

static int loop_fd;

int loop_init() {
  log_info("loop: Initializing\n");
  loop_fd = epoll_create1(0);
  return loop_fd;
}

void loop_iterate() {
  struct epoll_event events[MAX_EVENTS_PER_ITERATION];
  struct loop_data *loop_data = NULL;

  log_info("loop: Iteration\n");

  int received_event_count = epoll_wait(
      loop_fd, events, MAX_EVENTS_PER_ITERATION, ITERATION_TIMEOUT_MS);

  for (int i = 0; i < received_event_count; i++) {
    log_info("loop: Received an event\n");
    loop_data = events[i].data.ptr;

    loop_data->handler(loop_data);
    log_info("loop: Handled an event\n");
  }
}

void loop_add(int fd, void (*handler)(struct loop_data *), void *payload) {
  struct epoll_event evt = {0};
  struct loop_data *loop_data = malloc(sizeof(struct loop_data));
  loop_data->fd = fd;
  loop_data->handler = handler;
  loop_data->payload = payload;

  evt.events = EPOLLOUT;
  evt.data.ptr = loop_data;

  epoll_ctl(loop_fd, EPOLL_CTL_ADD, fd, &evt);
}
