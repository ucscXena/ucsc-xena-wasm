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

#endif //XENA_HTFC_H
