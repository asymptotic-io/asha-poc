#include "stdio.h"
#ifndef ASHA_SUPPORT_LOG_H
#define ASHA_SUPPORT_LOG_H

#define log_info printf

#ifdef DEBUG
#define log_debug printf
#else
#define log_debug(...)
#endif

#else
#endif
