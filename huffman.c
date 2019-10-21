#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <byteswap.h>
#include <stdio.h>
#include <string.h>
#include "baos.h"
#include "huffman.h"
#include "queue.h"
#include "bytes.h"

void update_freqs(int *freqs, uint8_t *s, size_t len) {
	while (len--) {
		freqs[*s++]++;
	}
}

int *byte_freqs(struct queue *q) {
	int *freqs = calloc(256, sizeof(int));
	int count = queue_count(q);
	queue_iter itr;
	queue_iter_init(q, &itr);
	while (queue_iter_next(&itr)) {
		struct bytes *b = queue_iter_value(itr);
		update_freqs(freqs, b->bytes, b->len);
	}
	return freqs;
}

// XXX not thread safe. Work-around for lack of qsort_r.
static int *gt_freqs = NULL;
static int gt(const void *a, const void *b) {
	int va = gt_freqs[*(int *)a];
	int vb = gt_freqs[*(int *)b];
	return va > vb ? 1 :
		vb > va ? -1 :
		0;
}

enum encode_node_type {SYMBOL, MERGED};

struct encode_tree {
	int priority;
	enum encode_node_type type;
	union {
		struct {
			struct encode_tree *left;
			struct encode_tree *right;
		} child;
		char symbol;
	};
};

void encode_tree_dump_n(struct encode_tree *tree, int depth) {
	if (tree) {
		if (tree->type == MERGED) {
			printf("%*c merged %d\n", depth, ' ', tree->priority);
			encode_tree_dump_n(tree->child.left, depth + 1);
			encode_tree_dump_n(tree->child.right, depth + 1);
		} else {
			printf("%*c symbol %d %d (%c)\n", depth, ' ', tree->priority, tree->symbol, isprint(tree->symbol) ? tree->symbol : ' ');
		}
	}
}

void encode_tree_dump(struct encode_tree *tree) {
	encode_tree_dump_n(tree, 1);
}

struct encode_tree *merge(struct encode_tree *a, struct encode_tree *b)  {
	struct encode_tree *m = malloc(sizeof(struct encode_tree));
	m->priority = a->priority + b->priority;
	m->type = MERGED;
	m->child.left = a;
	m->child.right = b;
	return m;
}

struct encode_tree *symbol(char s, int priority) {
	struct encode_tree *m = malloc(sizeof(struct encode_tree));
	m->priority = priority;
	m->type = SYMBOL;
	m->symbol = s;
	return m;
}

struct encode_tree *take_lowest(struct array_queue *a, struct array_queue *b) {
	// failure mode is a is null
	if (!array_queue_count(b)) {
		return array_queue_take(a);
	}
	if (!array_queue_count(a)) {
		return array_queue_take(b);
	}
	int va = ((struct encode_tree *)(array_queue_peek(a)))->priority;
	int vb = ((struct encode_tree *)(array_queue_peek(b)))->priority;
	return vb >= va ?
		array_queue_take(a) :
		array_queue_take(b);
}

// Encoding tree. Different from decoding tree.
// Needs to hold a symbol, or a merged node.
struct encode_tree *encode_tree_build(int *freqs) {
	int idx[NBYTE];
	for (int i = 0; i < NBYTE; ++i) {
		idx[i] = i;
	}
	gt_freqs = freqs;
	qsort(idx, NBYTE, sizeof(int), gt);
	struct array_queue *leaves = array_queue_new(NBYTE);
	struct array_queue *nodes = array_queue_new(NBYTE);
	for (int i = 0; i < NBYTE; ++i) {
		if (freqs[idx[i]]) {
			array_queue_add(leaves, symbol(idx[i], freqs[idx[i]]));
		}
	}
	// algorithm is to pull the two lowest values, merge, and repeat
	// until all are merged (i.e. there is one node)
	while (array_queue_count(leaves) + array_queue_count(nodes) > 1) {
		struct encode_tree *a = take_lowest(leaves, nodes);
		struct encode_tree *b = take_lowest(leaves, nodes);
		array_queue_add(nodes, merge(a, b));
	}
	struct encode_tree *tree = array_queue_take(nodes);
	array_queue_free(nodes);
	array_queue_free(leaves);
	return tree;
}

void encode_tree_free(struct encode_tree *tree) {
	if (tree) {
		if (tree->type == MERGED) {
			encode_tree_free(tree->child.left);
			encode_tree_free(tree->child.right);
		}
		free(tree);
	}
}

struct queue *find_depth_n(struct queue *nodes, struct queue *acc) {
	if (!queue_count(nodes)) {
		queue_free(nodes);
		return acc;
	}
	struct queue *symbols = queue_new();
	struct queue *merged = queue_new();
	struct encode_tree *n;
	while ((n = queue_take(nodes))) {
		if (n->type == MERGED) {
			queue_add(merged, n->child.left);
			queue_add(merged, n->child.right);
		} else {
			queue_add(symbols, (void *)(size_t)n->symbol);
		}
	}
	queue_add(acc, symbols);
	queue_free(nodes);
	return find_depth_n(merged, acc);
}

// group and order symbols by depth.
struct queue *find_depth(struct encode_tree *tree) {
	struct queue *nodes = queue_new();
	struct queue *acc = queue_new();
	queue_add(nodes, tree);
	return find_depth_n(nodes, acc);
}

struct huffman_code {
	int length;
	long code;
};

uint8_t *queue_to_chars(struct queue *q) {
	int n = queue_count(q);
	uint8_t *chars = malloc(n);
	while (n--) {
		chars[n] = (size_t)queue_take(q);
	}
	queue_free(q);
	return chars;
}

int cmp_char(const void *a, const void *b) {
	uint8_t x = *(uint8_t *)a;
	uint8_t y = *(uint8_t *)b;
	return x > y ? 1 :
		y > x ? -1 :
		0;
}

struct canonical {
	int n;
	int *lens;
	uint8_t *symbols;
};

struct huffman_encoder {
	struct canonical canonical;
	struct huffman_code dict[NBYTE];
};

struct huffman_encoder *huffman_encoder_new(int n) {
	struct huffman_encoder *codes = calloc(1, sizeof(struct huffman_encoder));
	codes->canonical.n = n;
	codes->canonical.lens = calloc(n, sizeof(int));
	codes->canonical.symbols = NULL;
	return codes;
}

void huffman_serialize(struct baos *out, struct huffman_encoder *huff) {
	struct canonical *c = &huff->canonical;
	int n = c->n;
	int total = 0;
	baos_push_int(out, n);
	for (int i = 0; i < n; ++i) {
		int len = c->lens[i];
		baos_push_int(out, len);
		total += len;
	}
	for (int i = 0; i < total; ++i) {
		baos_push(out, c->symbols[i]);
	}
	// extend to word boundary
	for (int i = 0; i < 4 * ((total + 3) / 4) - total; ++i) {
		baos_push(out, 0);
	}
}

void huffman_encoder_free(struct huffman_encoder *enc) {
	free(enc->canonical.lens);
	free(enc->canonical.symbols);
	free(enc);
}

// returns list of [{length, symbols: [{symbol, code}]}]
// We need two things from this process: the dict for decoding,
// and the hash for encoding. Hash for encoding is byte -> {symbol, length}.
// This can just be an array. Decode dict can be just lens
// and symbols. We can reconstruct the decode tree from that.
struct huffman_encoder *encoder(struct queue *depths) {
	struct queue *symbols;
	int depth = 1;
	int code = 0;
	struct huffman_encoder *encoder = huffman_encoder_new(queue_count(depths));
	struct huffman_code *dict = encoder->dict;
	int total = 0;
	while ((symbols = queue_take(depths))) {
		int n;
		if ((n = queue_count(symbols))) {
			uint8_t *syms = queue_to_chars(symbols);
			qsort(syms, n, 1, cmp_char);
			for (int i = 0; i < n; ++i) {
				dict[syms[i]].length = depth;
				dict[syms[i]].code = code++;
			}
			encoder->canonical.lens[depth - 1] = n;
			total += n;
			encoder->canonical.symbols = realloc(encoder->canonical.symbols, total);
			memcpy(encoder->canonical.symbols + total - n, syms, n);
			free(syms);
		} else {
			queue_free(symbols);
		}
		depth++;
		code <<= 1;
	}
	queue_free(depths);
	return encoder;
}

// build encoder for a set of strings
struct huffman_encoder *huffman_strings_encoder(int count, char **s) {
	struct queue *in = queue_new();
	for (int i = 0; i < count; ++i) {
		queue_add(in, bytes_new(strlen(s[i]) + 1, s[i]));
	}
	int *freqs = byte_freqs(in);
	queue_free(in);
	struct encode_tree *t = encode_tree_build(freqs);
	struct queue *depths = find_depth(t);
	encode_tree_free(t);
	free(freqs);
	return encoder(depths);
}

// build encoder for a set of buffers
struct huffman_encoder *huffman_bytes_encoder(struct queue *in) {
	int *freqs = byte_freqs(in);
	struct encode_tree *t = encode_tree_build(freqs);
	struct queue *depths = find_depth(t);
	encode_tree_free(t);
	free(freqs);
	return encoder(depths);
}

// Is this the right API? This emits some extra bits at the end, due to use of baos.
// Are those bits zero? Might be a problem if not.
void huffman_encode_bytes(struct baos *output, struct huffman_encoder *enc, int len, uint8_t *in) {
	struct huffman_code *dict = enc->dict;
	uint64_t out = 0;
	int m = 0;
	for (int i = 0; i < len; ++i) {
		struct huffman_code *c = dict + in[i];
		long code = c->code;
		int length = c->length;
		out |= code << (64 - m - length);
		m += length;
		if (m >= 8) {
			baos_push(output, (out >> 56) & 0xff);
			if (m >= 16) {
				baos_push(output, (out >> 48) & 0xff);
				if (m >= 24) {
					baos_push(output, (out >> 40) & 0xff);
					if (m >= 32) {
						baos_push(output, (out >> 32) & 0xff);
						if (m >= 40) {
							baos_push(output, (out >> 24) & 0xff);
							if (m >= 48) {
								baos_push(output, (out >> 16) & 0xff);
								if (m >= 56) {
									baos_push(output, (out >> 8) & 0xff);
									if (m == 64) {
										baos_push(output, out & 0xff);
									}
								}
							}
						}
					}
				}
			}
		}
		if (m >= 8) {
			int shift = 8 * (m / 8);
			out <<= shift;
			m -= shift;
		}
	}
	if (m != 0) {
		baos_push(output, (out >> 56) & 0xff);
	}
}

// decode tree

enum node_type {LEAF, INNER};

struct node {
	enum node_type type;
	union {
		struct {
			struct node *left;
			struct node *right;
		} inner;
		uint8_t symbol;
	};
};

static struct node *Inner(void) {
    struct node *ret = malloc(sizeof(struct node));
    ret->type = INNER;
    ret->inner.left = NULL;
    ret->inner.right = NULL;
    return ret;
}

static struct node *Leaf(uint8_t symbol) {
    struct node *ret = malloc(sizeof(struct node));
    ret->type = LEAF;
    ret->symbol = symbol;
    return ret;
}

// top-level constructor
struct node *huffman_new(void) {
	return Inner();
}

void huffman_free(struct node *node) {
	if (node) {
		if (node->type == LEAF) {
			free(node);
		} else {
			huffman_free(node->inner.left);
			huffman_free(node->inner.right);
			free(node);
		}
	}
}

static void insert(struct node *root, long code, int len, uint8_t sym) {
	struct node *n = root;
	for (int i = len - 1; i > 0; i--) {
		if ((code & (1 << i)) == 0) {
			if (n->inner.right == NULL) {
				n->inner.right = Inner();
			}
			n = n->inner.right;
		} else {
			if (n->inner.left == NULL) {
				n->inner.left = Inner();
			}
			n = n->inner.left;
		}
	}
	if ((code & 1) == 0) {
		n->inner.right = Leaf(sym);
	} else {
		n->inner.left = Leaf(sym);
	}
}

struct node *huffman_tree(struct node *root, uint8_t *buff8, int offset32) {
	uint32_t *buff32 = (uint32_t *)buff8;
	int len = buff32[offset32];
	long code = 0;
	int symbol8 = 4 * (offset32 + 1 + len);
	struct node *tree = root;
	for (int i = 1; i <= len; ++i) {
	    int N = buff32[offset32 + i];
	    long icode = code;
	    for (int j = 0; j < N; ++j, ++icode) {
		    insert(root, icode, i, buff8[symbol8 + j]);
	    }
	    code = (code + N) << 1;
	    symbol8 += N;
    }
    return root;
}

// some debug helpers
#ifdef HUFFMAN_DEBUG
void dump_binary1(uint64_t c, int len) {
	for (uint64_t j = 1LL << (len - 1); j > 0; j >>= 1) {
		printf(j & c ? "1" : "0");
	}
}

void dump_binary(uint64_t c, int len) {
	for (int j = len - 1; j >= 0; --j) {
		printf((1LL << j) & c ? "1" : "0");
		if ((j % 8) == 0) {
			printf(" ");
		}
	}
}

void dump_tree_c(struct node *root, long c, int len) {
	if (root->type == LEAF) {
		dump_binary(c, len);
		if (isprint(root->symbol)) {
			printf(": %x(%c)\n", root->symbol, root->symbol);
		} else {
			printf(": %x\n", root->symbol);
		}
	} else {
		if (root->inner.left) {
			dump_tree_c(root->inner.left, (c << 1) | 1, len + 1);
		}
		if  (root->inner.right) {
			dump_tree_c(root->inner.right, c << 1, len + 1);
		}
	}
}

void dump_tree(struct node *root) {
	dump_tree_c(root, 0, 0);
}

void dump_sym(uint8_t s) {
	if (isprint(s)) {
		printf("pushing %x(%c)\n", s, s);
	} else {
		printf("pushing %x\n", s);
	}
}
#endif

int identity(int x) {
    return x;
}

// for canonical decoder. offset, and right-justified base
void huffman_decoder_init_transform(struct decoder *decoder, uint8_t *buff8, int offset32, int (*transform)(int)) {
	uint32_t *buff32 = (uint32_t *)buff8;
	int len = buff32[offset32];
	uint64_t *base = malloc(sizeof(*base) * (len + 1));
	uint32_t *offset = malloc(sizeof(*offset) * len);
	uint64_t code = 0;
	uint32_t offs = 0;
	int i;
	for (i = 0; i < len; ++i) {
		int N = buff32[offset32 + i + 1];
		base[i] = code;
		offset[i] = offs;
		code = (code + N) << 1;
		offs += N;
	}
	base[i] = code; // needed in the decoder for termination
	decoder->base = base;
	decoder->offset = offset;
	uint8_t *symbols = buff8 + 4 * (offset32 + 1 + len);
	// XXX is offs the right number?
	decoder->symbols = malloc(offs);
	for (int i = 0; i < offs; ++i) {
		decoder->symbols[i] = transform(symbols[i]);
	}
}

void huffman_decoder_free(struct decoder *decoder) {
	free(decoder->base);
	free(decoder->offset);
	free(decoder->symbols);
}

void huffman_decoder_init(struct decoder *decoder, uint8_t *buff8, int offset32) {
    huffman_decoder_init_transform(decoder, buff8, offset32, identity);
}

void huffman_decoder_init_case(struct decoder *decoder, uint8_t *buff8, int offset32) {
    huffman_decoder_init_transform(decoder, buff8, offset32, tolower);
}

// for the ONE-SHIFT algorithm, below
#if 0
// our "canonical" codes are inverted compared to the usual encoding, which
// makes base & offset different from the paper.
// offset, and left-justified base. base and offset are *last* element, not first.
void huffman_decoder_init2(struct decoder *decoder, uint8_t *buff8, int offset32) {
	uint32_t *buff32 = (uint32_t *)buff8;
	int len = buff32[offset32];
	uint64_t *base = malloc(sizeof(*base) * (len + 1));
	uint32_t *offset = malloc(sizeof(*offset) * len);
	uint64_t code = 0;
	uint32_t offs = 0;
	int i;
	for (i = 0; i < len; ++i) {
	    int N = buff32[offset32 + i + 1];
		base[i] = code << (62 - i);
		offset[i] = offs;
	    code = (code + N) << 1;
		offs += N;
	}
	base[i] = code << (62 - i); // needed in the decoder for termination
	decoder->base = base;
	decoder->offset = offset;
	decoder->symbols = buff8 + 4 * (offset32 + 1 + len);
}
#endif

struct node *huffman_ht_tree_transform(struct node *root, uint8_t *buff8, int offset8,
	    int (*transform)(int)) {
	uint32_t *buff32 = (uint32_t *)buff8;
	int offset32 = offset8 / 4;
	int len = buff32[offset32];
	int codes = 1; // wtf is this?
	int symbols = 4 * (1 + 2 * len);
	for (int j = 0; j < len; ++j) {
		insert(root, buff32[offset32 + codes + 2 * j],
			buff32[offset32 + codes + 2 * j + 1], transform(buff8[offset8 + symbols + j]));
	}
	return root;
}

struct node *huffman_ht_tree(struct node *root, uint8_t *buff8, int offset8) {
    return huffman_ht_tree_transform(root, buff8, offset8, identity);
}

struct node *huffman_ht_tree_case(struct node *root, uint8_t *buff8, int offset8) {
    return huffman_ht_tree_transform(root, buff8, offset8, tolower);
}

int huffman_decode_to(struct node *root, uint8_t *buff8, int start, struct baos *out) {
	struct node *n = root;
	for (int i = start; 1; ++i) {
		uint8_t b = buff8[i];
		for (int j = 0x80; j > 0; j >>= 1) {
			struct node *m = (((b & j) == 0) ? n->inner.right : n->inner.left);
			if (m->type == LEAF) {
				uint8_t s = m->symbol;
				baos_push(out, s);
				if (s == 0) {
					return i + 1;
				}
				n = root;
			} else {
				n = m;
			}
		}
	}
}

// tree-based decoding. Usually not the fastest method.
void huffman_decode_range(struct node *root, uint8_t *buff8, int start, int end, struct baos *out) {
	struct node *n = root;
	for (int i = start; i < end; ++i) {
		uint8_t b = buff8[i];

		struct node *m;

		// unrolled for speed
		m = (((b & 0x80) == 0) ? n->inner.right : n->inner.left);
		if (m->type == LEAF) {
			baos_push(out, m->symbol);
			n = root;
		} else {
			n = m;
		}
		m = (((b & 0x40) == 0) ? n->inner.right : n->inner.left);
		if (m->type == LEAF) {
			baos_push(out, m->symbol);
			n = root;
		} else {
			n = m;
		}
		m = (((b & 0x20) == 0) ? n->inner.right : n->inner.left);
		if (m->type == LEAF) {
			baos_push(out, m->symbol);
			n = root;
		} else {
			n = m;
		}
		m = (((b & 0x10) == 0) ? n->inner.right : n->inner.left);
		if (m->type == LEAF) {
			baos_push(out, m->symbol);
			n = root;
		} else {
			n = m;
		}
		m = (((b & 0x08) == 0) ? n->inner.right : n->inner.left);
		if (m->type == LEAF) {
			baos_push(out, m->symbol);
			n = root;
		} else {
			n = m;
		}
		m = (((b & 0x04) == 0) ? n->inner.right : n->inner.left);
		if (m->type == LEAF) {
			baos_push(out, m->symbol);
			n = root;
		} else {
			n = m;
		}
		m = (((b & 0x02) == 0) ? n->inner.right : n->inner.left);
		if (m->type == LEAF) {
			baos_push(out, m->symbol);
			n = root;
		} else {
			n = m;
		}
		m = (((b & 0x01) == 0) ? n->inner.right : n->inner.left);
		if (m->type == LEAF) {
			baos_push(out, m->symbol);
			n = root;
		} else {
			n = m;
		}
	}
}

// ref: "On the Implementation of Minimum Redundancy Prefix Codes" - Moffat
// Our codes are ordered differently, so the conditions in the loop are slightly
// different from the paper. Note: there's a generic method for converting non-canonical
// codes to canonical codes for the purpose of decoding, so should probably be doing
// that, instead. Not sure of a good reference.
void huffman_canonical_decode(struct decoder *decoder, uint8_t *buff8, int start, int end, struct baos *out) {
	uint64_t code = 0;
	int32_t len = -1;
	uint64_t *base = decoder->base;
	uint32_t *offset = decoder->offset;
	uint8_t *symbols = decoder->symbols;
	for (int i = start; i < end; ++i) {
		uint8_t byte = buff8[i];
#define CHECKBIT(j) \
		code = (j & byte) ? ((code << 1) | 1) : (code << 1); \
		len +=1; \
		if (code << 1 < base[len + 1]) { \
			baos_push(out, symbols[offset[len] + (uint32_t)(code - base[len])]); \
			code = 0; \
			len = -1; \
		}
		CHECKBIT(0x80)
		CHECKBIT(0x40)
		CHECKBIT(0x20)
		CHECKBIT(0x10)
		CHECKBIT(0x08)
		CHECKBIT(0x04)
		CHECKBIT(0x02)
		CHECKBIT(0x01)
	}
}

// ONE-SHIFT algorithm from Moffat and Turpin, 1997. Should be faster
// than CANONICAL-DECODE, above, but it's not. Need to debug further.
// It should be possible to easily extend this to use a table to speed
// up the initial length calculation, as per the paper, but it's unclear
// if the speed is currently limited by the length calc or managing the
// left-justified bit register.
#if 0
void huffman_canonical_decode2(struct decoder *decoder, uint8_t *buff8, int start, int end, struct baos *out) {
	uint64_t code = 0;
	uint64_t *base = decoder->base;
	uint32_t *offset = decoder->offset;
	uint8_t *symbols = decoder->symbols;
	// XXX Check performance implications of bswap.
	// XXX Doing 64 bit aligned reads has no effect on performance, which is weird.
	//     Might need to dig into the wasm instructions to see what's going on.
#ifdef ALIGN64
	int leading = (8 - ((size_t)buff8 + start) % 8) % 8; // bytes not on 64 bit read boundary
	uint64_t v = 0;
	for (size_t i = 0; i < leading; ++i) {
		v = (v << 8) | buff8[start + i];
	}
	v <<= 63 - 8 * leading; // reserve the high bit in v

	uint64_t *buff = (uint64_t*)(buff8 + start + leading);
	uint64_t *endp = buff + (end - start - leading) / 8;

	uint64_t w = bswap_64(*buff++);
	int needed = (63 - leading * 8);
	v |= w >> (64 - needed);
	w <<= needed;
	int r = 64 - needed;
#else
	uint64_t *buff = (uint64_t*)(buff8 + start);
	uint64_t *endp = buff + (end - start) / 8;
	uint64_t v = bswap_64(*buff++); // current 63 bits being decoded, left justified.
	uint64_t w = (v & 1) << 63;     // next 64 bit word from buffer.
	v >>= 1;
	int r = 1;                      // unconsidered bits remaining in w.
#endif


	uint64_t b;
	uint64_t bn;
	int len;
	while (1) {
		len = 1;
		b = base[0];
		bn = base[1];
		while (bn < v) {
			len +=1;
			b = bn;
			bn = base[len];
		}

		baos_push(out, symbols[offset[len - 1] + ((v - b) >> (63 - len))]);

		if (len <= r) {             // if we have enough bits in w, use them.
			v = (v << len) & 0x7FFFFFFFFFFFFFFFLL;
			v |= (w >> (64 - len));
			w <<= len;
			r -= len;
		} else {                    // get some more bits.
			v = (v << r) & 0x7FFFFFFFFFFFFFFFLL;
			v |= (w >> (64 - r));
			len -= r;
			if (buff == endp) {     // out of 64-bit words.
				break;
			}
			w = bswap_64(*buff++);
			r = 64;
			v = (v << len) & 0x7FFFFFFFFFFFFFFFLL;
			v |= (w >> (64 - len));
			w <<= len;
			r -= len;
		}
	}

	// handle remaining bytes
#ifdef ALIGN64
	int rem = (end - start - leading) % 8;
#else
	int rem = (end - start) % 8;
#endif
	r = rem * 8;
	w = 0;
	for (int i = end - rem; i < end; ++i) {
		w = (w << 8) | buff8[i];
	}
	w <<= 8 * (8 - rem);

	v = (v << len) & 0x7FFFFFFFFFFFFFFFLL;
	v |= (w >> (64 - len));
	w <<= len;
	r -= len;

	// with 'remainder' of -63 bits, we will have read all
	// of v.
	while (r > -63) {
		len = 1;
		b = base[0];
		bn = base[1];
		while (bn <= v) {
			len += 1;
			b = bn;
			bn = base[len];
		}
		if (len > r + 63) {
			break; // XXX shouldn't be necessary, but good for testing.
			// w/o this we get some more decoded junk at the end. We get
			// less decoded junk with the other decoders.
		}
		baos_push(out, symbols[offset[len - 1] + ((v - b) >> (63 - len))]);
		v = (v << len) & 0x7FFFFFFFFFFFFFFFLL;
		v |= (w >> (64 - len));
		w <<= len;
		r -= len;
	}
}
#endif
