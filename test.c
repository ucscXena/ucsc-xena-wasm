#include <stdlib.h>
#include <check.h>

int main(int argc, char *argv[]) {
	int number_failed;
	Suite *s = suite_create("xena");
	TCase *tc;

#define TEST(x) \
	do {\
		void add_ ## x(TCase *s); \
		tc = tcase_create(#x); \
		add_ ## x(tc); \
		suite_add_tcase(s, tc); \
	} while (0)


	TEST(baos);
	TEST(queue);
	TEST(stats);
	TEST(huffman);
	TEST(htfc);

	SRunner *sr = srunner_create(s);
//	srunner_set_fork_status(sr, CK_NOFORK);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
