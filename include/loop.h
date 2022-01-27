#include "stdio.h"
#ifndef ASHA_SUPPORT_LOOP_H
#define ASHA_SUPPORT_LOOP_H

struct loop_data {
  int fd;
  void (*handler)(struct loop_data *);
  void *payload;
};

struct event_data {};

int loop_init();
void loop_iterate(int loop_fd);
void loop_add(int loop_fd, int fd, void (*handler)(struct loop_data *),
              void *payload);

#else
#endif
