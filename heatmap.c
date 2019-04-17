#include <stdint.h>
#include <math.h>
#include <string.h>
#include "color_scales.h"

// We're holding a color scale, which has some domain,
// with 2-4 stops, k. We compute the sum and count of
// data points, per domain region, i.e. the k + 1 regions
// defined by the k stops.

#include "heatmap_struct.h"

void tally_domains(struct summary *summary, struct scale *s, float *data, int start, int end) {
	int h = s->count;
	float v;
	int d;
	memset(summary, 0, sizeof(struct summary));
	for (int i = start; i < end; ++i) {
		float v = data[i];
		if (isnan(v)) {
			continue;
		}
		if (v >= s->domain[h - 1]) {
			d = h;
		} else if (v <= s->domain[0]) {
			d = 0;
		} else {
			switch (h) { // consider splitting into multiple fns to avoid this switch
				case 4:
					if (v > s->domain[2]) {
						d = 3;
						break;
					}
				case 3:
					if (v > s->domain[1]) {
						d = 2;
						break;
					}
				case 2:
					d = 1;
			}
		}
		summary->count[d]++;
		summary->sum[d] += v;
	}
}

static int sum(uint32_t *counts, int n)  {
	int s = 0;
	for (int i = 0; i < n; ++i) {
		s += counts[i];
	}
	return s;
}

// XXX make polymorphic for categorical
// XXX is struct copying expensive, here? Should we be allocating
// at the top & passing pointers? Esp. for summary?
static uint32_t region_color(uint32_t (*colorfn)(struct scale *, double), struct scale *s, float *d, int start, int end) {
	struct summary sy;
	tally_domains(&sy, s, d, start, end);
	int ranges = s->count + 1;
	int total = sum(sy.count, ranges);
	uint32_t color;

	if (total) {
		double r = 0, g = 0, b = 0;
		for (int i = 0; i < ranges; ++i) {
			int c = sy.count[i];
			if (c) {
				uint32_t rcolor = colorfn(s, sy.sum[i] / c);
				int rc = R(rcolor);
				r += ((double)rc * rc * c) / total;
				int gc = G(rcolor);
				g += ((double)gc * gc * c) / total;
				int bc = B(rcolor);
				b += ((double)bc * bc * c) / total;
			}
		}
		color = RGB((int)sqrt(r), (int)sqrt(g), (int)sqrt(b));
	} else {
		color = RGB(0x80, 0x80, 0x80);
	}
	return color;
}

uint32_t region_color_linear_test(struct scale *s, float *d, int start, int end) {
	return region_color(get_color_linear, s, d, start, end);
}

static void fill_region(uint32_t *img, int width, int elstart, int elsize,
		int ry, int rheight, uint32_t c32) {

	for (int y = ry; y < ry + rheight; ++y) {
		int pxRow = y * width;
		int buffStart = pxRow + elstart;
		int buffEnd = buffStart + elsize;
		// try 64 bits? try simd when it's available.
		for (int l = buffStart; l < buffEnd;) {
			img[l++] = c32;
		}
	}
}


static void project_samples(uint32_t (*colorfn)(struct scale *, double),
		struct scale *s, float *d, int first, int count,
		uint32_t *img, int width, int height,
		int elstart, int elsize) {

	for (int i = 0; i < count; ++i) {
		// XXX skip empty regions
		int ry = i * height / count;
		int rheight = (i + 1) * height / count - ry;

		uint32_t color = region_color(colorfn, s, d, first + i, first + i + 1);
		if (color != 0x808080FF) {
			fill_region(img, width, elstart, elsize, ry, rheight, color);
		}
	}
}

// divide & take ceiling
static int div_ceil(int x, int y) {
	return (x + y - 1) / y;
}

static void project_pixels(uint32_t (*colorfn)(struct scale *, double),
		struct scale *s, float *d, int first, int count,
		uint32_t *img, int width, int height,
		int elstart, int elsize) {

	for (int ry = 0; ry < height; ++ry) {
		// XXX skip empty regions
		int rstart = div_ceil(ry * count, height);
		int rend = div_ceil((ry + 1) * count, height) - 1;

		uint32_t color = region_color(colorfn, s, d, first + rstart, first + rend + 1);
		if (color != 0x808080FF) {
			fill_region(img, width, elstart, elsize, ry, 1, color);
		}
	}
}

void draw_subcolumn(
		uint32_t (*colorfn)(struct scale *, double),
		struct scale *s, float *d, int first, int count,
		uint32_t *img, int width, int height,
		int elstart, int elsize) {

	if (height > count) {
		project_samples(colorfn, s, d, first, count, img, width, height, elstart, elsize);
	} else {
		project_pixels(colorfn, s, d, first, count, img, width, height, elstart, elsize);
	}
}
