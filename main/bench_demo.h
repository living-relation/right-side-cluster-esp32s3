#ifndef BENCH_DEMO_H
#define BENCH_DEMO_H

#include "sdkconfig.h"

#if CONFIG_TC_BENCH_MODE
void bench_demo_start(void);
#else
static inline void bench_demo_start(void) {}
#endif

#endif
