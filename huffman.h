
struct node;

struct node *huffman_tree(struct node *root, uint8_t *buff8, int offset32);
struct node *huffman_ht_tree(struct node *root, uint8_t *buff8, int offset8);
struct node *huffman_ht_tree_case(struct node *root, uint8_t *buff8, int offset8);
int huffman_decode_to(struct node *root, uint8_t *buff8, int start, struct baos *out);
void huffman_decode_range(struct node *root, uint8_t *buff8, int start, int end, struct baos *out);

// tree decoder
struct node *huffman_new(void);
void huffman_free(struct node *);

void dump_tree(struct node *root);

struct decoder {
	uint64_t *base;
	uint32_t *offset;
	uint8_t *symbols;
};

// optimized decoder
void huffman_decoder_init(struct decoder *decoder, uint8_t *buff8, int offset32);
void huffman_decoder_init_case(struct decoder *decoder, uint8_t *buff8, int offset32);
void huffman_decoder_free(struct decoder *decoder);
void huffman_canonical_decode(struct decoder *decoder, uint8_t *buff8, int start, int end, struct baos *out);

// encoder
struct encode_tree *encode_tree_build(int *freqs);
void encode_tree_free(struct encode_tree *tree);
void encode_tree_dump(struct encode_tree *tree);

#define NBYTE 256
int *byte_freqs(int count, char **s);
struct huffman_encoder *strings_encoder(int count, char **s);
void huffman_serialize(struct baos *out, struct huffman_encoder *huff);
void huffman_encoder_free(struct huffman_encoder *enc);
void encode_bytes(struct baos *output, struct huffman_encoder *enc, int len, uint8_t *in);
