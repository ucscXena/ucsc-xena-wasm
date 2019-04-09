#include <stdint.h>
#include <math.h>
#include "color_scales.h"

struct rgb get_color_linear(struct scale *s, double v) {
	struct rgb color;
	int h = s->count - 1;
	if (v >= s->domain[h]) {
		return s->range[h];
	}
	if (v <= s->domain[0]) {
		return s->range[0];
	}
	int r;
	switch (h) {
		case 3:
			if (v > s->domain[2]) {
				r = 2;
				break;
			}
		case 2:
			if (v > s->domain[1]) {
				r = 1;
				break;
			}
		case 1:
			if (v > s->domain[0]) {
				r = 0;
				break;
			}
	}
	double dx = v - s->domain[r];
	double x = s->domain[r + 1] - s->domain[r];
	color.r = roundf(dx * (s->range[r + 1].r - s->range[r].r) / x + s->range[r].r);
	color.g = roundf(dx * (s->range[r + 1].g - s->range[r].g) / x + s->range[r].g);
	color.b = roundf(dx * (s->range[r + 1].b - s->range[r].b) / x + s->range[r].b);

	return color;
}

uint32_t get_color_linear_as_number(struct scale *s, double v) {
	struct rgb color = get_color_linear(s, v);
	return color.r << 16 | color.g << 8 | color.b;
}

// for now, only support one range
struct rgb get_color_log2(struct scale *s, struct color_lines *l, double v) {
	struct rgb color;
	if (v < s->domain[0]) {
		return s->range[0];
	}
	if (v > s->domain[1]) {
		return s->range[1];
	}
	color.r = round(l->m[0] * log2(v) + l->b[0]);
	color.g = round(l->m[1] * log2(v) + l->b[1]);
	color.b = round(l->m[2] * log2(v) + l->b[2]);

	return color;
}

uint32_t get_color_log_as_number(struct scale *s, struct color_lines *l, double v) {
	struct rgb color = get_color_log2(s, l, v);
	return color.r << 16 | color.g << 8 | color.b;
}
