#include <stdlib.h> // malloc
#include <string.h> // memset
#include "radix.h"

static void countSort(int arr[], int n, int byte, int *output) {
	int i, count[256] = {0};

	for (i = 0; i < n; i++)
		count[(arr[i] >> (8 * byte)) & 0xFF]++;

	for (i = 1; i < 256; i++)
		count[i] += count[i - 1];

	for (i = n - 1; i >= 0; i--) {
		int j = (arr[i] >> (8 * byte)) & 0xFF;
		output[count[j] - 1] = arr[i];
		count[j]--;
	}
}

static int getMaxByte(int arr[], int n) {
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

// Sort array of integers
// XXX probably wrong for negative numbers
void radixSort(int arr[], int n) {
	// Find the maximum number to know number of digits
	int m = getMaxByte(arr, n), byte;
	int *current = arr,
		*output = malloc(n * sizeof(int)),
		*t;

	// Do counting sort for every digit. Note that instead
	// of passing digit number, exp is passed. exp is 10^i
	// where i is current digit number
	for (byte = 0; byte < m; byte += 1) {
		countSort(current, n, byte, output);
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

static void zero(int *a, int n) {
	memset(a, 0, n * sizeof(int));
}

static int *count;
static void countSortIndicies(int *vals, int *input, int n, int *output) {
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

// Sort indicies by integer values.
// XXX The complexity of valList isn't necessary: it should be
// equivalent to making multiple calls.
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
			countSortIndicies(oVals, current, n, output);
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
		output = current;
	}
	free(oVals);
	free(output);
}

void radixSortMultiple_init() {
	count = malloc(256 * sizeof(int));
}

static int *count32;
static void countSortIndicies32(int *vals, int *input, int n, int *output) {
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

// XXX The complexity of valList isn't necessary: it should be
// equivalent to making multiple calls.
// XXX Note that this sorts ORDINALS, not arbitrary integers. In particular,
// the highest number should be less than or equal to n.
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

void radixSortMultiple32_init(int N) {
	count32 = malloc(N * sizeof(int));
}

//static int N = 10;

static int icmp(const void *ip, const void *jp, void *x) {
	int i = *(const int *)ip;
	int j = *(const int *)jp;
	return
		i == j ? 0 :
		i > j ? 1:
		-1;
}

#if 0
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
#endif
