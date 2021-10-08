#include "stdio.h"
#ifndef ASHA_SUPPORT_LOOP_H
#define ASHA_SUPPORT_LOOP_H

struct loop_data {
  int loop_fd;
};

int loop_init();
void loop_iterate();

#else
#endif
