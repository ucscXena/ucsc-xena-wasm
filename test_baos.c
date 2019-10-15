#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // uint32_t
#include <string.h>
#include <check.h>
#include "baos.h"

#define _ck_assert_strn(X, OP, Y, n) do { \
  const char* _ck_x = (X); \
  const char* _ck_y = (Y); \
  ck_assert_msg(0 OP strncmp(_ck_y, _ck_x, n), \
    "Assertion '%s' failed: %s == \"%.*s\", %s == \"%.*s\"", #X" "#OP" "#Y, #X, n, _ck_x, #Y, n, _ck_y); \
} while (0)


#define ck_assert_strn_eq(X, Y, n) _ck_assert_strn(X, ==, Y, n)

START_TEST(test_empty)
{
	struct baos *baos;
	char *result;
	int len;


	baos = baos_new();
	len = baos_count(baos);
	result = baos_to_array(baos);
	ck_assert_int_eq(len, 0);
	ck_assert_ptr_eq(result, NULL);
	free(result);
}
END_TEST

START_TEST(test_one)
{
	struct baos *baos;
	char *result;
	int len;
	char *s = "abcdef";


	baos = baos_new();
	baos_push(baos, s[0]);
	len = baos_count(baos);
	result = baos_to_array(baos);

	ck_assert_int_eq(len, 1);
	ck_assert_ptr_ne(result, NULL);
	ck_assert_strn_eq(result, "a", 1);

	free(result);
}
END_TEST

START_TEST(test_boundary)
{
	struct baos *baos;
	char *result;
	int len;
	char *s = "abcdef";


	for (int N = 4095; N < 4099; ++N) {
		uint8_t *expected;

		baos = baos_new();
		for (int i = 0; i < N; ++i) {
			baos_push(baos, s[i % sizeof(s)]);
		}
		len = baos_count(baos);
		result = baos_to_array(baos);
		ck_assert_int_eq(len, N);
		ck_assert_ptr_ne(result, NULL);

		expected = malloc(N);
		for (int i = 0; i < N; ++i) {
			expected[i] = s[i % sizeof(s)];
		}
		ck_assert_strn_eq(result, "ab", 2);
		free(expected);
		free(result);
	}
}
END_TEST

void add_baos(TCase *tc) {
	tcase_add_test(tc, test_empty);
	tcase_add_test(tc, test_one);
	tcase_add_test(tc, test_boundary);
}
