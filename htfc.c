#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "baos.h"
#include "huffman.h"
#include "htfc.h"

struct inner {
	uint8_t *buff;
	size_t len;
	size_t c;
	uint8_t current[500]; // XXX handle resizing
};

static void inner_init(struct inner *inner, uint8_t *buff, size_t len,
		uint8_t *header, size_t header_len) {
	inner->buff = buff;
	inner->len = len;
	inner->c = 0;
	memcpy(inner->current, header, header_len);
	inner->current[header_len] = '\0';
}

static uint32_t vbyte_decode(struct inner *inner) {
	int x = 0;
	while (1) {
		uint8_t b = inner->buff[inner->c++];
		if ((b & 0x80) == 0) {
			x = (x | b) << 7;
		} else {
			return x | (b & 0x7f);
		}
	}
}

static void inner_next(struct inner *inner) {
	uint32_t i = vbyte_decode(inner);
	uint8_t b;
	// XXX check length against 'current' size
	b = inner->buff[inner->c++];
	while (b != 0) {
		inner->current[i++] = b;
		b = inner->buff[inner->c++];
	}
	inner->current[i] = '\0';
}

static size_t ht_dict_len(uint32_t *buff, size_t offset) {
	int len = buff[offset];
	return 1 + 2 * len + (len + 3) / 4;
}

static size_t huff_dict_len(uint32_t *buff, size_t offset) {
	int bits = buff[offset];
	int sum = 0;

	for (int i = 0; i < bits; ++i) {
		sum += buff[offset + 1 + i];
	}
	return 1 + bits + (sum + 3) / 4;
}

struct htfc {
	uint8_t *buff;
	int buff_length;
	int length;
	int bin_size;
	int bin_offsets;
	int bin_count; // drop this?
	int first_bin;
	struct node *bin_huff;
	struct node *header_huff;
};

void htfc_init(struct htfc *htfc, uint8_t *buff, size_t buff_length) {
	htfc->buff = buff;
	htfc->buff_length = buff_length;
	uint32_t *buff32 = (uint32_t *)buff;
	htfc->length = buff32[0];
	htfc->bin_size = buff32[1];
	int bin_dict_offset = 2 + ht_dict_len(buff32, 2); // 32
	int bin_count_offset = bin_dict_offset + huff_dict_len(buff32, bin_dict_offset); // 32
	htfc->bin_count = buff32[bin_count_offset];
	htfc->first_bin = bin_count_offset + htfc->bin_count + 1; // 32
	htfc->bin_huff = huffman_new();
	huffman_tree(htfc->bin_huff, buff, bin_dict_offset);
	htfc->header_huff = huffman_new();
	huffman_ht_tree(htfc->header_huff, buff, 8);
	htfc->bin_offsets = bin_count_offset + 1;
}

void htfc_free(struct htfc *htfc) {
	huffman_free(htfc->bin_huff);
	huffman_free(htfc->header_huff);
	free(htfc);
}

struct htfc *htfc_new(uint8_t *buff, size_t len) {
	struct htfc *ret = malloc(sizeof(struct htfc));
	htfc_init(ret, buff, len);
	return ret;
}

int htfc_count(struct htfc *htfc) {
	return htfc->length;
}

void baos_push_int(struct baos *out, int i) {
	baos_push(out, i & 0xff);
	baos_push(out, (i >> 8) & 0xff);
	baos_push(out, (i >> 16) & 0xff);
	baos_push(out, (i >> 24) & 0xff);
}

// XXX Can we avoid unnecessary allocations? E.g. reuse buffers when possible.
// Working buffers:
//     decompress header string
//     decompress inner bin
//     concat inner string
// Need to change the API surface for huffman, and baos?
// Maybe use something other than baos for header?
struct inner *uncompress_bin(struct htfc *htfc, int bin_index, struct inner *inner) {
	struct baos *out = baos_new(); // XXX smaller bin size, or alternate output API?

	uint32_t *buff32 = (uint32_t *)htfc->buff;

	int bin = 4 * htfc->first_bin + buff32[htfc->bin_offsets + bin_index];
	int header_p = huffman_decode_to(htfc->header_huff, htfc->buff, bin, out);
	size_t header_len = baos_count(out);
	uint8_t *header = baos_to_array(out);
	int rem = htfc->length % htfc->bin_size;
	int count = (rem == 0) ? htfc->bin_size :
		(bin_index == htfc->bin_count - 1) ? rem :
		htfc->bin_size;

	int upper = (bin_index == htfc->bin_count - 1) ? htfc->buff_length :
		4 * htfc->first_bin + buff32[htfc->bin_offsets + 1 + bin_index];
	struct baos *out2 = baos_new();

	huffman_decode_range(htfc->bin_huff, htfc->buff, header_p, upper, out2);
	int inner_len = baos_count(out2);
	inner_init(inner, baos_to_array(out2), inner_len, header, header_len);
	free(header);
}

void htfc_search(struct htfc *htfc, char *substring, struct search_result *result) {
	int matches = 0;
	struct baos *out = baos_new();
	struct inner inner;
	int i;
	for (i = 0; i < htfc->bin_count - 1; ++i) {
		uncompress_bin(htfc, i, &inner);
		if (strstr(inner.current, substring)) {
			baos_push_int(out, i * htfc->bin_size);
		}
		for (int j = 1; j < htfc->bin_size; ++j) {
			inner_next(&inner);
			if (strstr(inner.current, substring)) {
				baos_push_int(out, i * htfc->bin_size + j);
			}
		}
		free(inner.buff); // XXX this API is a bit wonky
	}
	uncompress_bin(htfc, i, &inner);
	if (strstr(inner.current, substring)) {
		baos_push_int(out, i * htfc->bin_size);
	}
	for (int j = 1; j < htfc->length % htfc->bin_size; ++j) {
		inner_next(&inner);
		if (strstr(inner.current, substring)) {
			baos_push_int(out, i * htfc->bin_size + j);
		}
	}
	free(inner.buff); // XXX this API is a bit wonky
	result->count = baos_count(out) / 4;
	result->matches = (uint32_t *)baos_to_array(out);
}
