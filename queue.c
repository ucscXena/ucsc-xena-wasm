#include <stdlib.h>
// FIFO

struct el {
	struct el *next;
	void *value;
};

struct queue {
	int count;
	struct el *head;
	struct el *tail;
};

struct queue *queue_new() {
	struct queue *queue = malloc(sizeof(struct queue));
	queue->head = NULL;
	queue->tail = NULL;
	queue->count = 0;
	return queue;
}

void queue_add(struct queue *queue, void *value) {
	struct el *node = malloc(sizeof(struct el));
	node->next = NULL;
	node->value = value;
	if (queue->tail) {
		queue->tail->next = node;
		queue->tail = node;
	} else {
		queue->head = node;
		queue->tail = node;
	}
	queue->count++;
}

void *queue_peek(struct queue *queue) {
	return queue->head->value;
}

void *queue_take(struct queue *queue) {
	struct el *head = queue->head;
	if (!queue->head) {
		return NULL;
	}
	void *value = head->value;
	queue->head = head->next;
	if (queue->head == NULL) {
		queue->tail = NULL;
	}
	free(head);
	queue->count--;
	return value;
}

int queue_count(struct queue *queue) {
	return queue->count;
}

void queue_free(struct queue *queue) {
	while (queue_take(queue)) {
		// XXX leaks values?
	}
	free(queue);
}

struct array_queue {
	void **arr;
	int size;
	int head;
	int tail;
};

struct array_queue *array_queue_new(int size) {
	struct array_queue *queue = malloc(sizeof(struct array_queue));
	// adding 1 to avoid aliasing full and empty
	queue->arr = calloc(size + 1, sizeof(void *));
	queue->size = size + 1;
	queue->head = 0;
	queue->tail = 0;
	return queue;
}

void array_queue_add(struct array_queue *queue, void *value) {
	int size = queue->size;
	queue->arr[queue->tail] = value;
	queue->tail = (queue->tail + 1) % size;
}

void *array_queue_take(struct array_queue *queue) {
	if (queue->head == queue->tail) {
		return NULL;
	}
	int size = queue->size;
	void *value = queue->arr[queue->head];
	queue->head = (queue->head + 1) % size;
	return value;
}

void *array_queue_peek(struct array_queue *queue) {
	return (queue->head == queue->tail) ? NULL :
		queue->arr[queue->head];
}

int array_queue_count(struct array_queue *queue) {
	return queue->head > queue->tail ?
		queue->size + queue->tail - queue->head :
		queue->tail - queue->head;
}

int array_queue_free(struct array_queue *queue) {
	free(queue->arr);
	free(queue);
}
