#include <stdlib.h>
#include "bytes.h"

struct bytes *bytes_new(size_t len, uint8_t *bytes) {
	struct bytes *b = malloc(sizeof(struct bytes));
	b->len = len;
	b->bytes = bytes;
	return b;
}

void bytes_free(struct bytes *b) {
	free(b->bytes);
	free(b);
}
