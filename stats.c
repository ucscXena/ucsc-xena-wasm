#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h> // uint32_t
#include "fradix16.h"
#include "stats.h"

// XXX single-threaded operation
static float *mm_result;

void fastats_init() {
	mm_result = malloc(LAST * sizeof(float));
}

static int first_val(float *arr, int n) {
	float *last = arr + n;
	float *p = arr;
	while (p < last && isnan(*p)) {
		++p;
	}
	return p - arr;
}

static float famean(float *vals, int n) {
	float sum = 0;
	for (int i = 0; i < n; ++i) {
		sum += *(vals++);
	}
	return sum / n;
}

float fasd(float *vals, int n, float mean) {
    float *w = malloc(n * sizeof(float));
    for (int i = 0; i < n; ++i) {
	float d = vals[i] - mean;
	w[i] = d * d;
    }
    float r = sqrtf(famean(w, n));
    free(w);
    return r;
}

// https://en.wikipedia.org/wiki/Percentile
// second variant, C = 1, same as numpy.
float percentile(float *vals, int n, float p) {
    float x = (n - 1) * p;
    float fx = floorf(x);
    int i = (int)fx;
    return vals[i] + (i < n - 1 ? (x - fx) * (vals[i + 1] - vals[i]) : 0);
}

// modifies the input array
float *fastats(float *vals, int n) {
	for (int i = 0; i < LAST; ++i) {
	    mm_result[i] = NAN;
	}
	mm_result[MIN] = INFINITY;
	mm_result[MAX] = -INFINITY;
	if (n > 0) {
		fradixSort16InPlace((uint32_t *)vals, n);
		int f = first_val(vals, n);
		int count = n - f;

		if (count) {
			mm_result[MIN] = vals[f];
			mm_result[MAX] = vals[n - 1];
			mm_result[MEDIAN] =
				(vals[(f + n - 1) / 2] + vals[(f + n) / 2]) / 2;
			float mean = famean(vals + f, count);
			mm_result[MEAN] = mean;

			mm_result[SD] = fasd(vals + f, count, mean);

			mm_result[P01] = percentile(vals + f, count, 0.01);
			mm_result[P99] = percentile(vals + f, count, 0.99);
			mm_result[P05] = percentile(vals + f, count, 0.05);
			mm_result[P95] = percentile(vals + f, count, 0.95);
			mm_result[P10] = percentile(vals + f, count, 0.10);
			mm_result[P90] = percentile(vals + f, count, 0.90);
			mm_result[P25] = percentile(vals + f, count, 0.25);
			mm_result[P75] = percentile(vals + f, count, 0.75);
			mm_result[P33] = percentile(vals + f, count, 0.33);
			mm_result[P66] = percentile(vals + f, count, 0.66);
		}
	}
	return mm_result;
}
