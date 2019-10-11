#ifndef XENA_STATS_H
#define XENA_STATS_H

void faminmax_init();
void fameanmedian_init();
float *faminmax(float *vals, int n);
float *fameanmedian(float *vals, int n);

#endif //XENA_STATS_H
