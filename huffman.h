
struct node;

struct node *huffman_tree(struct node *root, uint8_t *buff8, int offset32);
struct node *huffman_ht_tree(struct node *root, uint8_t *buff8, int offset8);
struct node *huffman_ht_tree_case(struct node *root, uint8_t *buff8, int offset8);
int huffman_decode_to(struct node *root, uint8_t *buff8, int start, struct baos *out);
void huffman_decode_range(struct node *root, uint8_t *buff8, int start, int end, struct baos *out);
struct node *huffman_new(void);
void huffman_free(struct node *);

void dump_tree(struct node *root);

struct decoder {
	uint64_t *base;
	uint32_t *offset;
	uint8_t *symbols;
};

void huffman_decoder_init(struct decoder *decoder, uint8_t *buff8, int offset32);
void huffman_decoder_init_case(struct decoder *decoder, uint8_t *buff8, int offset32);
void huffman_canonical_decode(struct decoder *decoder, uint8_t *buff8, int start, int end, struct baos *out);
