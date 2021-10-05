#include "stdio.h"
#define log_info printf
#ifdef DEBUG
#define log_debug printf
#else
#define log_debug(...)
#endif
