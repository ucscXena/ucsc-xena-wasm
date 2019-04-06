#include <stdlib.h>
#include <math.h>
#include <stdint.h> // uint32_t
#include "fradix16.h"

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

// XXX single-threaded operation
static float *mm_result;
#define MEAN 0
#define MEDIAN 1
void fameanmedian_init() {
	mm_result = malloc(2 * sizeof(float));
}

static int first_val(float *arr, int n) {
	float *last = arr + n;
	float *p = arr;
	while (isnan(*p) && p < last) {
		++p;
	}
	return p - arr;
}

float *fameanmedian(float *vals, int n) {
	if (n == 0) {
		mm_result[MEDIAN] = NAN;
	} else {
		fradixSort16InPlace((uint32_t *)vals, n);
		int f = first_val(vals, n);
		mm_result[MEDIAN] = (vals[(f + n - 1) / 2] + vals[(f + n) / 2]) / 2;

		int count = n - f;
		if (count) {
			float sum = 0;
			vals += f;
			n -= f;
			while (n--) {
				sum += *(vals++);
			}
			mm_result[MEAN] = sum / count;
			return mm_result;
		}
	}
	mm_result[MEAN] = NAN;
	return mm_result;
}
