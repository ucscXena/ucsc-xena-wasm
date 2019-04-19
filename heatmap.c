#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
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

typedef uint32_t region_color_fn(uint32_t (*)(ctx, double), ctx, float *, int, int);

// XXX make polymorphic for categorical
// XXX is struct copying expensive, here? Should we be allocating
// at the top & passing pointers? Esp. for summary?
static uint32_t region_color_default(uint32_t (*colorfn)(ctx, double), ctx ctx, float *d, int start, int end) {
	struct scale *s = ctx.scale;
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
				uint32_t rcolor = colorfn(ctx, sy.sum[i] / c);
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

uint32_t region_color_linear_test(void *vctx, float *d, int start, int end) {
	ctx ctx;
	ctx.scale = vctx;
	return region_color_default(get_method(LINEAR), ctx, d, start, end);
}

int randmax(int m) {
	return (int)(((double)rand() * m) / RAND_MAX);
}

// XXX look into eliminating this alloc/free
static uint32_t region_color_ordinal(uint32_t (*colorfn)(ctx, double), ctx ctx, float *d, int start, int end) {
	// The js algorithm is to pick randomly from the
	// non-null values in the range [start, end). It does this
	// by first filtering the data. Not sure there's a better way.
	float *filtered = malloc((end - start) * sizeof(float));
	float *p = filtered;
	for (int i = start; i < end; ++i) {
		float v = d[i];
		if (!isnan(v)) {
			*p++ = v;
		}
	}
	int count = p - filtered;
	uint32_t color = count ? colorfn(ctx, filtered[randmax(count)]) : 0xFF808080;
	free(filtered);
	return color;
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

static void project_samples(
		region_color_fn region_color,
		color_fn colorfn,
		ctx ctx, float *d, int first, int count,
		uint32_t *img, int width, int height,
		int elstart, int elsize) {

	for (int i = 0; i < count; ++i) {
		int ry = i * height / count;
		int rheight = (i + 1) * height / count - ry;

		uint32_t color = region_color(colorfn, ctx, d, first + i, first + i + 1);
		if (color != 0xFF808080) {
			fill_region(img, width, elstart, elsize, ry, rheight, color);
		}
	}
}

// divide & take ceiling
static int div_ceil(int x, int y) {
	return (x + y - 1) / y;
}

static void project_pixels(
		region_color_fn region_color,
		color_fn colorfn,
		ctx ctx, float *d, int first, int count,
		uint32_t *img, int width, int height,
		int elstart, int elsize) {

	for (int ry = 0; ry < height; ++ry) {
		int rstart = div_ceil(ry * count, height);
		int rend = div_ceil((ry + 1) * count, height) - 1;

		uint32_t color = region_color(colorfn, ctx, d, first + rstart, first + rend + 1);
		if (color != 0xFF808080) {
			fill_region(img, width, elstart, elsize, ry, 1, color);
		}
	}
}

void draw_subcolumn(
		enum type type,
		void *vctx, float *d, int first, int count,
		uint32_t *img, int width, int height,
		int elstart, int elsize) {

	ctx ctx;
	// Using a union to represent different pointer types. Maybe we should
	// just use void*. We can't pass it in as a union because the wasm
	// entry point doesn't handle the union correctly. So, passing it as void*
	// and populating the union here. Doesn't really matter which union member
	// we assign, since they're the same size, and location.
	ctx.scale = vctx;

	region_color_fn *region_color = type == ORDINAL ? region_color_ordinal : region_color_default;
	color_fn *colorfn = get_method(type);

	if (height > count) {
		project_samples(region_color, colorfn, ctx, d, first, count, img, width, height, elstart, elsize);
	} else {
		project_pixels(region_color, colorfn, ctx, d, first, count, img, width, height, elstart, elsize);
	}
}

void map_indicies(int count, uint32_t *indicies, float *in, float *out) {
	for (int i = 0; i < count; i++) {
		out[i] = in[indicies[i]];
	}
}
