#ifndef XENA_HFC_H
#define XENA_HFC_H

#include <stdint.h>
#include "hfc_struct.h"
#include "bytes.h"

struct hfc;


void hfc_init(struct hfc *, uint8_t *, size_t);
int hfc_count(struct hfc *);
struct hfc *hfc_new(uint8_t *, size_t);

void hfc_search(struct hfc *, int (*cmp)(const char *a, const char *b), int, char *, struct search_result *);

void hfc_free(struct hfc *);

struct hfc_iter *hfc_iter_init(struct hfc *hfc);
char *hfc_iter_next(struct hfc_iter *iter);
void hfc_iter_free(struct hfc_iter *iter);

struct bytes *hfc_compress(int count, uint8_t **strings);
struct hfc *hfc_merge(struct hfc *ha, struct hfc *hb);

#endif //XENA_HTC_H
