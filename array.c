#include <stdlib.h>
#include "array.h"

#define BINSIZE 256

struct array *array_new() {
	struct array *a = malloc(sizeof(struct array));
	a->length = 0;
	a->arr = NULL;
	return a;
}

void array_add(struct array *a, uint8_t *s) {
	if (a->length % BINSIZE == 0) {
		a->arr = realloc(a->arr, sizeof(uint8_t *) * BINSIZE * (a->length / BINSIZE + 1));
	}
	a->arr[a->length] = s;
	a->length++;
}

void array_free(struct array *a) {
	free(a->arr);
	free(a);
}

