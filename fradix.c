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
		printf("%f ", arr[i]);
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
// - use 11 bit shifts, in 3 passes
// - compute all histograms in one pass
// - call repeatedly for additional keys, vs. building it into the API
//    - allow reuse of allocated buffers 
// - try computing negative space, with special final pass
// - try flopping negative bits
// - try reporting if the data is fully sorted
//    can we do this as we go? if not, check on the way in, during histogram.


#define offsetSize (1 << 11)
int *offset0;
int *offset1;
int *offset2;

#define flop(x) ((x) ^ (-((x) >> 31) | 0x80000000))
#define mask ((1 << 11) - 1)
#define part0(x) (mask & (x))
#define part1(x) (mask & ((x) >> 11))
#define part2(x) (mask & ((x) >> 22))

void zero(int *a, int n) {
	memset(a, 0, n * sizeof(int));
}

void computeHisto(uint *vals, int n) {
	zero(offset0, offsetSize);
	zero(offset1, offsetSize);
	zero(offset2, offsetSize);
	for (int i = 0; i < n; ++i) {
		uint flopped = flop(vals[i]);
		offset0[part0(flopped)]++;
		offset1[part1(flopped)]++;
		offset2[part2(flopped)]++;
	}
	for (int i = 1; i < offsetSize; i++) {
		offset0[i] += offset0[i - 1];
		offset1[i] += offset1[i - 1];
		offset2[i] += offset2[i - 1];
	}
}

int spy(int x) {
	printf("idx %d\n", x);
	return x;
}

// This is unexpectedly slow. Is it due to flop? No. Very weird.
// Is it due to high cardinality? Maybe test the same bits as integers.
//    -> we can't use the 32 bit algorithm for this, because it goes over
//          the sample count.
// Due to single-pass histo compute?
// Due to not being 32 bit? But we do three columns in 32 bit, which is
//   also three passes, same as here, and it's 3x faster. wtf.
// Due to different histo sizes? Here we use 3 2048 histos. In the
//    fast code we use 1 1M histo.
// 
// Sorting indicies is quite a bit slower than direct sorting. Is there
// any way to combine the two? Maybe merge the indicies into a 64 bit value,
// and use 64 bit store/loads?
int *radixSort(uint *vals, int n, int *indicies) {
	int *output = malloc(n * sizeof(int));

	computeHisto(vals, n);

	for (int i = n - 1; i >= 0; i--) {
		output[offset0[part0(flop(vals[i]))]-- - 1] = i;
	}

	for (int i = n - 1; i >= 0; i--) {
		indicies[offset1[part1(flop(vals[output[i]]))]-- - 1] = output[i];
	}

	for (int i = n - 1; i >= 0; i--) {
		output[offset2[part2(flop(vals[indicies[i]]))]-- - 1] = indicies[i];
	}
	return output;
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

int main() 
{ 
	struct timespec start, stop;

	offset0 = malloc(offsetSize * sizeof(int));
	offset1 = malloc(offsetSize * sizeof(int));
	offset2 = malloc(offsetSize * sizeof(int));
	
	float *floats = getarr();
	int *i = getIndicies();

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	uint *out = radixSort((uint *)floats, N, i);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
	double result = (stop.tv_sec - start.tv_sec) * 1e3 + (stop.tv_nsec - start.tv_nsec) / 1e6;    // in milliseconds
	printf("radix time %f ms\n", result);

	float *col = mapIndicies(floats, out, N);
//	print(floats, 10);
	print(col, 10);
	print(col + N - 10, 10);

//	uint x = 0xFFFFFFFF;
//	printf("%x\n", flop(x));
//	int y = -1;
//	printf("%x\n", flop(y));
}
