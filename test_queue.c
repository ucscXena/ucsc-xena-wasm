#include <stdio.h>
#include <check.h>
#include "queue.h"

START_TEST(test_array_queue)
{
	struct array_queue *aq = array_queue_new(5);

	ck_assert_int_eq(array_queue_count(aq), 0);
	array_queue_add(aq, (void *)12);
	ck_assert_int_eq(array_queue_count(aq), 1);
	ck_assert_int_eq((long)array_queue_take(aq), 12);
	ck_assert_int_eq(array_queue_count(aq), 0);
	array_queue_add(aq, (void *)1);
	array_queue_add(aq, (void *)2);
	array_queue_add(aq, (void *)3);
	array_queue_add(aq, (void *)4);
	ck_assert_int_eq(array_queue_count(aq), 4);
	array_queue_free(aq);
}
END_TEST

START_TEST(test_queue)
{
	struct queue *aq = queue_new(5);

	ck_assert_int_eq(queue_count(aq), 0);
	queue_add(aq, (void *)12);
	ck_assert_int_eq(queue_count(aq), 1);
	ck_assert_int_eq((long)queue_take(aq), 12);
	ck_assert_int_eq(queue_count(aq), 0);
	queue_add(aq, (void *)1);
	queue_add(aq, (void *)2);
	queue_add(aq, (void *)3);
	queue_add(aq, (void *)4);
	ck_assert_int_eq(queue_count(aq), 4);
	queue_free(aq);
}
END_TEST

void add_queue(TCase *tc) {
	tcase_add_test(tc, test_array_queue);
	tcase_add_test(tc, test_queue);
}
