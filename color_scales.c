#include <stdint.h>
#include <math.h>
#include "color_scales.h"

uint32_t get_color_linear(struct scale *s, double v) {
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
uint32_t get_color_log2(struct scale *s, double v) {
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

// XXX Haven't found a way to export function pointers during build.
uint32_t (*get_get_color_linear())(struct scale *s, double v) {
	return get_color_linear;
}

uint32_t (*get_get_color_log2())(struct scale *s, double v) {
	return get_color_log2;
}
