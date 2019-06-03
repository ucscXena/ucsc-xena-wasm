#include <stdint.h>
#include <stdlib.h>
#ifdef HUFFMAN_DEBUG
#include <ctype.h>
#endif
#include <byteswap.h>
#include "baos.h"
#include "huffman.h"

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

// for canonical decoder. offset, and right-justified base
void huffman_decoder_init(struct decoder *decoder, uint8_t *buff8, int offset32) {
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
	decoder->symbols = buff8 + 4 * (offset32 + 1 + len);
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

struct node *huffman_ht_tree(struct node *root, uint8_t *buff8, int offset8) {
	uint32_t *buff32 = (uint32_t *)buff8;
	int offset32 = offset8 / 4;
	int len = buff32[offset32];
	int codes = 1; // wtf is this?
	int symbols = 4 * (1 + 2 * len);
	for (int j = 0; j < len; ++j) {
		insert(root, buff32[offset32 + codes + 2 * j],
			buff32[offset32 + codes + 2 * j + 1], buff8[offset8 + symbols + j]);
	}
	return root;
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
// It should be possible to easily extend this to use a table too speed
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
