#include "color_scales_struct.h"

#define R(x) (x & 0xFF)
#define G(x) (((x) >> 8) & 0xFF)
#define B(x) (((x) >> 16) & 0xFF)

#define RGB(r, g, b) ((255 << 24) | ((b) << 16) | ((g) << 8) | r)

union ctx {
	struct scale *scale;
	uint32_t *ordinal;
};
typedef union ctx ctx;

typedef uint32_t color_fn(ctx, double);

color_fn *get_method(enum type);
