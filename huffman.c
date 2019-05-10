#include <stdint.h>
#include <stdlib.h>
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
