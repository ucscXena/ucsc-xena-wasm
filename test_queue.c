#include <stdio.h>
#include "queue.h"

int main(int argc, char *argv[]) {
	printf("hello\n");
	struct array_queue *aq = array_queue_new(5);
	printf("init count %d\n", array_queue_count(aq));
	array_queue_add(aq, (void *)12);
	printf("one count %d\n", array_queue_count(aq));
	long x = (long)array_queue_take(aq);
	printf("took %ld, expect 12\n", x);
	printf("empty count %d\n", array_queue_count(aq));
	array_queue_add(aq, (void *)1);
	array_queue_add(aq, (void *)2);
	array_queue_add(aq, (void *)3);
	array_queue_add(aq, (void *)4);
	printf("four count %d\n", array_queue_count(aq));

	struct queue *q = queue_new(5);
	printf("init count %d\n", queue_count(q));
	queue_add(q, (void *)12);
	printf("one count %d\n", queue_count(q));
	long y = (long)queue_take(q);
	printf("took %ld, expect 12\n", y);
	printf("empty count %d\n", queue_count(q));
	queue_add(q, (void *)1);
	queue_add(q, (void *)2);
	queue_add(q, (void *)3);
	queue_add(q, (void *)4);
	printf("four count %d\n", queue_count(q));

}
