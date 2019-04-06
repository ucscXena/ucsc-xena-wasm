#define _GNU_SOURCE
#include <stdint.h> // uint32_t
#include <stdlib.h> // malloc
#include <string.h> // memset
#include "fradix16.h"

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
static int *offset0;
static int *offset1;

// sort NAN to bottom
#define flop(x) ((x) == 0x7FC00000U ? 0x7FFFFFU : ((x) ^ (-((x) >> 31) | 0x80000000U)))
//#define flop(x) ((x) ^ (-((x) >> 31) | 0x80000000))
#define mask ((1 << 16) - 1)
#define part0(x) (mask & (x))
#define part1(x) (mask & ((x) >> 16))

static void zero(int *a, int n) {
	memset(a, 0, n * sizeof(int));
}

static void computeHisto(uint32_t *vals, int n) {
	zero(offset0, offsetSize);
	zero(offset1, offsetSize);
	for (int i = 0; i < n; ++i) {
		uint32_t flopped = flop(vals[i]);
		offset0[part0(flopped)]++;
		offset1[part1(flopped)]++;
	}
	for (int i = 1; i < offsetSize; i++) {
		offset0[i] += offset0[i - 1];
		offset1[i] += offset1[i - 1];
	}
}

// Sorting indicies is quite a bit slower than direct sorting. Is there
// any way to combine the two? Maybe merge the indicies into a 64 bit value,
// and use 64 bit store/loads? See fradix16-64.

void fradixSort16(uint32_t *vals, int n, int *indicies) {
	int *output = malloc(n * sizeof(int));

	computeHisto(vals, n);

	for (int i = n - 1; i >= 0; i--) {
		int ii = indicies[i];
		output[offset0[part0(flop(vals[ii]))]-- - 1] = ii;
	}

	for (int i = n - 1; i >= 0; i--) {
		indicies[offset1[part1(flop(vals[output[i]]))]-- - 1] = output[i];
	}
	free(output);
}

void fradixSort16InPlace(uint32_t *vals, int n) {
	int *scratch = malloc(n * sizeof(int));

	computeHisto(vals, n);

	for (int i = n - 1; i >= 0; i--) {
		uint32_t v = vals[i];
		scratch[offset0[part0(flop(v))]-- - 1] = v;
	}

	for (int i = n - 1; i >= 0; i--) {
		uint32_t v = scratch[i];
		vals[offset1[part1(flop(v))]-- - 1] = v;
	}
	free(scratch);
}

void fradixSort16_init() {
	offset0 = malloc(offsetSize * sizeof(int));
	offset1 = malloc(offsetSize * sizeof(int));
}
