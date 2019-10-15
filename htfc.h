#ifndef XENA_HTFC_H
#define XENA_HTFC_H

#include <stdint.h>
#include "htfc_struct.h"

struct htfc;


void htfc_init(struct htfc *, uint8_t *, size_t);
int htfc_count(struct htfc *);
struct htfc *htfc_new(uint8_t *, size_t);

void htfc_search(struct htfc *, int (*cmp)(const char *a, const char *b), int, char *, struct search_result *);

void htfc_free(struct htfc *);

struct htfc_iter *htfc_iter_init(struct htfc *htfc);
char *htfc_iter_next(struct htfc_iter *iter);
void htfc_iter_free(struct htfc_iter *iter);

#endif //XENA_HTFC_H
