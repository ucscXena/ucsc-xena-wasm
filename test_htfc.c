#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <check.h>
#include "htfc.h"

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
	sprintf(filename, "data/htfc%d-test.dict", i);
	FILE *fp = fopen(filename, "r");
	ck_assert_ptr_ne(fp, NULL);
	uint8_t *buff = malloc(BUFSIZE);
	int len = fread(buff, 1, BUFSIZE, fp);
	struct htfc *htfc = htfc_new(buff, len);
	char expected[500];
	int *o = order(i);

	ck_assert_int_eq(htfc_count(htfc), i);

	struct htfc_iter *iter = htfc_iter_init(htfc);
	char *s;

	for (int j = 0; j < i; ++j) {
		sprintf(expected, "sample%d", o[j]);
		s = htfc_iter_next(iter);
		ck_assert_str_eq(s, expected);
	}

cleanup:
	htfc_iter_free(iter);
	htfc_free(htfc);
	free(buff);
	free(o);
	fclose(fp);
}

START_TEST(test_basic)
{
	FILE *fp = fopen("data/htfc-basic-test.dict", "r");
	ck_assert_ptr_ne(fp, NULL);
	uint8_t *buff = malloc(BUFSIZE);
	int len = fread(buff, 1, BUFSIZE, fp);
	struct htfc *htfc = htfc_new(buff, len);
	struct search_result result;

	ck_assert_int_eq(htfc_count(htfc), 4);

	htfc_search(htfc, contains, 0, "wo", &result);

	ck_assert_int_eq(result.count, 1);
	ck_assert_int_eq(result.matches[0], 3);

	free(result.matches);
	htfc_free(htfc);
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

void add_htfc(TCase *tc) {
	tcase_add_test(tc, test_basic);
	tcase_add_test(tc, test_boundaries);
}
