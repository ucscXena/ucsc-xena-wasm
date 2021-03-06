#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <check.h>
#include "hfc.h"
#include "bytes.h"

#define BUFSIZE 1000

static int contains(const char *a, const char *b) {
	return strstr(a, b) ? 1 : 0;
}

static int intstrcmp(const void *a, const void *b) {
	char astr[100];
	char bstr[100];
	sprintf(astr, "%d", *(const int *)a);
	sprintf(bstr, "%d", *(const int *)b);
	return strcmp(astr, bstr);
}

static int *order(int n) {
	int *r = malloc(n * sizeof(int));
	for (int i = 0; i < n; ++i) {
		r[i] = i;
	}
	qsort(r, n, sizeof(int), intstrcmp);
	return r;
}

static char filename[500];
static void test_dict(int i) {
	sprintf(filename, "data/hfc%d-test.dict", i);
	FILE *fp = fopen(filename, "r");
	ck_assert_ptr_ne(fp, NULL);
	uint8_t *buff = malloc(BUFSIZE);
	int len = fread(buff, 1, BUFSIZE, fp);
	struct hfc *hfc = hfc_new(buff, len);
	char expected[500];
	int *o = order(i);

	ck_assert_int_eq(hfc_count(hfc), i);

	struct hfc_iter *iter = hfc_iter_init(hfc);
	char *s;

	for (int j = 0; j < i; ++j) {
		sprintf(expected, "sample%d", o[j]);
		s = hfc_iter_next(iter);
		ck_assert_str_eq(s, expected);
	}

cleanup:
	hfc_iter_free(iter);
	hfc_free(hfc);
	free(o);
	fclose(fp);
}

START_TEST(test_basic)
{
	FILE *fp = fopen("data/hfc-basic-test.dict", "r");
	ck_assert_ptr_ne(fp, NULL);
	uint8_t *buff = malloc(BUFSIZE);
	int len = fread(buff, 1, BUFSIZE, fp);
	struct hfc *hfc = hfc_new(buff, len);
	struct search_result result;

	ck_assert_int_eq(hfc_count(hfc), 4);

	hfc_search_method(hfc, contains, 0, "wo", &result);

	ck_assert_int_eq(result.count, 1);
	ck_assert_int_eq(result.matches[0], 3);

	free(result.matches);
	hfc_free(hfc);
	fclose(fp);
}
END_TEST

START_TEST(test_boundaries)
{
	test_dict(2);
	test_dict(255);
	test_dict(256);
	test_dict(257);
}
END_TEST

START_TEST(test_encode_basic)
{
	char *strings[] = {"foo", "bar", "baz"};
	struct bytes *b = hfc_compress(3, strings);

	struct hfc *hfc = hfc_new(b->bytes, b->len);
	ck_assert_int_eq(hfc_count(hfc), 3);
	struct hfc_iter *it = hfc_iter_init(hfc);
	ck_assert_str_eq(hfc_iter_next(it), "bar");
	ck_assert_str_eq(hfc_iter_next(it), "baz");
	ck_assert_str_eq(hfc_iter_next(it), "foo");
	hfc_free(hfc);
	free(b);
	hfc_iter_free(it);
}
END_TEST

START_TEST(test_empty)
{
	char *strings[] = {};
	struct bytes *b = hfc_compress(0, strings);
	struct hfc *hfc = hfc_new(b->bytes, b->len);
	ck_assert_int_eq(hfc_count(hfc), 0);
	struct hfc_iter *it = hfc_iter_init(hfc);
	ck_assert_ptr_eq(hfc_iter_next(it), NULL);
	hfc_iter_free(it);
	hfc_free(hfc);
	free(b);
}
END_TEST

START_TEST(test_one)
{
	char *strings[] = {"foo"};
	struct bytes *b = hfc_compress(1, strings);
	struct hfc *hfc = hfc_new(b->bytes, b->len);
	ck_assert_int_eq(hfc_count(hfc), 1);
	struct hfc_iter *it = hfc_iter_init(hfc);
	ck_assert_str_eq(hfc_iter_next(it), "foo");
	ck_assert_ptr_eq(hfc_iter_next(it), NULL);
	hfc_iter_free(it);
	hfc_free(hfc);
	free(b);
}
END_TEST

START_TEST(test_merge)
{
	char *strings0[] = {"foo", "bar", "baz"};
	struct bytes *b0 = hfc_compress(3, strings0);
	struct hfc *hfc0 = hfc_new(b0->bytes, b0->len);

	char *strings1[] = {"foo", "bin", "baz"};
	struct bytes *b1 = hfc_compress(3, strings1);
	struct hfc *hfc1 = hfc_new(b1->bytes, b1->len);

	struct hfc *merged = hfc_merge_two(hfc0, hfc1);

	ck_assert_int_eq(hfc_count(merged), 4);
	struct hfc_iter *it = hfc_iter_init(merged);
	ck_assert_str_eq(hfc_iter_next(it), "bar");
	ck_assert_str_eq(hfc_iter_next(it), "baz");
	ck_assert_str_eq(hfc_iter_next(it), "bin");
	ck_assert_str_eq(hfc_iter_next(it), "foo");
	hfc_free(merged);
	hfc_free(hfc0);
	hfc_free(hfc1);
	free(b0);
	free(b1);
	hfc_iter_free(it);
}
END_TEST

START_TEST(test_filter)
{
	char *strings0[] = {"one", "two", "three", "four", "five", "six", "seven", "eight"};

	struct bytes *b0 = hfc_compress(8, strings0);
	hfc_set(b0->bytes, b0->len);
	free(b0);
	char *x = strdup("i");
	struct search_result *result = hfc_search(x, CONTAINS);
	hfc_filter(result->matches, result->count);
	struct hfc_iter *it = hfc_iter_init(hfc_get_cache());
	// sorted: eight five four one seven six three two
	// matches: 0, 1, 5
	ck_assert_str_eq(hfc_iter_next(it), "eight");
	ck_assert_str_eq(hfc_iter_next(it), "five");
	ck_assert_str_eq(hfc_iter_next(it), "six");
	hfc_iter_free(it);
	free(x);
	hfc_clear_cache();
}
END_TEST

START_TEST(test_larger_filter)
{
	int count = 512;
	char *strings0[count];
	for (int i = 0; i < count; ++i) {
		strings0[i] = malloc(4);
		sprintf(strings0[i], "%d", i);
	}

	struct bytes *b0 = hfc_compress(count, strings0);
	hfc_set(b0->bytes, b0->len);
	free(b0);
	char *x = strdup("78");
	struct search_result *result = hfc_search(x, CONTAINS);
	hfc_filter(result->matches, result->count);
	struct hfc_iter *it = hfc_iter_init(hfc_get_cache());
	ck_assert_str_eq(hfc_iter_next(it), "178");
	ck_assert_str_eq(hfc_iter_next(it), "278");
	ck_assert_str_eq(hfc_iter_next(it), "378");
	ck_assert_str_eq(hfc_iter_next(it), "478");
	ck_assert_str_eq(hfc_iter_next(it), "78");
	hfc_iter_free(it);
	free(x);
	hfc_clear_cache();
	for (int i = 0; i < count; ++i) {
		free(strings0[i]);
	}
}
END_TEST

START_TEST(test_lookup)
{
	char *strings0[] = {"one", "two", "three", "four", "five", "six", "seven", "eight"};

	struct bytes *b0 = hfc_compress(8, strings0);
	hfc_set(b0->bytes, b0->len);
	free(b0);
	// sorted: eight five four one seven six three two
	ck_assert_str_eq(hfc_lookup(0), "eight");
	ck_assert_str_eq(hfc_lookup(1), "five");
	ck_assert_str_eq(hfc_lookup(2), "four");
	ck_assert_str_eq(hfc_lookup(3), "one");
	ck_assert_str_eq(hfc_lookup(4), "seven");
	ck_assert_str_eq(hfc_lookup(5), "six");
	ck_assert_str_eq(hfc_lookup(6), "three");
	ck_assert_str_eq(hfc_lookup(7), "two");
	hfc_clear_cache();
}
END_TEST

START_TEST(test_set_empty)
{
	hfc_set_empty();
	ck_assert_int_eq(hfc_length(), 0);
}
END_TEST


// we keep a static reference to the current sample list.
// Testing some operations on that object.
START_TEST(test_singleton)
{
	hfc_set_empty();
	ck_assert_int_eq(hfc_length(), 0);

	char *strings0[] = {"foo", "bar", "baz"};
	struct bytes *b0 = hfc_compress(3, strings0);
	hfc_merge(b0->bytes, b0->len);
	ck_assert_int_eq(hfc_length(), 3);

	char *strings1[] = {"foo", "bin", "baz"};
	struct bytes *b1 = hfc_compress(3, strings1);
	hfc_merge(b1->bytes, b1->len);
	ck_assert_int_eq(hfc_length(), 4);
	free(b0);
	free(b1);
}
END_TEST

void add_hfc(TCase *tc) {
	tcase_add_test(tc, test_basic);
	tcase_add_test(tc, test_boundaries);
	tcase_add_test(tc, test_encode_basic);
	tcase_add_test(tc, test_empty);
	tcase_add_test(tc, test_one);
	tcase_add_test(tc, test_merge);
	tcase_add_test(tc, test_filter);
	tcase_add_test(tc, test_larger_filter);
	tcase_add_test(tc, test_lookup);
	tcase_add_test(tc, test_set_empty);
	tcase_add_test(tc, test_singleton);
}
