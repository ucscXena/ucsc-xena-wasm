#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include "baos.h"
#include "huffman.h"
#include "hfc.h"
#include "array.h"

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

static int common_prefix(uint8_t *a, uint8_t *b) {
	int n = 0;
	int lena = strlen((const char *)a);
	int lenb = strlen((const char *)b);
	int mlen = lena < lenb ? lena : lenb;
	while (mlen > n && a[n] == b[n]) {
		++n;
	}
	return n;
}

static void diff_rec(struct baos *out, uint8_t *a, uint8_t *b) {
	int n = common_prefix(a, b);
	vbyte_encode(out, n);
	baos_copy(out, (uint8_t *)b + n, strlen((const char *)b) - n + 1);
}

static struct bytes *compute_inner(int count, uint8_t **strings) {
	struct baos *out = baos_new();
	int i = 0;
	baos_copy(out, strings[0], strlen((const char *)strings[0]) + 1);
	count--;
	while (i < count) {
		diff_rec(out, strings[i], strings[i + 1]);
		i += 1;
	}
	return baos_to_bytes(out);
}

#define BINSIZE 256

static uint32_t *compute_offsets(int count, struct bytes **bins) {
	uint32_t *offsets = NULL;
	if (count) {
		offsets = malloc(sizeof(uint32_t) * count);
		offsets[0] = 0;
		for (int i = 1; i < count; ++i) {
			struct bytes *b = bins[i - 1];
			offsets[i] = b->len + offsets[i - 1];
		}
	}
	return offsets;
}

struct bytes *hfc_compress_sorted(int count, uint8_t **strings) {
	// build front-coded bins
	int bin_count = (count + BINSIZE - 1)  / BINSIZE;
	struct bytes **bins = malloc(sizeof(struct bytes *) * bin_count);
	for (int i = 0; i < bin_count; ++i) {
		int len = i < bin_count - 1 || count % BINSIZE == 0 ? BINSIZE : count % BINSIZE;
		bins[i] = compute_inner(len, strings + i * BINSIZE);
	}

	struct huffman_encoder *enc = huffman_bytes_encoder(bin_count, bins);

	struct bytes **inner_bins = malloc(sizeof(struct bytes *) * bin_count);
	for (int i = 0; i < bin_count; ++i) {
		struct baos *out = baos_new();
		struct bytes *b = bins[i];
		huffman_encode_bytes(out, enc, b->len, b->bytes);
		bytes_free(b);
		inner_bins[i] = baos_to_bytes(out);
	}
	free(bins);

	uint32_t *offsets = compute_offsets(bin_count, inner_bins);
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
		struct bytes *b = inner_bins[i];
		baos_copy(out, b->bytes, b->len);
		bytes_free(b);
	}
	free(inner_bins);
	// align to word boundary
	int total = baos_count(out);
	for (int i = 0; i < (4 - (total % 4)) % 4; ++i) {
		baos_push(out, 0);
	}
	free(offsets);
	huffman_encoder_free(enc);

	return baos_to_bytes(out);
}

static int cmpstr(const void *a, const void *b) {
	return strcmp(*((const char **)a), *((const char **)b));
}

// XXX modifies strings
struct bytes *hfc_compress(int count, uint8_t **strings) {
	qsort(strings, count, sizeof(uint8_t *), cmpstr);
	return hfc_compress_sorted(count, strings);
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
	size_t header_len = strlen((const char *)buff);
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
	free(hfc->buff);
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
	uint32_t *buff32 = (uint32_t *)hfc->buff;

	int bin = 4 * hfc->first_bin + buff32[hfc->bin_offsets + bin_index];

	int rem = hfc->length % hfc->bin_size;
	int count = (rem == 0) ? hfc->bin_size :
		(bin_index == hfc->bin_count - 1) ? rem :
		hfc->bin_size;

	int upper = (bin_index == hfc->bin_count - 1) ? hfc->buff_length :
		4 * hfc->first_bin + buff32[hfc->bin_offsets + 1 + bin_index];
	struct baos *out = baos_new();

	struct decoder *decoder = ignore_case ? &hfc->decoder_case : &hfc->decoder;
	huffman_canonical_decode(decoder, hfc->buff, bin, upper, out);

	int inner_len = baos_count(out);
	uint8_t *out_arr = baos_to_array(out);

	inner_init(inner, out_arr, inner_len);
}

void hfc_search_method(struct hfc *hfc, int (*cmp)(const char *, const char *), int ignore_case, char *substring, struct search_result *result) {
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

static void add_dup(struct array *a, const char *s) {
	// we have to copy the string out of the mutable iterator, because
	// the collection is compressed.
	array_add(a, strdup(s));
}

static void clear_array(struct array *a) {
	if (a) {
		for (int i = 0; i < a->length; ++i) {
			free(a->arr[i]);
		}
		array_free(a);
	}
}

struct hfc *hfc_merge_two(struct hfc *ha, struct hfc *hb) {
	struct hfc_iter *ia = hfc_iter_init(ha);
	struct hfc_iter *ib = hfc_iter_init(hb);

	// assumes ha and hb are not empty
	char *ba = hfc_iter_next(ia);
	char *bb = hfc_iter_next(ib);

	struct array *out = array_new();
	while (ba && bb) {
		int cmp = strcmp(ba, bb);
		if (cmp == 0) {
			add_dup(out, ba);
			ba = hfc_iter_next(ia);
			bb = hfc_iter_next(ib);
		} else if (cmp < 0) {
			add_dup(out, ba);
			ba = hfc_iter_next(ia);
		} else {
			add_dup(out, bb);
			bb = hfc_iter_next(ib);
		}
	}
	// only one of these loops will run, because the other
	// is empty.
	while (ba) {
		add_dup(out, ba);
		ba = hfc_iter_next(ia);
	}
	while (bb) {
		add_dup(out, bb);
		bb = hfc_iter_next(ib);
	}
	struct bytes *buff = hfc_compress(out->length, (uint8_t **)out->arr);
	struct hfc *hfc = hfc_new(buff->bytes, buff->len);
	clear_array(out);
	free(buff);
	hfc_iter_free(ia);
	hfc_iter_free(ib);

	return hfc;
}

//
// js exported API
//
static struct hfc *hfc_cache = NULL;
static struct search_result hfc_cache_result;
static struct array *inner_cache = NULL;
static int inner_bin = -1;

void hfc_clear_cache() {
	clear_array(inner_cache);
	inner_cache = NULL;
	inner_bin = -1;
}

void hfc_set_internal(struct hfc *hfc) {
	if (hfc_cache) {
		hfc_free(hfc_cache);
	}
	hfc_cache = hfc;
	hfc_clear_cache();
	// XXX clean up search result. Also, make sure we aren't
	// leaking search result in the client after every search.
	// hfc_cache_result
}

void hfc_set(uint8_t *buff, uint32_t len) {
	hfc_set_internal(hfc_new(buff, len));
}

static int contains(const char *a, const char *b) {
	return strstr(a, b) ? 1 : 0;
}

static int exact(const char *a, const char *b) {
	return strcmp(a, b) ? 0 : 1;
}

// result.matches must be freed after use.
// XXX substring is modified.
struct search_result *hfc_search(char *substring, enum search_type type) {
	size_t len = strlen(substring);
    if (type == CONTAINS) {
		for (int i = 0; i < len; ++i) {
			substring[i] = tolower(substring[i]);
		}
	}
	hfc_search_method(hfc_cache,
		type == EXACT ? exact : contains,
		type == EXACT ? 0 : 1,
		substring,
		&hfc_cache_result);
	return &hfc_cache_result;
}

void hfc_merge(uint8_t *buff, uint32_t len) {
	struct hfc *hfc = hfc_new(buff, len);
	hfc_set_internal(hfc_merge_two(hfc_cache, hfc));
	hfc_free(hfc);
}

char *hfc_lookup(int i) {
	int bin = i / hfc_cache->bin_size;
	if (bin != inner_bin) {
		clear_array(inner_cache);
		inner_cache = array_new();
		inner_bin = bin;
		struct inner inner;
		uncompress_bin(hfc_cache, bin, &inner, 0);
		array_add(inner_cache, strdup(inner.current));
		int rem = hfc_cache->length % hfc_cache->bin_size;
		int last = rem == 0 ? hfc_cache->bin_size :
			bin == hfc_cache->bin_count - 1 ? rem :
			hfc_cache->bin_size;
		for (int i = 1; i < last; ++i) {
			inner_next(&inner);
			array_add(inner_cache, strdup(inner.current));
		}
		free(inner.buff); // XXX this API is a bit wonky
	}
	return inner_cache->arr[i % hfc_cache->bin_size];
}

// apply search results as a filter
void hfc_filter() {
	struct array *out = array_new();
	for (int i = 0; i < hfc_cache_result.count; ++i) {
		// have to copy out of cache, since cache will be invalidated
		// during this loop.
		array_add(out, strdup(hfc_lookup(hfc_cache_result.matches[i])));
	}
	struct bytes *buff = hfc_compress(out->length, (uint8_t **)out->arr);
	clear_array(out);
	hfc_set(buff->bytes, buff->len);
	free(buff);
}
struct hfc *hfc_get_cache() {
	return hfc_cache;
}

int hfc_length() {
	return hfc_cache->length;
}

int hfc_buff_length() {
	return hfc_cache->buff_length;
}

char *hfc_buff() {
	return (char *)hfc_cache->buff;
}
