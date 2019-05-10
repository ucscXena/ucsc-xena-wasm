
struct node;

struct node *huffman_tree(struct node *root, uint8_t *buff8, int offset32);
struct node *huffman_ht_tree(struct node *root, uint8_t *buff8, int offset8);
int huffman_decode_to(struct node *root, uint8_t *buff8, int start, struct baos *out);
void huffman_decode_range(struct node *root, uint8_t *buff8, int start, int end, struct baos *out);
struct node *huffman_new(void);
void huffman_free(struct node *);
