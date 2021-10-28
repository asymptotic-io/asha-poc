#include "stdio.h"
#ifndef ASHA_SUPPORT_LOOP_H
#define ASHA_SUPPORT_LOOP_H

struct loop_data {
  int loop_fd;
};

struct event_data {
  int fd;
  void *handler();
};

int loop_init();
void loop_iterate();
void loop_add(int fd);

#else
#endif
