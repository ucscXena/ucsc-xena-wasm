#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include "baos.h"
#include "huffman.h"
#include "htfc.h"

struct inner {
	uint8_t *buff;
	size_t len;
	size_t c;
	char current[500]; // XXX handle resizing
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
#ifdef TREE
	struct node *bin_huff;
	struct node *bin_huff_case;
#else
	struct decoder decoder;
	struct decoder decoder_case;
#endif
	struct node *header_huff;
	struct node *header_huff_case;
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
#ifdef TREE
	htfc->bin_huff = huffman_new();
	huffman_tree(htfc->bin_huff, buff, bin_dict_offset);
	htfc->bin_huff_case = huffman_new();
	huffman_tree(htfc->bin_huff_case, buff, bin_dict_offset);
#else
	huffman_decoder_init(&htfc->decoder, buff, bin_dict_offset); // for canonical decoder
	huffman_decoder_init_case(&htfc->decoder_case, buff, bin_dict_offset); // for canonical decoder
#endif
	htfc->header_huff = huffman_new();
	huffman_ht_tree(htfc->header_huff, buff, 8);
	htfc->header_huff_case = huffman_new();
	huffman_ht_tree_case(htfc->header_huff_case, buff, 8);
	htfc->bin_offsets = bin_count_offset + 1;
}

void htfc_free(struct htfc *htfc) {
#ifdef TREE
	huffman_free(htfc->bin_huff);
	huffman_free(htfc->bin_huff_case);
#else
	huffman_decoder_free(&htfc->decoder);
	huffman_decoder_free(&htfc->decoder_case);
#endif
	huffman_free(htfc->header_huff);
	huffman_free(htfc->header_huff_case);
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

// XXX Can we avoid unnecessary allocations? E.g. reuse buffers when possible.
// Working buffers:
//     decompress header string
//     decompress inner bin
//     concat inner string
// Need to change the API surface for huffman, and baos?
// Maybe use something other than baos for header?
static void uncompress_bin(struct htfc *htfc, int bin_index, struct inner *inner, int ignore_case) {
	struct node *header_huff = ignore_case ? htfc->header_huff_case : htfc->header_huff;
	struct baos *out = baos_new(); // XXX smaller bin size, or alternate output API?

	uint32_t *buff32 = (uint32_t *)htfc->buff;

	int bin = 4 * htfc->first_bin + buff32[htfc->bin_offsets + bin_index];
	int header_p = huffman_decode_to(header_huff, htfc->buff, bin, out);
	size_t header_len = baos_count(out);
	uint8_t *header = baos_to_array(out);
	int rem = htfc->length % htfc->bin_size;
	int count = (rem == 0) ? htfc->bin_size :
		(bin_index == htfc->bin_count - 1) ? rem :
		htfc->bin_size;

	int upper = (bin_index == htfc->bin_count - 1) ? htfc->buff_length :
		4 * htfc->first_bin + buff32[htfc->bin_offsets + 1 + bin_index];
	struct baos *out2 = baos_new();

#ifdef TREE
	huffman_decode_range(ignore_case ? htfc->bin_huff_case : htfc->bin_huff, htfc->buff, header_p, upper, out2);
#else
	struct decoder *decoder = ignore_case ? &htfc->decoder_case : &htfc->decoder;
	huffman_canonical_decode(decoder, htfc->buff, header_p, upper, out2);
#endif

	int inner_len = baos_count(out2);
	uint8_t *out2_arr = baos_to_array(out2);

	inner_init(inner, out2_arr, inner_len, header, header_len);
	free(header);
}

void htfc_search(struct htfc *htfc, int (*cmp)(const char *, const char *), int ignore_case, char *substring, struct search_result *result) {
	int matches = 0;
	struct baos *out = baos_new();
	struct inner inner;
	int i;
	for (i = 0; i < htfc->bin_count - 1; ++i) {
		uncompress_bin(htfc, i, &inner, ignore_case);
		if (cmp(inner.current, substring)) {
			baos_push_int(out, i * htfc->bin_size);
		}
		for (int j = 1; j < htfc->bin_size; ++j) {
			inner_next(&inner);
			if (cmp(inner.current, substring)) {
				baos_push_int(out, i * htfc->bin_size + j);
			}
		}
		free(inner.buff); // XXX this API is a bit wonky
	}
	uncompress_bin(htfc, i, &inner, ignore_case);
	if (cmp(inner.current, substring)) {
		baos_push_int(out, i * htfc->bin_size);
	}
	for (int j = 1; j < htfc->length % htfc->bin_size; ++j) {
		inner_next(&inner);
		if (cmp(inner.current, substring)) {
			baos_push_int(out, i * htfc->bin_size + j);
		}
	}
	free(inner.buff); // XXX this API is a bit wonky
	result->count = baos_count(out) / 4;
	result->matches = (uint32_t *)baos_to_array(out);
}

struct htfc_iter {
	struct htfc *htfc;
	struct inner inner;
	int i;
};

struct htfc_iter *htfc_iter_init(struct htfc *htfc) {
	struct htfc_iter *iter = calloc(1, sizeof(struct htfc_iter));
	iter->htfc = htfc;
	return iter;
}

char *htfc_iter_next(struct htfc_iter *iter) {
	if (iter->i < iter->htfc->length) {
		if (iter->i % iter->htfc->bin_size == 0) {
			if (iter->inner.buff) {
				free(iter->inner.buff);
			}
			uncompress_bin(iter->htfc, iter->i / iter->htfc->bin_size, &iter->inner, 0);
		} else {
			inner_next(&iter->inner);
		}
		iter->i++;
		return iter->inner.current;
	}
	return NULL;
}

void htfc_iter_free(struct htfc_iter *iter) {
	if (iter->inner.buff) {
		free(iter->inner.buff);
	}
	free(iter);
}

// singletons, for xena client
static struct htfc *htfc_cache = NULL;
static struct search_result htfc_cache_result;

void htfc_store(uint8_t *buff, uint32_t len) {
	if (htfc_cache) {
		free(htfc_cache->buff);
		free(htfc_cache);
	}
	htfc_cache = htfc_new(buff, len);
}

static int contains(const char *a, const char *b) {
	return strstr(a, b) ? 1 : 0;
}

static int exact(const char *a, const char *b) {
	return strcmp(a, b) ? 0 : 1;
}

// result.matches must be freed after use.
// XXX substring is modified.
struct search_result *htfc_search_store(char *substring, enum search_type type) {
	size_t len = strlen(substring);
    if (type == CONTAINS) {
		for (int i = 0; i < len; ++i) {
			substring[i] = tolower(substring[i]);
		}
	}
	htfc_search(htfc_cache,
		type == EXACT ? exact : contains,
		type == EXACT ? 0 : 1,
		substring,
		&htfc_cache_result);
	return &htfc_cache_result;
}
