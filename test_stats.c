#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <check.h>
#include "stats.h"
#include "fradix16.h"

START_TEST(test_fastats)
{
	fastats_init();
	fradixSort16_init();
	// Don't allocate on the stack, because valgrind can't find overflows
	// in that case.
	float *vals = malloc(8 * sizeof(float));
	vals[0] = -1.;
	vals[1] = NAN;
	vals[2] = -2.;
	vals[3] = NAN;
	vals[4] = -3.;
	vals[5] = -1.;
	vals[6] = -4.;
	vals[7] = -1.;

	float *r = fastats(vals, 8);
	ck_assert_msg(r[MIN] == -4, "Assertion min == -4 failed: min == %f", r[0]);
	ck_assert_msg(r[MAX] == -1, "Assertion max == -1 failed: max == %f", r[0]);
	ck_assert_msg(r[MEAN] == -2, "Assertion mean == -2 failed: mean == %f", r[0]);
	ck_assert_msg(r[MEDIAN] == -1.5, "Assertion median == -1.5 failed: median == %f", r[1]);

	ck_assert_float_eq_tol(r[SD], 1.1547, 0.01);
	ck_assert_float_eq_tol(r[P01], -3.95, 0.01);
	ck_assert_float_eq_tol(r[P99], -1., 0.01);
	ck_assert_float_eq_tol(r[P05], -3.75, 0.01);
	ck_assert_float_eq_tol(r[P95], -1., 0.01);
	ck_assert_float_eq_tol(r[P10], -3.5, 0.01);
	ck_assert_float_eq_tol(r[P90], -1., 0.01);
	ck_assert_float_eq_tol(r[P25], -2.75, 0.01);
	ck_assert_float_eq_tol(r[P75], -1., 0.01);
	ck_assert_float_eq_tol(r[P33], -2.35, 0.01);
	ck_assert_float_eq_tol(r[P66], -1., 0.01);
	free(vals);

	vals = malloc(2 * sizeof(float));
	vals[0] = NAN;
	vals[1] = NAN;
	r = fastats(vals, 2);
	ck_assert_msg(r[MIN] == INFINITY, "Assertion min is INFINITY failed: min == %f", r[0]);
	ck_assert_msg(r[MAX] == -INFINITY, "Assertion max is -INFINITY failed: max == %f", r[0]);
	ck_assert_msg(r[MEAN] != r[MEAN], "Assertion mean is nan failed: mean == %f", r[0]);
	ck_assert_msg(r[MEDIAN] != r[MEDIAN], "Assertion median is nan failed: median == %f", r[1]);
	free(vals);

	vals = malloc(0 * sizeof(float));
	r = fastats(vals, 0);
	ck_assert_msg(r[MIN] == INFINITY, "Assertion min is INFINITY failed: min == %f", r[0]);
	ck_assert_msg(r[MAX] == -INFINITY, "Assertion max is -INFINITY failed: max == %f", r[0]);
	ck_assert_msg(r[MEAN] != r[MEAN], "Assertion mean is nan failed: mean == %f", r[0]);
	ck_assert_msg(r[MEDIAN] != r[MEDIAN], "Assertion median is nan failed: median == %f", r[1]);
	free(vals);
}
END_TEST

void add_stats(TCase *tc) {
	tcase_add_test(tc, test_fastats);
}
