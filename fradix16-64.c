#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h> // malloc
#include <string.h> // memset
#include "fradix16-64.h"

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

#define NAN_FLOP

#ifdef NAN_FLOP
// nan, +inf, -inf
// s111 1111 1xxx xxxx xxxx xxxx xxxx xxxx
// +inf s is 0, high x is 0
// -inf s is 1, high x is 0
// nan: s is 0, high x is 1
// All other non-zero x are also nan, but 7FC is the canonical one we
// will see in the data.

// In order to sort negatives correctly, we have to 1) swap the sign bit
// 2) swap all the other bits if the number is negative. Additionally, since
// nan is bitwise above +inf, and we want to sort it as -inf, explictly map
// it to the flop of -inf.
#define flop(x) ((x) == 0x7FC00000 ? 0x7FFFFF : ((x) ^ (-((x) >> 31) | 0x80000000)))
#else /* NAN_FLOP */
#define flop(x) ((x) ^ (-((x) >> 31) | 0x80000000))
#endif

#if 0
uint32_t flop(uint32_t x) {
	return x == 0x7FC00000 ? 0x7FFFFF : (x ^ (-(x >> 31) | 0x80000000));
}
#endif

#define mask ((1 << 16) - 1)
#define part0(x) (mask & (x))
#define part1(x) (mask & ((x) >> 16))

static void zero(int *a, int n) {
	memset(a, 0, n * sizeof(int));
}

// XXX Consider caching these. They won't change, for a given
// column. If the size is prohibitive, consider more passes:
// cached histo + more passes might be faster than computing histo &
// fewer passes. Maybe look at some profiles to see how it breaks down.
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

#ifndef NAN_FLOP
// method to move nans to the bottom
void nans(uint32_t *vals, int *indicies, int n) {
	int *m = indicies + n;
	while (vals[*--m] == 0x7FC00000) { }
	++m;


	if (m != indicies + n) {
		// buffer shift to m
		int *j = indicies;
		int *k = m;
		int *last = indicies + n;
		// can this be written as a set of memcpy, or memmove?
		while (j != k) {
			uint32_t v0 = *j;
			*j++ = *k;
			*k++ = v0;
			if (k == last) {
				k = m;
			} else if (j == m) {
				m = k;
			}
		}
	}
}
#endif

void fradixSort16_64(uint32_t *vals, int n, int *indicies, uint64_t *scratch) {

	computeHisto(vals, n);

	for (int i = n - 1; i >= 0; i--) {
		int ii = indicies[i];
		uint32_t v = vals[ii];
		// contatenate index with value into 64 bits, to take advantage of
		// long word read/write, and avoid a 2nd array lookup, later.
		// XXX we could store the flopped value, to avoid flopping it again.
		// Probably this will not be measurable, since flopping is very fast.
		// Weirdly, it seems to be slower if we store the flop. Maybe due to
		// additional register load, or something?
		scratch[offset0[part0(flop(v))]-- - 1] = ((uint64_t)ii << 32) | v;
	}

	for (int i = n - 1; i >= 0; i--) {
		indicies[offset1[part1(flop(scratch[i] & 0xFFFFFFFF))]-- - 1] = scratch[i] >> 32;
	}
#ifndef NAN_FLOP
	nans(vals, indicies, n);
#endif
}

void fradixSortL16_64(uint32_t **valsList, int m, int n, int *indicies, uint64_t *scratch) {
	while (m--) {
		fradixSort16_64(*valsList--, n, indicies, scratch);
	}
}

// set up working space.
void fradixSort16_64_init() {
	offset0 = malloc(offsetSize * sizeof(int));
	offset1 = malloc(offsetSize * sizeof(int));
}
