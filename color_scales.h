struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct linear_scale {
	int count;
	float domain[4];
	struct rgb range[4];
};

struct rgb get_color_linear(struct linear_scale *s, float v);
