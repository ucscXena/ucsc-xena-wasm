#ifndef XENA_FRADIX16_H
#define XENA_FRADIX16_H

#include <stdint.h>

void fradixSort16_init();
void fradixSort16(uint32_t *vals, int n, int *indicies);
void fradixSort16InPlace(uint32_t *vals, int n);

#endif //XENA_FRADIX16_H
