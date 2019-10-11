#ifndef XENA_RADIX_H
#define XENA_RADIX_H

void radixSortMultiple32(int **valList, int valListLength, int n, int *indicies);
void radixSortMultiple32_init(int N);
void radixSortMultiple(int **valList, int valListLength, int n, int *indicies);
void radixSortMultiple_init();

#endif //XENA_RADIX_H
