#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include "baos.h"
#include "huffman.h"
#include "hfc.h"

#include <stdio.h> // debugging

// encode support
static void vbyte_encode(struct baos *out, long i) {
	int len = (i < 0x80) ? 0 :
		(i < 0x4000) ? 1 :
		(i < 0x200000) ? 2 :
		3;
	switch (len) {
		case 3:
			baos_push(out, (i >> 21) & 0x7f);
		case 2:
			baos_push(out, (i >> 14) & 0x7f);
		case 1:
			baos_push(out, (i >> 7) & 0x7f);
		case 0:
			baos_push(out, 0x80 | (i & 0x7f));
	}
}

static int common_prefix(char *a, char *b) {
	int n = 0;
	int lena = strlen(a);
	int lenb = strlen(b);
	int mlen = lena < lenb ? lena : lenb;
	while (mlen > n && a[n] == b[n]) {
		++n;
	}
	return n;
}

static void diff_rec(struct baos *out, uint8_t *a, uint8_t *b) {
	int n = common_prefix(a, b);
	vbyte_encode(out, n);
	baos_copy(out, (uint8_t *)b + n, strlen(b) - n + 1);
}

static struct bytes *compute_inner(int count, uint8_t **strings) {
	struct baos *out = baos_new();
	int i = 0;
	baos_copy(out, strings[0], strlen(strings[0]) + 1);
	count--;
	while (i < count) {
		diff_rec(out, strings[i], strings[i + 1]);
		i += 1;
	}
	return baos_to_bytes(out);
}

#define BINSIZE 256

uint32_t *compute_offsets(struct queue *bins) {
	int c = queue_count(bins);
	uint32_t *offsets = malloc(sizeof(uint32_t) * c);
	offsets[0] = 0;
	for (int i = 1; i < c; ++i) {
		struct bytes *b = queue_take(bins);
		offsets[i] = b->len + offsets[i - 1];
	}
	queue_free(bins);
	return offsets;
}

struct bytes *hfc_compress(int count, uint8_t **strings) {
	// build front-coded bins
	struct queue *bin_queue = queue_new();
	for (int i = 0; i < count; i += BINSIZE) {
		int len = i + BINSIZE > count ? count % BINSIZE : BINSIZE;
		queue_add(bin_queue, compute_inner(len, strings + i));
	}
	// Note that we need to walk the queue, then walk it again.
	// Could switch to a list or array type, instead of copy.
	struct huffman_encoder *enc = huffman_bytes_encoder(queue_copy(bin_queue));

	int bin_count = queue_count(bin_queue);
	struct queue *inner_queue = queue_new();
	for (int i = 0; i < bin_count; ++i) {
		struct baos *out = baos_new();
		struct bytes *b = queue_take(bin_queue);
		huffman_encode_bytes(out, enc, b->len, b->bytes);
		queue_add(inner_queue, baos_to_bytes(out));
	}
	queue_free(bin_queue);

	uint32_t *offsets = compute_offsets(queue_copy(inner_queue));
	struct baos *out = baos_new();
	baos_push(out, 'h');
	baos_push(out, 'f');
	baos_push(out, 'c');
	baos_push(out, 0);
	baos_push_int(out, count);
	baos_push_int(out, BINSIZE);
	huffman_serialize(out, enc);
	for (int i = 0; i < bin_count; ++i) {
		baos_push_int(out, offsets[i]);
	}
	for (int i = 0; i < bin_count; ++i) {
		struct bytes *b = queue_take(inner_queue);
		baos_copy(out, b->bytes, b->len);
	}
	// align to word boundary
	int total = baos_count(out);
	for (int i = 0; i < (4 - (total % 4)) % 4; ++i) {
		baos_push(out, 0);
	}
	queue_free(inner_queue);
	free(offsets);

	return baos_to_bytes(out);
}


// decode

// support for iterators
struct inner {
	uint8_t *buff;
	size_t len;
	size_t c;
	char current[500]; // XXX handle resizing
};

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

static void inner_init(struct inner *inner, uint8_t *buff, size_t len) {
	size_t header_len = strlen(buff);
	uint8_t *header = buff;
	inner->buff = buff;
	inner->len = len;
	inner->c = header_len + 1;
	memcpy(inner->current, header, header_len);
	inner->current[header_len] = '\0';
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

static size_t huff_dict_len(uint32_t *buff, size_t offset) {
	int bits = buff[offset];
	int sum = 0;

	for (int i = 0; i < bits; ++i) {
		sum += buff[offset + 1 + i];
	}
	return 1 + bits + (sum + 3) / 4;
}

struct hfc {
	uint8_t *buff;
	int buff_length;
	int length;
	int bin_size;
	int bin_offsets;
	int bin_count;
	int first_bin;
	struct decoder decoder;
	struct decoder decoder_case;
};

void hfc_init(struct hfc *hfc, uint8_t *buff, size_t buff_length) {
	hfc->buff = buff;
	hfc->buff_length = buff_length;
	uint32_t *buff32 = (uint32_t *)buff;
	// [0] is the type id
	hfc->length = buff32[1];
	hfc->bin_size = buff32[2];
	int bin_dict_offset = 3; // 32
	hfc->bin_offsets = bin_dict_offset + huff_dict_len(buff32, bin_dict_offset);
	hfc->bin_count = (hfc->length + hfc->bin_size - 1) / hfc->bin_size;
	hfc->first_bin = hfc->bin_offsets + hfc->bin_count; // 32
	huffman_decoder_init(&hfc->decoder, buff, bin_dict_offset); // for canonical decoder
	huffman_decoder_init_case(&hfc->decoder_case, buff, bin_dict_offset); // for canonical decoder
}

void hfc_free(struct hfc *hfc) {
	huffman_decoder_free(&hfc->decoder);
	huffman_decoder_free(&hfc->decoder_case);
	free(hfc);
}

struct hfc *hfc_new(uint8_t *buff, size_t len) {
	struct hfc *ret = malloc(sizeof(struct hfc));
	hfc_init(ret, buff, len);
	return ret;
}

int hfc_count(struct hfc *hfc) {
	return hfc->length;
}

// XXX Can we avoid unnecessary allocations? E.g. reuse buffers when possible.
// Working buffers:
//     decompress header string
//     decompress inner bin
//     concat inner string
// Need to change the API surface for huffman, and baos?
// Maybe use something other than baos for header?
static void uncompress_bin(struct hfc *hfc, int bin_index, struct inner *inner, int ignore_case) {
	struct baos *out = baos_new(); // XXX smaller bin size, or alternate output API?

	uint32_t *buff32 = (uint32_t *)hfc->buff;

	int bin = 4 * hfc->first_bin + buff32[hfc->bin_offsets + bin_index];

	int rem = hfc->length % hfc->bin_size;
	int count = (rem == 0) ? hfc->bin_size :
		(bin_index == hfc->bin_count - 1) ? rem :
		hfc->bin_size;

	int upper = (bin_index == hfc->bin_count - 1) ? hfc->buff_length :
		4 * hfc->first_bin + buff32[hfc->bin_offsets + 1 + bin_index];
	struct baos *out2 = baos_new();

	struct decoder *decoder = ignore_case ? &hfc->decoder_case : &hfc->decoder;
	huffman_canonical_decode(decoder, hfc->buff, bin, upper, out2);

	int inner_len = baos_count(out2);
	uint8_t *out2_arr = baos_to_array(out2);

	inner_init(inner, out2_arr, inner_len);
}

void hfc_search(struct hfc *hfc, int (*cmp)(const char *, const char *), int ignore_case, char *substring, struct search_result *result) {
	int matches = 0;
	struct baos *out = baos_new();
	struct inner inner;
	int i;
	for (i = 0; i < hfc->bin_count - 1; ++i) {
		uncompress_bin(hfc, i, &inner, ignore_case);
		if (cmp(inner.current, substring)) {
			baos_push_int(out, i * hfc->bin_size);
		}
		for (int j = 1; j < hfc->bin_size; ++j) {
			inner_next(&inner);
			if (cmp(inner.current, substring)) {
				baos_push_int(out, i * hfc->bin_size + j);
			}
		}
		free(inner.buff); // XXX this API is a bit wonky
	}
	uncompress_bin(hfc, i, &inner, ignore_case);
	if (cmp(inner.current, substring)) {
		baos_push_int(out, i * hfc->bin_size);
	}
	for (int j = 1; j < hfc->length % hfc->bin_size; ++j) {
		inner_next(&inner);
		if (cmp(inner.current, substring)) {
			baos_push_int(out, i * hfc->bin_size + j);
		}
	}
	free(inner.buff); // XXX this API is a bit wonky
	result->count = baos_count(out) / 4;
	result->matches = (uint32_t *)baos_to_array(out);
}

struct hfc_iter {
	struct hfc *hfc;
	struct inner inner;
	int i;
};

struct hfc_iter *hfc_iter_init(struct hfc *hfc) {
	struct hfc_iter *iter = calloc(1, sizeof(struct hfc_iter));
	iter->hfc = hfc;
	return iter;
}

char *hfc_iter_next(struct hfc_iter *iter) {
	if (iter->i < iter->hfc->length) {
		if (iter->i % iter->hfc->bin_size == 0) {
			if (iter->inner.buff) {
				free(iter->inner.buff);
			}
			uncompress_bin(iter->hfc, iter->i / iter->hfc->bin_size, &iter->inner, 0);
		} else {
			inner_next(&iter->inner);
		}
		iter->i++;
		return iter->inner.current;
	}
	return NULL;
}

void hfc_iter_free(struct hfc_iter *iter) {
	if (iter->inner.buff) {
		free(iter->inner.buff);
	}
	free(iter);
}

// singletons, for xena client
static struct hfc *hfc_cache = NULL;
static struct search_result hfc_cache_result;

void hfc_store(uint8_t *buff, uint32_t len) {
	if (hfc_cache) {
		free(hfc_cache->buff);
		free(hfc_cache);
	}
	hfc_cache = hfc_new(buff, len);
}

static int contains(const char *a, const char *b) {
	return strstr(a, b) ? 1 : 0;
}

static int exact(const char *a, const char *b) {
	return strcmp(a, b) ? 0 : 1;
}

// result.matches must be freed after use.
// XXX substring is modified.
struct search_result *hfc_search_store(char *substring, enum search_type type) {
	size_t len = strlen(substring);
    if (type == CONTAINS) {
		for (int i = 0; i < len; ++i) {
			substring[i] = tolower(substring[i]);
		}
	}
	hfc_search(hfc_cache,
		type == EXACT ? exact : contains,
		type == EXACT ? 0 : 1,
		substring,
		&hfc_cache_result);
	return &hfc_cache_result;
}
