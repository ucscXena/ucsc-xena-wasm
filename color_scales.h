#include "color_scales_struct.h"

#define R(x) (((x) >> 24) & 0xFF)
#define G(x) (((x) >> 16) & 0xFF)
#define B(x) (((x) >> 8) & 0xFF)
#define RGB(r, g, b) (((r) << 24) | ((g) << 16) | ((b) << 8) | 255)

uint32_t get_color_linear(struct scale *, double v);
uint32_t get_color_log2(struct scale *, double v);

