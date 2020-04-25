#include <stdlib.h>
#include <math.h>
#include <check.h>
#include "fradix16-64.h"

START_TEST(positives)
{
	float c0[] = {3.0, 2.0, 4.0, 1.0};
	float *cols[] = {c0};
	int indicies[] = {0, 1, 2, 3};
	direction dirs[] = {DOWN};
	fradixSort16_64_init();
	fradixSortL16_64((uint32_t**)cols, dirs, sizeof(cols) / sizeof(cols[0]),
			sizeof(c0) / sizeof(c0[0]), indicies);
	ck_assert_float_eq(c0[indicies[0]], 4.0);
	ck_assert_float_eq(c0[indicies[1]], 3.0);
	ck_assert_float_eq(c0[indicies[2]], 2.0);
	ck_assert_float_eq(c0[indicies[3]], 1.0);
}
END_TEST

START_TEST(negatives)
{
	float c0[] = {-3.0, -2.0, -4.0, -1.0};
	float *cols[] = {c0};
	int indicies[] = {0, 1, 2, 3};
	direction dirs[] = {DOWN};
	fradixSort16_64_init();
	fradixSortL16_64((uint32_t**)cols, dirs, sizeof(cols) / sizeof(cols[0]),
			sizeof(c0) / sizeof(c0[0]), indicies);
	ck_assert_float_eq(c0[indicies[0]], -1.0);
	ck_assert_float_eq(c0[indicies[1]], -2.0);
	ck_assert_float_eq(c0[indicies[2]], -3.0);
	ck_assert_float_eq(c0[indicies[3]], -4.0);
}
END_TEST

START_TEST(pos_neg)
{
	float c0[] = {3.0, -2.0, 4.0, -1.0};
	float *cols[] = {c0};
	int indicies[] = {0, 1, 2, 3};
	direction dirs[] = {DOWN};
	fradixSort16_64_init();
	fradixSortL16_64((uint32_t**)cols, dirs, sizeof(cols) / sizeof(cols[0]),
			sizeof(c0) / sizeof(c0[0]), indicies);
	ck_assert_float_eq(c0[indicies[0]], 4.0);
	ck_assert_float_eq(c0[indicies[1]], 3.0);
	ck_assert_float_eq(c0[indicies[2]], -1.0);
	ck_assert_float_eq(c0[indicies[3]], -2.0);
}
END_TEST

START_TEST(pos_neg_nan)
{
	float c0[] = {3.0, -2.0, NAN, 4.0, -1.0};
	float *cols[] = {c0};
	int indicies[] = {0, 1, 2, 3, 4};
	direction dirs[] = {DOWN};
	fradixSort16_64_init();
	fradixSortL16_64((uint32_t**)cols, dirs, sizeof(cols) / sizeof(cols[0]),
			sizeof(c0) / sizeof(c0[0]), indicies);
	ck_assert_float_eq(c0[indicies[0]], 4.0);
	ck_assert_float_eq(c0[indicies[1]], 3.0);
	ck_assert_float_eq(c0[indicies[2]], -1.0);
	ck_assert_float_eq(c0[indicies[3]], -2.0);
	ck_assert_float_nan(c0[indicies[4]]);
}
END_TEST

START_TEST(pos_neg_nan_large)
{
	int n = 20 * 5;
	float *c0 = malloc(sizeof(float) * n);
	for (int i = 0; i < n; i += 5) {
		c0[i] = 3.0;
		c0[i + 1] = -2.0;
		c0[i + 2] = NAN;
		c0[i + 3] = 4.0;
		c0[i + 4] = -1.0;
	}
	float *cols[] = {c0};
	int *indicies = malloc(sizeof(int) * n);
	for (int i = 0; i < n; ++i) {
		indicies[i] = i;
	}
	direction dirs[] = {DOWN};
	fradixSort16_64_init();
	fradixSortL16_64((uint32_t**)cols, dirs, 1, n, indicies);
	for (int i = 0; i < 20; ++i) {
		ck_assert_float_eq(c0[indicies[i]], 4.0);
	}
	for (int i = 20; i < 40; ++i) {
		ck_assert_float_eq(c0[indicies[i]], 3.0);
	}
	for (int i = 40; i < 60; ++i) {
		ck_assert_float_eq(c0[indicies[i]], -1.0);
	}
	for (int i = 60; i < 80; ++i) {
		ck_assert_float_eq(c0[indicies[i]], -2.0);
	}
	for (int i = 80; i < 100; ++i) {
		ck_assert_float_nan(c0[indicies[i]]);
	}
}

void add_sort(TCase *tc) {
	tcase_add_test(tc, positives);
	tcase_add_test(tc, negatives);
	tcase_add_test(tc, pos_neg);
	tcase_add_test(tc, pos_neg_nan);
	tcase_add_test(tc, pos_neg_nan_large);
}
