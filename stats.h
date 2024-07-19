#ifndef XENA_STATS_H
#define XENA_STATS_H

#include "stats_struct.h"
void fastats_init();
// modifies the input array
float *fastats(float *vals, int n);

#endif //XENA_STATS_H
