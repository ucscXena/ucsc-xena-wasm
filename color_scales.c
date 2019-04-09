#include <stdint.h>
#include <math.h>
#include "color_scales.h"

struct rgb get_color_linear(struct linear_scale *s, float v) {
	struct rgb color;
	int h = s->count - 1;
	if (v >= s->domain[h]) {
		color = s->range[h];
		return color;
	}
	if (v <= s->domain[0]) {
		color = s->range[0];
		return color;
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
	float dx = v - s->domain[r];
	float x = s->domain[r + 1] - s->domain[r];
	color.r = roundf(dx * (s->range[r + 1].r - s->range[r].r) / x + s->range[r].r);
	color.g = roundf(dx * (s->range[r + 1].g - s->range[r].g) / x + s->range[r].g);
	color.b = roundf(dx * (s->range[r + 1].b - s->range[r].b) / x + s->range[r].b);

	return color;
}

uint32_t get_color_linear_as_number(struct linear_scale *s, float v) {
	struct rgb color = get_color_linear(s, v);
	return color.r << 16 | color.g << 8 | color.b;
}
