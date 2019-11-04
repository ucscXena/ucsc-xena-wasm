#ifndef XENA_ARRAY_H
#define XENA_ARRAY_H
// growable string array

struct array *array_new();
void array_add(struct array *a, char *s);
void array_free(struct array *a);
struct array {
	char **arr;
	int length;
};

#endif //XENA_ARRAY_H

