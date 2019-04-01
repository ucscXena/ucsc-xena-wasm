#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// A utility function to print an array 
void print(int arr[], int n) 
{ 
	int i;
	for (i = 0; i < n; i++)  {
		printf("%d ", arr[i]);
	}
	printf("\n");
}

void printx(int arr[], int n) 
{ 
	int i;
	for (i = 0; i < n; i++)  {
		printf("%x ", arr[i]);
	}
	printf("\n");
}

// A utility function to get maximum value in arr[] 
int getMax(int arr[], int n) 
{ 
	int mx = arr[0], i;
	for (i = 1; i < n; i++) 
		if (arr[i] > mx) 
			mx = arr[i]; 
	return mx; 
} 

// A function to do counting sort of arr[] according to 
// the digit represented by exp. 
void countSort(int arr[], int n, int exp, int *output)
{ 
	int i, count[10] = {0}; 

	// Store count of occurrences in count[] 
	for (i = 0; i < n; i++) 
		count[ (arr[i]/exp)%10 ]++; 

	// Change count[i] so that count[i] now contains actual 
	// position of this digit in output[] 
	for (i = 1; i < 10; i++) 
		count[i] += count[i - 1]; 

	// Build the output array 
	for (i = n - 1; i >= 0; i--) 
	{ 
		output[count[ (arr[i]/exp)%10 ] - 1] = arr[i]; 
		count[ (arr[i]/exp)%10 ]--; 
	} 
} 

// The main function to that sorts arr[] of size n using 
// Radix Sort 
void radixsort(int arr[], int n) { 
	// Find the maximum number to know number of digits 
	int m = getMax(arr, n), exp,
		*current = arr,
		*output = malloc(n * sizeof(int)),
		*t;

	// Do counting sort for every digit. Note that instead 
	// of passing digit number, exp is passed. exp is 10^i 
	// where i is current digit number 
	for (exp = 1; m/exp > 0; exp *= 10) {
		countSort(current, n, exp, output);
		t = output;
		output = current;
		current = t;
	}
	if (current != arr) {
		int i;
		for (i = 0; i < n; i++) 
			arr[i] = current[i]; 
	}
}

void countSort2(int arr[], int n, int byte, int *output) { 
	int i, count[256] = {0};

	// Store count of occurrences in count[] 
	for (i = 0; i < n; i++) 
		count[(arr[i] >> (8 * byte)) & 0xFF]++;

	// Change count[i] so that count[i] now contains actual 
	// position of this digit in output[] 
	for (i = 1; i < 256; i++) 
		count[i] += count[i - 1]; 

	// Build the output array 
	for (i = n - 1; i >= 0; i--) { 
		int j = (arr[i] >> (8 * byte)) & 0xFF;
		output[count[j] - 1] = arr[i]; 
		count[j]--; 
	}
}

// combint with bin counting?
// check hackers delight for fast ways to do this
//   also check for a radix sort
int getMaxByte(int arr[], int n) {
	int i, all = 0;
	for (i = 0; i < n; ++i) {
		all |= arr[i];
		if (0xFF000000 & all) {
			break;
		}
	}
	return 0xFF000000 & all ? 4 :
		0xFF0000 & all ? 3 :
		0xFF00 & all ? 2:
		1;
}

// XXX probably wrong for negative numbers
// The main function to that sorts arr[] of size n using 
// Radix Sort 
void radixsort2(int arr[], int n) { 
	// Find the maximum number to know number of digits 
	int m = getMaxByte(arr, n), byte;
	int *current = arr,
		*output = malloc(n * sizeof(int)),
		*t;

	// Do counting sort for every digit. Note that instead 
	// of passing digit number, exp is passed. exp is 10^i 
	// where i is current digit number 
	for (byte = 0; byte < m; byte += 1) {
		countSort2(current, n, byte, output); 
		// to avoid multiple alloc, this reuses two
		// buffers as we sort each digit.
		t = output;
		output = current;
		current = t;
	}
	// if we end on the "wrong" buffer, copy it.
	if (current != arr) {
		int i;
		for (i = 0; i < n; i++) 
			arr[i] = current[i]; 
	}
}

// XXX use memset?
void zero(int *a, int n) {
	memset(a, 0, n * sizeof(int));
}

int *count;
// XXX drop 'byte'
void countSortIndicies1Ref(int *vals, int *input, int n, int *output) {
	zero(count, 256);

	// Store count of occurrences in count[]
	for (int i = 0; i < n; i++) {
		count[vals[i]]++;
	}

	// Change count[i] so that count[i] now contains actual
	// position of this digit in output[]
	for (int i = 1; i < 256; i++) {
		count[i] += count[i - 1];
	}

	// Build the output array
	for (int i = n - 1; i >= 0; i--) {
		int j = vals[i];
		output[count[j] - 1] = input[i];
		count[j]--;
	}
}

void radixSortMultiple(int **valList, int valListLength, int n, int *indicies) {
	int *current = indicies;
	int *output = malloc(n * sizeof(int));
	int *oVals = malloc(n * sizeof(int));

	for (int vi = 0; vi < valListLength; ++vi) {
		int *vals = valList[vi];
		// XXX If we're using the pre-sorted column order, we don't need to
		// scan to get the max.
		int m = getMaxByte(vals, n);
		int *t;
		for (int byte = 0; byte < m; byte += 1) {
			int shift = byte << 3;
			// XXX Doesn't this just slow things down? We don't
			// need the re-ordered values to compute histogram, so
			// the indirection is only when building the output array.
			// So we could move the indirection to that step & drop
			// this one.
			// Would need to pass in 'shift', again.
			for (int j = 0; j < n; ++j) {
				oVals[j] = (vals[current[j]] >> shift) & 0xFF;
			}
			countSortIndicies1Ref(oVals, current, n, output);
			// to avoid multiple alloc, this reuses two
			// buffers as we sort each digit.
			t = output;
			output = current;
			current = t;
		}
	}
	// if we end on the "wrong" buffer, copy it.
	// XXX use memcpy
	if (current != indicies) {
		for (int i = 0; i < n; i++) {
			indicies[i] = current[i];
		}
	}
}

int *count32;
void countSortIndicies32(int *vals, int *input, int n, int *output) {
	zero(count32, n);

	// Store count of occurrences in count[]
	for (int i = 0; i < n; i++) {
		count32[vals[i]]++;
	}

	// Change count[i] so that count[i] now contains actual
	// position of this digit in output[]
	for (int i = 1; i < n; i++) {
		count32[i] += count32[i - 1];
	}

	// Build the output array
	for (int i = n - 1; i >= 0; i--) {
		int j = vals[i];
		output[count32[j] - 1] = input[i];
		count32[j]--;
	}
}

void radixSortMultiple32(int **valList, int valListLength, int n, int *indicies) {
	int *current = indicies;
	int *output = malloc(n * sizeof(int));
	int *oVals = malloc(n * sizeof(int));

	for (int vi = 0; vi < valListLength; ++vi) {
		int *vals = valList[vi];
		// XXX If we're using the pre-sorted column order, we don't need to
		// scan to get the max.
		int m = getMaxByte(vals, n);
		int *t;
		for (int j = 0; j < n; ++j) {
			oVals[j] = vals[current[j]];
		}
		countSortIndicies32(oVals, current, n, output);
		// to avoid multiple alloc, this reuses two
		// buffers as we sort each digit.
		t = output;
		output = current;
		current = t;
	}
	// if we end on the "wrong" buffer, copy it.
	// XXX use memcpy
	if (current != indicies) {
		for (int i = 0; i < n; i++) {
			indicies[i] = current[i];
		}
	}
}

static int N = 1300000;
//static int N = 10;

int icmp(const void *ip, const void *jp, void *x) {
	int i = *(const int *)ip;
	int j = *(const int *)jp;
	return 
		i == j ? 0 :
		i > j ? 1:
		-1;
}

#define maxr RAND_MAX;
int rscale = RAND_MAX / maxr;

int *getarr() {
	int *arr = malloc(N * sizeof(int));
	int i;
	for (i = 0; i < N; ++i) {
		arr[i] = rand() / rscale;
	}
	return arr;
}

int *cpyarr(int *in) {
	int *out = malloc(N * sizeof(int));
	memcpy(out, in, N * sizeof(int));
	return out;
}

// N-1..0
int *revarr() {
	int *arr = malloc(N * sizeof(int));
	int i;
	for (i = 0; i < N; ++i) {
		arr[i] = N - i - 1;
	}
	return arr;
}

// 
int *intlarr() {
	int *arr = malloc(N * sizeof(int));
	int i;
	for (i = 0; i < N; i += 1) {
		arr[i] = 2 - i * 3 / N;
	}
	return arr;
}

int *intlarr2() {
	int *arr = malloc(N * sizeof(int));
	int i;
	for (i = 0; i < N; i += 1) {
		arr[i] = i * 6 / N;
	}
	return arr;
}

int *getIndicies() {
	int *arr = malloc(N * sizeof(int));
	int i;
	for (i = 0; i < N; ++i) {
		arr[i] = i;
	}
	return arr;
}

int *mapIndicies(int *vals, int *indicies, int n) {
	int *out = malloc(n * sizeof(int));
	for (int i = 0; i < n; ++i) {
		out[i] = vals[indicies[i]];
	}
	return out;
}

// Driver program to test above functions 
int main() 
{ 
	int *a1 = getarr();
	int *a2 = cpyarr(a1);
	int *a3 = cpyarr(a1);

	count = malloc(256 * sizeof(int));

	struct timespec start, stop;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	radixsort(a1, N); 
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
	double result = (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
	printf("radix time %f ms\n", result);

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	radixsort2(a2, N);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
	result = (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
	printf("radix2 time %f ms\n", result);

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	qsort_r(a3, N, sizeof(int), icmp, NULL);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
	result = (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
	printf("qsort time %f ms\n", result);

	{
		int *a4 = revarr();
		int *a5 = intlarr2();
		int *a6 = intlarr();
		int *i = getIndicies();
		int **list = malloc(3 * sizeof(int*));
		list[0] = a4;
		list[1] = a5;
		list[2] = a6;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
		radixSortMultiple(list, 3, N, i);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
		result = (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
		printf("multi time %f ms\n", result);
		print(i, 10);
		print(i + N - 10, 10);
	}

	{
		count32 = malloc(N * sizeof(int));
		int *a4 = revarr();
		int *a5 = intlarr2();
		int *a6 = intlarr();
		int *i = getIndicies();
		int **list = malloc(3 * sizeof(int*));
		list[0] = a4;
		list[1] = a5;
		list[2] = a6;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
		radixSortMultiple32(list, 3, N, i);
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
		result = (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
		printf("multi32 time %f ms\n", result);
		print(i, 10);
		print(i + N - 10, 10);
#if 0
		int *col;
		print(i, N);
		col = mapIndicies(list[0], i, N);
		print(col, N);
		free(col);
		col = mapIndicies(list[1], i, N);
		print(col, N);
		free(col);
		col = mapIndicies(list[2], i, N);
		print(col, N);
		free(col);
#endif
	}

#if 0
	print(a1, N);
	print(a2, N);
	print(a3, N);
#else
	print(a1, 1);
	print(a1 + N - 1, 1);
	print(a2, 1);
	print(a2 + N - 1, 1);
	print(a3, 1);
	print(a3 + N - 1, 1);
#endif
	return 0; 
} 
