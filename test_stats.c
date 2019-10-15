#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <check.h>
#include "stats.h"
#include "fradix16.h"

START_TEST(test_fmeanmedian)
{
	fameanmedian_init();
	fradixSort16_init();
	float vals[] = {-1., NAN, -2., NAN, -3., -1., -4., -1.};

	float *r = fameanmedian(vals, 8);
	ck_assert_msg(r[0] == -2, "Assertion mean == -2 failed: mean == %f", r[0]);
	ck_assert_msg(r[1] == -1.5, "Assertion median == -1.5 failed: median == %f", r[1]);

	vals[0] = NAN;
	vals[1] = NAN;
	r = fameanmedian(vals, 2);
	ck_assert_msg(r[0] != r[0], "Assertion mean is nan failed: mean == %f", r[0]);
	ck_assert_msg(r[1] != r[1], "Assertion median is nan failed: median == %f", r[1]);

	r = fameanmedian(vals, 0);
	ck_assert_msg(r[0] != r[0], "Assertion mean is nan failed: mean == %f", r[0]);
	ck_assert_msg(r[1] != r[1], "Assertion median is nan failed: median == %f", r[1]);
}
END_TEST

START_TEST(test_faminmax)
{
	faminmax_init();
	float vals2[] = {-2., -3., -4., -1.};
	float *r = faminmax(vals2, sizeof(vals2) / sizeof(float));
	ck_assert_msg(r[0] == -4, "Assertion min is -4 failed: min == %f", r[0]);
	ck_assert_msg(r[1] == -1, "Assertion max is -1 failed: max == %f", r[1]);
}
END_TEST

void add_stats(TCase *tc) {
	tcase_add_test(tc, test_fmeanmedian);
	tcase_add_test(tc, test_faminmax);
}
