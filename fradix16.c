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

int spy(int x) {
	printf("idx %d\n", x);
	return x;
}

// Sorting indicies is quite a bit slower than direct sorting. Is there
// any way to combine the two? Maybe merge the indicies into a 64 bit value,
// and use 64 bit store/loads?
void radixSort(uint *vals, int n, int *indicies) {
	int *output = malloc(n * sizeof(int));

	computeHisto(vals, n);

	for (int i = n - 1; i >= 0; i--) {
		output[offset0[part0(flop(vals[i]))]-- - 1] = i;
	}

	for (int i = n - 1; i >= 0; i--) {
		indicies[offset1[part1(flop(vals[output[i]]))]-- - 1] = output[i];
	}
	free(output);
}

#define N 1300000

float *getarr() {
	float *arr = malloc(N * sizeof(float));
	for (int i = 0; i < N; ++i) {
		arr[i] = ((float)(rand() - RAND_MAX / 2)) / (RAND_MAX / 10);
	}
	return arr;
}

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

int *intlarr() {
	int *arr = malloc(N * sizeof(int));
	int i;
	for (i = 0; i < N; i += 1) {
		arr[i] = 2 - i * 3 / N;
	}
	return arr;
}

double testOne() {
	struct timespec start, stop;
	float *floats = getarr();
	int *i = getIndicies();

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	radixSort((uint *)floats, N, i);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
#if 0
	float *col = mapIndicies(floats, i, N);
//	print(floats, 10);
	print(col, 10);
	print(col + N - 10, 10);
#endif
	free(floats);
	free(i);
	return (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
}

double mean(double *a, int n) {
	double sum = 0;
	for (int i = 0; i < n; ++i) {
		sum += a[i];
	}
	return sum / n;
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
	double m = mean(result, testN);
#else
	double m = testOne();
#endif
	printf("radix time mean %f ms\n", m);
}
