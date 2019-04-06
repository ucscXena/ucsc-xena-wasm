#include <stdlib.h>
#include <math.h>

// XXX single-threaded operation
static float *result;
#define MIN 0
#define MAX 1

void faminmax_init() {
	result = malloc(2 * sizeof(float));
}

float *faminmax(float *vals, int n) {
	float min = INFINITY;
	float max = -INFINITY;
	float v;
	while (n--) {
		v = *(vals++);
		if (v < min) {
			min = v;
		}
		if (v > max) {
			max = v;
		}
	}
	result[MIN] = min;
	result[MAX] = max;
	return result;
}
