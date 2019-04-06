#define _GNU_SOURCE
#include <stdint.h> // uint32_t
#include <stdlib.h> // malloc
#include <string.h> // memset
#include "fradix.h"

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

static void zero(int *a, int n) {
	memset(a, 0, n * sizeof(int));
}

static void computeHisto(uint32_t *vals, int n) {
	zero(offset0, offsetSize);
	zero(offset1, offsetSize);
	zero(offset2, offsetSize);
	for (int i = 0; i < n; ++i) {
		uint32_t flopped = flop(vals[i]);
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

int *fradixSort(uint32_t *vals, int n, int *indicies) {
	int *output = malloc(n * sizeof(int));

	computeHisto(vals, n);

	for (int i = n - 1; i >= 0; i--) {
		int ii = indicies[i];
		output[offset0[part0(flop(vals[ii]))]-- - 1] = ii;
	}

	for (int i = n - 1; i >= 0; i--) {
		indicies[offset1[part1(flop(vals[output[i]]))]-- - 1] = output[i];
	}

	for (int i = n - 1; i >= 0; i--) {
		output[offset2[part2(flop(vals[indicies[i]]))]-- - 1] = indicies[i];
	}
	return output;
}

void fradixSort_init() {
	offset0 = malloc(offsetSize * sizeof(int));
	offset1 = malloc(offsetSize * sizeof(int));
	offset2 = malloc(offsetSize * sizeof(int));
}
