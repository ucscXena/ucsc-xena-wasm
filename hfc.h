#ifndef XENA_HFC_H
#define XENA_HFC_H

#include <stdint.h>
#include "hfc_struct.h"
#include "bytes.h"

struct hfc;


void hfc_init(struct hfc *, uint8_t *, size_t);
int hfc_count(struct hfc *);
struct hfc *hfc_new(uint8_t *, size_t);

void hfc_search_method(struct hfc *, int (*cmp)(const char *a, const char *b), int, char *, struct search_result *);

void hfc_free(struct hfc *);

struct hfc_iter *hfc_iter_init(struct hfc *hfc);
char *hfc_iter_next(struct hfc_iter *iter);
void hfc_iter_free(struct hfc_iter *iter);

struct bytes *hfc_compress(int count, char **strings);
struct hfc *hfc_merge_two(struct hfc *ha, struct hfc *hb);

// js API
void hfc_set(uint8_t *buff, uint32_t len);
void hfc_set_empty();
void hfc_merge(uint8_t *buff, uint32_t len);
char *hfc_lookup(int i);
struct search_result *hfc_search(char *substring, enum search_type type);
void hfc_filter();
struct hfc *hfc_get_cache();
void hfc_clear_cache(); // inner cache only
int hfc_length();

#endif //XENA_HTC_H
