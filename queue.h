#ifndef XENA_QUEUE_H
#define XENA_QUEUE_H

struct queue *queue_new();
void queue_add(struct queue *queue, void *value);
void *queue_peek(struct queue *queue);
void *queue_take(struct queue *queue);
void queue_free(struct queue *queue);
int queue_count(struct queue *queue);
struct queue *queue_copy(struct queue *queue);
typedef struct el *queue_iter;
void queue_iter_init(struct queue *queue, queue_iter *);
int queue_iter_next(queue_iter *);
void *queue_iter_value(queue_iter);
#ifdef DEBUG
void queue_dump(struct queue *queue);
#endif

struct array_queue *array_queue_new();
void array_queue_add(struct array_queue *array_queue, void *value);
void *array_queue_peek(struct array_queue *array_queue);
void *array_queue_take(struct array_queue *array_queue);
void array_queue_free(struct array_queue *array_queue);
int array_queue_count(struct array_queue *queue);

#endif //XENA_QUEUE_H
