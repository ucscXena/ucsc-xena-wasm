#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SIZE 4096

struct list {
	struct list *next;
	uint8_t bin[SIZE];
};

struct baos {
	uint32_t count;
	struct list *bins;
	struct list *bin;
};

static void add_bin(struct baos *baos) {
	baos->bin = malloc(sizeof(struct list));
	memset(baos->bin, 0, sizeof(struct list)); // XXX check this
	baos->bin->next = baos->bins;
	baos->bins = baos->bin;
}

struct baos *baos_new() {
	struct baos *baos = malloc(sizeof(struct baos));
	baos->count = 0;
	baos->bins = NULL;
	baos->bin = NULL;
	add_bin(baos);
	return baos;
}

int baos_count(struct baos *baos) {
	return baos->count;
}

void baos_push(struct baos *baos, uint8_t b) {
	baos->bin->bin[baos->count % SIZE] = b;
	baos->count += 1;
	if (baos->count % SIZE == 0) {
		add_bin(baos);
	}
}

static struct list *reverse(struct list *list) {
	struct list *rev = NULL;
	struct list *next;
	while (list) {
		next = list->next;
		list->next = rev;
		rev = list;
		list = next;
	}
	return rev;
}

// This destroys the baos, because we hammer all the list pointers during
// reverse(). So, clean it up here.
// Alternatively, we'd need to allocate a new list for reverse(), or
// double-link the lists, or use xor trick, or something.
uint8_t *baos_to_array(struct baos *baos) {
	uint8_t *out;
	int count = baos->count;
	if (count) {
		out = malloc(count);
		uint8_t *outp = out;
		struct list *list = reverse(baos->bins);
		while (list) {
			struct list *next = list->next;
			if (list->next) {
				memcpy(outp, list->bin, SIZE);
			} else {
				memcpy(outp, list->bin, count % SIZE);
			}
			outp += SIZE;
			free(list);
			list = next;
		}
	} else {
		out = NULL;
	}

	free(baos);
	return out;
}
