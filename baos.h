struct baos;

void baos_push(struct baos *baos, uint8_t b);
uint8_t *baos_to_array(struct baos *baos);
struct baos *baos_new();
int baos_count(struct baos *baos);