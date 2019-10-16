#ifndef XENA_BAOS_H
#define XENA_BAOS_H

#include <stdint.h>

struct baos;

void baos_push(struct baos *baos, uint8_t b);
void baos_push_int(struct baos *baos, uint32_t i);
void baos_copy(struct baos *baos, uint8_t *p, int start, int end);
uint8_t *baos_to_array(struct baos *baos);
struct baos *baos_new();
int baos_count(struct baos *baos);
#endif //XENA_BAOS_H
