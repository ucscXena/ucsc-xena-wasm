#include <stdint.h>
#include <math.h>
#include "color_scales.h"

uint32_t get_color_linear(ctx ctx, double v) {
	struct scale *s = ctx.scale;
	int h = s->count - 1;
	if (v >= s->domain[h]) {
		return s->range[h];
	}
	if (v <= s->domain[0]) {
		return s->range[0];
	}
	int di;
	switch (h) { // consider splitting into multiple fns to avoid this switch
		case 3:
			if (v > s->domain[2]) {
				di = 2;
				break;
			}
		case 2:
			if (v > s->domain[1]) {
				di = 1;
				break;
			}
		case 1:
			di = 0;
	}
	double dx = v - s->domain[di];
	double x = s->domain[di + 1] - s->domain[di];
	// XXX is it faster to store s->range[r], vs. referencing it multiple times?
	// Does the compiler figure this out?
	int r = round(dx * (int)(R(s->range[di + 1]) - R(s->range[di])) / x + R(s->range[di]));
	int g = round(dx * (int)(G(s->range[di + 1]) - G(s->range[di])) / x + G(s->range[di]));
	int b = round(dx * (int)(B(s->range[di + 1]) - B(s->range[di])) / x + B(s->range[di]));

	return RGB(r, g, b);
}

// for now, only support one range
uint32_t get_color_log2(ctx ctx, double v) {
	struct scale *s = ctx.scale;
	if (v < s->domain[0]) {
		return s->range[0];
	}
	if (v > s->domain[1]) {
		return s->range[1];
	}
	int r = round(s->m[0] * log2(v) + s->b[0]);
	int g = round(s->m[1] * log2(v) + s->b[1]);
	int b = round(s->m[2] * log2(v) + s->b[2]);
	return RGB(r, g, b);
}

// ordinal scale is stored as
// [n, c0, c1, c2, .... cn-1]
uint32_t get_color_ordinal(ctx ctx, double v) {
	int i = v;
	uint32_t *o = ctx.ordinal;
	uint32_t c = o[0];
	return (o + 1)[i % c];
}

uint32_t (*get_method(enum type type))(ctx, double) {
	switch (type) {
		case LINEAR:
			return get_color_linear;
		case LOG:
			return get_color_log2;
		case ORDINAL:
			return get_color_ordinal;
	}
}

uint32_t test_scale_method(enum type type, void *vctx, double v) {
	ctx ctx;
	ctx.scale = vctx;
	return get_method(type)(ctx, v);
}
