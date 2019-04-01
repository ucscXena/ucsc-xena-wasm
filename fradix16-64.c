#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// A utility function to print an array
void print(float arr[], int n) {
	int i;
	for (i = 0; i < n; i++)  {
		printf("%g ", arr[i]);
	}
	printf("\n");
}

void printi(uint arr[], int n) {
	for (int i = 0; i < n; i++)  {
		printf("%d ", arr[i]);
	}
	printf("\n");
}

// plan
// - use 16 bit shifts, in 2 passes
// - compute all histograms in one pass
// - call repeatedly for additional keys, vs. building it into the API
//    - allow reuse of allocated buffers
// - try computing negative space, with special final pass
// - try flopping negative bits
// - try reporting if the data is fully sorted
//    can we do this as we go? if not, check on the way in, during histogram.


#define offsetSize (1 << 16)
int *offset0;
int *offset1;

#define flop(x) ((x) ^ (-((x) >> 31) | 0x80000000))
#define mask ((1 << 16) - 1)
#define part0(x) (mask & (x))
#define part1(x) (mask & ((x) >> 16))

void zero(int *a, int n) {
	memset(a, 0, n * sizeof(int));
}

// XXX Consider caching these. They won't change, for a given
// column. If the size is prohibitive, consider more passes:
// cached histo + more passes might be faster than computing histo &
// fewer passes. Maybe look at some profiles to see how it breaks down.
void computeHisto(uint *vals, int n) {
	zero(offset0, offsetSize);
	zero(offset1, offsetSize);
	for (int i = 0; i < n; ++i) {
		uint flopped = flop(vals[i]);
		offset0[part0(flopped)]++;
		offset1[part1(flopped)]++;
	}
	for (int i = 1; i < offsetSize; i++) {
		offset0[i] += offset0[i - 1];
		offset1[i] += offset1[i - 1];
	}
}

//int spy(int x) {
//	printf("idx %d\n", x);
//	return x;
//}

// Sorting indicies is quite a bit slower than direct sorting. Is there
// any way to combine the two? Maybe merge the indicies into a 64 bit value,
// and use 64 bit store/loads?

// We start with a val array & an index array.
// We want to emit or update the index array.
// We make two passes over it, here. If we have multiple
// columns, we will make more passes.
// [v], [i] 
// copy together
// [v, i]
// Do passes over this.
// extract i
// 
// Multi-key
// copy together
// do passes
// copy next key
// do passes
// copy next key
// do passes
// extract i
int spy(int x) {
	printf("i %d\n", x);
	return x;
}
void radixSort(uint *vals, int n, int *indicies) {

	computeHisto(vals, n);

	ulong *output = malloc(n * sizeof(ulong));

	for (int i = n - 1; i >= 0; i--) {
		int ii = indicies[i];
		uint v = vals[ii];
		// contatenate index with value into 64 bits, to take advantage of
		// long word read/write, and avoid a 2nd array lookup, later.
		// XXX we could store the flopped value, to avoid flopping it again.
		// Probably this will not be measurable, since flopping is very fast.
		output[offset0[part0(flop(v))]-- - 1] = ((ulong)ii << 32) | v;
	}

	for (int i = n - 1; i >= 0; i--) {
		indicies[offset1[part1(flop(output[i] & 0xFFFFFFFF))]-- - 1] = output[i] >> 32;
	}
	free(output);
}

#define N 1300000

// some unique values
float *uniqArr() {
	float *arr = malloc(N * sizeof(float));
	for (int i = 0; i < N; ++i) {
		arr[i] = ((float)(rand() - RAND_MAX / 2)) / (RAND_MAX / 10);
	}
	return arr;
}

// initial indicies to be sorted
int *getIndicies() {
	int *arr = malloc(N * sizeof(int));
	for (int i = 0; i < N; ++i) {
		arr[i] = i;
	}
	return arr;
}

float *mapIndicies(float *vals, uint *indicies, int n) {
	float *out = malloc(n * sizeof(float));
	for (int i = 0; i < n; ++i) {
		out[i] = vals[indicies[i]];
	}
	return out;
}

// array with cardinality 3
float *threeValArr() {
	float *arr = malloc(N * sizeof(float));
	for (int i = 0; i < N; i += 1) {
		arr[i] = 2 - i * 3 / N;
	}
	return arr;
}

// array with cardinality 6
float *sixValArr() {
	float *arr = malloc(N * sizeof(float));
	for (int i = 0; i < N; i += 1) {
		arr[i] = i * 6 / N;
	}
	return arr;
}

double testOne() {
	struct timespec start, stop;
	float *a0 = uniqArr();
	float *a1 = sixValArr();
	float *a2 = threeValArr();
	int *indicies = getIndicies();

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	radixSort((uint *)a0, N, indicies);
	radixSort((uint *)a1, N, indicies);
	radixSort((uint *)a2, N, indicies);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
#if 0
	float *col;
	col = mapIndicies(a0, indicies, N);
	print(col, N);
	free(col);
	col = mapIndicies(a1, indicies, N);
	print(col, N);
	free(col);
	col = mapIndicies(a2, indicies, N);
	print(col, N);
	free(col);
#endif
#if 0
	// XXX need a better test here. Maybe compare to quicksort.
	float last = a0[indicies[0]];
	for (int i = 1; i < N; ++i) {
		float next = a0[indicies[i]];
		if (next < last) {
			printf("wrong sort: %g before %g\n", last, next);
			break;
		}
		last = next;
	}
#endif
	free(a0);
	free(a1);
	free(a2);
	free(indicies);
	return (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
}

double min(double *a, int n) {
	double min = 100000;
	for (int i = 0; i < n; ++i) {
		if (a[i] < min) {
			min = a[i];
		}
	}
	return min;
}


int main()
{

	offset0 = malloc(offsetSize * sizeof(int));
	offset1 = malloc(offsetSize * sizeof(int));

#if 1
	int testN = 100;
	double *result = malloc(testN * sizeof(double));
	for (int i = 0; i < testN; ++i) {
		result[i] = testOne();
	}
	double m = min(result, testN);
#else
	double m = testOne();
#endif
	printf("radix time min %f ms\n", m);
}
