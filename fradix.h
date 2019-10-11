#ifndef XENA_FRADIX_H
#define XENA_FRADIX_H

#include <stdint.h>

void fradixSort_init();
int *fradixSort(uint32_t *vals, int n, int *indicies);

#endif //XENA_FRADIX_H
