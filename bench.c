#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include "fradix16-64.h"
#include "fradix16.h"
#include "fradix.h"
#include "radix.h"
#include "float.h"

#define N 1300000
//#define N 100

#ifdef __EMSCRIPTEN__ // not sure if this works
#define CLOCK CLOCK_MONOTONIC
#else
#define CLOCK CLOCK_PROCESS_CPUTIME_ID
#endif

// A utility function to print an array
static void print(float arr[], int n) {
	int i;
	for (i = 0; i < n; i++)  {
		printf("%g ", arr[i]);
	}
	printf("\n");
}

static void printi(int arr[], int n) {
	for (int i = 0; i < n; i++)  {
		printf("%d ", arr[i]);
	}
	printf("\n");
}

// initial indicies to be sorted
static int *getIndicies() {
	int *arr = malloc(N * sizeof(int));
	for (int i = 0; i < N; ++i) {
		arr[i] = i;
	}
	return arr;
}

static float *mapIndicies(float *vals, int *indicies, int n) {
	float *out = malloc(n * sizeof(float));
	for (int i = 0; i < n; ++i) {
		out[i] = vals[indicies[i]];
	}
	return out;
}

// some unique values
static float *uniqArr() {
	float *arr = malloc(N * sizeof(float));
	for (int i = 0; i < N; ++i) {
		arr[i] = ((float)(rand() - RAND_MAX / 2)) / (RAND_MAX / 10);
	}
	return arr;
}

// array with cardinality 3
static float *threeValArr() {
	float *arr = malloc(N * sizeof(float));
	for (int i = 0; i < N; i += 1) {
		arr[i] = 2 - i * 3 / N;
	}
	return arr;
}

// array with cardinality 6
static float *sixValArr() {
	float *arr = malloc(N * sizeof(float));
	for (int i = 0; i < N; i += 1) {
		arr[i] = i * 6 / N;
	}
	return arr;
}

static void setnan(float *arr, int n, int rate, int offset) {
	for (int i = offset; i < n; i += rate) {
		arr[i] = NAN;
	}
}

// Ordinals in reverse order
static int *revIArr() {
	int *arr = malloc(N * sizeof(int));
	int i;
	for (i = 0; i < N; ++i) {
		arr[i] = N - i - 1;
	}
	return arr;
}

// int array with cardinality 3
static int *threeValIArr() {
	int *arr = malloc(N * sizeof(int));
	int i;
	for (i = 0; i < N; i += 1) {
		arr[i] = 2 - i * 3 / N;
	}
	return arr;
}

// int array with cardinality 6
static int *sixValIArr() {
	int *arr = malloc(N * sizeof(int));
	int i;
	for (i = 0; i < N; i += 1) {
		arr[i] = i * 6 / N;
	}
	return arr;
}

static void dumpSorted(float *a0, float *a1, float *a2, int *indicies, int n) {
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
}

static void validate(float *a0, float *a1, float *a2, int *indicies, int n) {
	for (int i = 1; i < N; ++i) {
		int j = indicies[i - 1];
		int k = indicies[i];
		if (a0[k] < a0[j] ||  // wrong major sort
		    (a0[k] == a0[j] && a1[k] < a1[j]) || // wrong middle sort
		    (a0[k] == a0[j] && a1[k] == a1[j] && a2[k] < a2[j])) { // wrong minor sort

			printf("Wrong sort:\n");
//			dumpSorted(a0, a1, a2, indicies, n);
//			exit(1);
			break;
		}
	}
}

static int *mapIndiciesI(int *vals, int *indicies, int n) {
	int *out = malloc(n * sizeof(int));
	for (int i = 0; i < n; ++i) {
		out[i] = vals[indicies[i]];
	}
	return out;
}

static void dumpSortedI(int *a0, int *a1, int *a2, int *indicies, int n) {
	int *col;
	col = mapIndiciesI(a0, indicies, N);
	printi(col, N);
	free(col);
	col = mapIndiciesI(a1, indicies, N);
	printi(col, N);
	free(col);
	col = mapIndiciesI(a2, indicies, N);
	printi(col, N);
	free(col);
}

static void validateI(int *a0, int *a1, int *a2, int *indicies, int n) {
	for (int i = 1; i < N; ++i) {
		int j = indicies[i - 1];
		int k = indicies[i];
		if (a0[k] < a0[j] ||  // wrong major sort
		    (a0[k] == a0[j] && a1[k] < a1[j]) || // wrong middle sort
		    (a0[k] == a0[j] && a1[k] == a1[j] && a2[k] < a2[j])) { // wrong minor sort

			printf("Wrong sort:\n");
			dumpSortedI(a0, a1, a2, indicies, n);
//			exit(1);
			break;
		}
	}
}

static void testNaN() {
	float a[] = {FLT_MAX, FLT_MIN, 0, NAN, -FLT_MAX, -FLT_MIN, INFINITY, -INFINITY};
	int indicies[] = {0, 1, 2, 3, 4, 5, 6, 7};
	uint64_t scratch[] = {0, 0, 0, 0, 0, 0, 0, 0};

	fradixSort16_64((uint32_t *)a, 8, indicies, scratch);
	float *mapped = mapIndicies(a, indicies, 8);
	print(mapped, 8);
}

static double test16_64(int val) {
	struct timespec start, stop;
	float *a0 = threeValArr();
	float *a1 = sixValArr();
	float *a2 = uniqArr();
	setnan(a0, N, 10, 0);
	setnan(a1, N, 10, 2);
	setnan(a2, N, 10, 4);
	int *indicies = getIndicies();
	uint64_t *scratch = malloc(N * sizeof(uint64_t));

	// Note: would prefer CLOCK_PROCESS_CPUTIME_ID, but it
	// doesn't work in wasm32.
	clock_gettime(CLOCK, &start);
	fradixSort16_64((uint32_t *)a2, N, indicies, scratch);
	fradixSort16_64((uint32_t *)a1, N, indicies, scratch);
	fradixSort16_64((uint32_t *)a0, N, indicies, scratch);
	clock_gettime(CLOCK, &stop);
	if (val) {
		validate(a0, a1, a2, indicies, N);
	}
	free(scratch);
	free(a0);
	free(a1);
	free(a2);
	free(indicies);
	return (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
}

static double test16(int val) {
	struct timespec start, stop;
	float *a0 = threeValArr();
	float *a1 = sixValArr();
	float *a2 = uniqArr();
	int *indicies = getIndicies();

	clock_gettime(CLOCK, &start);
	fradixSort16((uint32_t *)a2, N, indicies);
	fradixSort16((uint32_t *)a1, N, indicies);
	fradixSort16((uint32_t *)a0, N, indicies);
	clock_gettime(CLOCK, &stop);
	if (val) {
		validate(a0, a1, a2, indicies, N);
	}
	free(a0);
	free(a1);
	free(a2);
	free(indicies);
	return (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
}

static double test(int val) {
	struct timespec start, stop;
	float *a0 = threeValArr();
	float *a1 = sixValArr();
	float *a2 = uniqArr();
	int *indicies = getIndicies();
	int *out;

	// XXX try to eliminate these allocations
	clock_gettime(CLOCK, &start);
	out = fradixSort((uint32_t *)a2, N, indicies);
	free(indicies);
	indicies = out;
	out = fradixSort((uint32_t *)a1, N, indicies);
	free(indicies);
	indicies = out;
	out = fradixSort((uint32_t *)a0, N, indicies);
	free(indicies);
	indicies = out;
	clock_gettime(CLOCK, &stop);
	if (val) {
		validate(a0, a1, a2, indicies, N);
	}
	free(a0);
	free(a1);
	free(a2);
	return (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
}

static double testI(int val) {
	struct timespec start, stop;

	int *a0 = threeValIArr();
	int *a1 = sixValIArr();
	int *a2 = revIArr();
	int *indicies = getIndicies();
	int **list = malloc(3 * sizeof(int*));
	list[0] = a2;
	list[1] = a1;
	list[2] = a0;
	clock_gettime(CLOCK, &start);
	radixSortMultiple(list, 3, N, indicies);
	clock_gettime(CLOCK, &stop);
	if (val) {
		validateI(a0, a1, a2, indicies, N);
	}
	free(a0);
	free(a1);
	free(a2);
	free(list);
	free(indicies);
	return (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
}

static double testI32(int val) {
	struct timespec start, stop;

	int *a0 = threeValIArr();
	int *a1 = sixValIArr();
	int *a2 = revIArr();
	int *indicies = getIndicies();
	int **list = malloc(3 * sizeof(int*));
	list[0] = a2;
	list[1] = a1;
	list[2] = a0;
	clock_gettime(CLOCK, &start);
	radixSortMultiple32(list, 3, N, indicies);
	clock_gettime(CLOCK, &stop);
	if (val) {
		validateI(a0, a1, a2, indicies, N);
	}
	free(a0);
	free(a1);
	free(a2);
	free(indicies);
	free(list);
	return (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
}

static double min(double *a, int n) {
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

	{
		fradixSort16_64_init();
#if 0
		testNaN();
		float *f = malloc(sizeof(float));
		uint32_t *i = (uint32_t*)f;
		*f = NAN;
//		printf("%x\n", abs(*i - 0x7FC00000 + 0x80000000));
		*i ^= ((abs(*i - 0x7FC00000 + 0x80000000) & 0x80000000) >> 9) | 0x80000000;
		printf("%x\n", *i);
		printf("%f\n", *f);
		printf("hello\n");
		exit(0);
#endif
#if 1
		test16_64(1);
		int testN = 100;
		double *result = malloc(testN * sizeof(double));
		for (int i = 0; i < testN; ++i) {
			result[i] = test16_64(0);
		}
		double m = min(result, testN);
#else
		double m = test16_64(0);
#endif
		printf("fradix16-64 time min %f ms\n", m);
	}

	{
		fradixSort16_init();
#if 1
		test16(1);
		int testN = 100;
		double *result = malloc(testN * sizeof(double));
		for (int i = 0; i < testN; ++i) {
			result[i] = test16(0);
		}
		double m = min(result, testN);
#else
		double m = test16(0);
#endif
		printf("fradix16 time min %f ms\n", m);
	}

	{
		fradixSort_init();
// XXX This method is not sorting correctly
#if 1
		test(1);
		int testN = 100;
		double *result = malloc(testN * sizeof(double));
		for (int i = 0; i < testN; ++i) {
			result[i] = test(0);
		}
		double m = min(result, testN);
#else
		double m = test(0);
#endif
		printf("fradix time min %f ms\n", m);
	}

	{
		radixSortMultiple_init();
#if 1
		testI(1);
		int testN = 100;
		double *result = malloc(testN * sizeof(double));
		for (int i = 0; i < testN; ++i) {
			result[i] = testI(0);
		}
		double m = min(result, testN);
#else
		double m = testI(0);
#endif
		printf("radixSortMultiple time min %f ms\n", m);
	}

	{
		radixSortMultiple32_init(N);
#if 1
		testI32(1);
		int testN = 100;
		double *result = malloc(testN * sizeof(double));
		for (int i = 0; i < testN; ++i) {
			result[i] = testI32(0);
		}
		double m = min(result, testN);
#else
		double m = testI32(0);
#endif
		printf("radixSortMultiple32 time min %f ms\n", m);
	}
}
