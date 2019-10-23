#ifndef XENA_HUFFMAN_H
#define XENA_HUFFMAN_H

#include <stdint.h>
#include "baos.h"
#include "queue.h"


// tree decoder. Only used for hu-tucker.
struct node;
struct node *huffman_ht_tree(struct node *root, uint8_t *buff8, int offset8);
struct node *huffman_ht_tree_case(struct node *root, uint8_t *buff8, int offset8);
int huffman_decode_to(struct node *root, uint8_t *buff8, int start, struct baos *out);
void huffman_decode_range(struct node *root, uint8_t *buff8, int start, int end, struct baos *out);
struct node *huffman_new(void);
void huffman_free(struct node *);

// optimized decoder
struct decoder {
	uint64_t *base;
	uint32_t *offset;
	uint8_t *symbols;
};
void huffman_decoder_init(struct decoder *decoder, uint8_t *buff8, int offset32);
void huffman_decoder_init_case(struct decoder *decoder, uint8_t *buff8, int offset32);
void huffman_decoder_free(struct decoder *decoder);
void huffman_canonical_decode(struct decoder *decoder, uint8_t *buff8, int start, int end, struct baos *out);

// encoder
#define NBYTE 256
int *byte_freqs(int count, struct bytes **bin);
struct huffman_encoder *huffman_bytes_encoder(int count, struct bytes **bins);
void huffman_serialize(struct baos *out, struct huffman_encoder *huff);
void huffman_encoder_free(struct huffman_encoder *enc);
void huffman_encode_bytes(struct baos *output, struct huffman_encoder *enc, int len, uint8_t *in);

#endif //XENA_HUFFMAN_H
