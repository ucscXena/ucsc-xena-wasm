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
	sprintf(filename, "hfc%d-test.dict", i);
	FILE *fp = fopen(filename, "r");
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
	free(buff);
	free(o);
	fclose(fp);
}

START_TEST(test_basic)
{
	FILE *fp = fopen("hfc-basic-test.dict", "r");
	uint8_t *buff = malloc(BUFSIZE);
	int len = fread(buff, 1, BUFSIZE, fp);
	struct hfc *hfc = hfc_new(buff, len);
	struct search_result result;

	ck_assert_int_eq(hfc_count(hfc), 4);

	hfc_search(hfc, contains, 0, "wo", &result);

	ck_assert_int_eq(result.count, 1);
	ck_assert_int_eq(result.matches[0], 3);

	free(result.matches);
	hfc_free(hfc);
	free(buff);
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
	uint8_t *strings[] = {"foo", "bar", "baz"};
	struct bytes *b = hfc_compress(3, strings);

	struct hfc *hfc = hfc_new(b->bytes, b->len);
	ck_assert_int_eq(hfc_count(hfc), 3);
	struct hfc_iter *it = hfc_iter_init(hfc);
	ck_assert_str_eq(hfc_iter_next(it), "foo");
	ck_assert_str_eq(hfc_iter_next(it), "bar");
	ck_assert_str_eq(hfc_iter_next(it), "baz");
	hfc_free(hfc);
	bytes_free(b);
}
END_TEST

void add_hfc(TCase *tc) {
	tcase_add_test(tc, test_basic);
	tcase_add_test(tc, test_boundaries);
	tcase_add_test(tc, test_encode_basic);
}