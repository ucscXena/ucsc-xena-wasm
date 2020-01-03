#ifndef XENA_ARRAY_H
#define XENA_ARRAY_H
// growable string array

#include <stdint.h>

struct array *array_new();
void array_add(struct array *a, char *s);
void array_free(struct array *a);
struct array {
	char **arr;
	uint32_t length;
};

#endif //XENA_ARRAY_H

