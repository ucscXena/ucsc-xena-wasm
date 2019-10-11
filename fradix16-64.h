#ifndef XENA_FRADIX_16_64_H
#define XENA_FRADIX_16_64_H

#include <stdint.h>

void fradixSortL16_64(uint32_t **valsList, int m, int n, int *indicies);
void fradixSort16_64(uint32_t *vals, int n, int *indicies, uint64_t *scratch);
void fradixSort16_64_init();

#endif // XENA_FRADIX_16_64_H
