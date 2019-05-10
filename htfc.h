struct htfc;


void htfc_init(struct htfc *, uint8_t *, size_t);
int htfc_count(struct htfc *);
struct htfc *htfc_new(uint8_t *, size_t);
struct search_result {
	uint32_t *matches;
	size_t count;
};
void htfc_search(struct htfc *, char *, struct search_result *);

void htfc_free(struct htfc *);
