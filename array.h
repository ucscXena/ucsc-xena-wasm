#ifndef XENA_ARRAY_H
#define XENA_ARRAY_H
// growable string array

#include <stdint.h>

struct array *array_new();
void array_add(struct array *a, uint8_t *s);
void array_free(struct array *a);
struct array {
	uint8_t **arr;
	int length;
};

#endif //XENA_ARRAY_H

