#ifndef XENA_BYTES_H

#include <stdint.h>
#include <stddef.h>

struct bytes {
	size_t len;
	uint8_t *bytes;
};

struct bytes *bytes_new(size_t len, uint8_t *bytes);
void bytes_free(struct bytes *b);

#define XENA_BYTES_H
#endif //XENA_BYTES_H
