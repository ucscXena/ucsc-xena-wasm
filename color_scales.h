struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct scale {
	double domain[4];
	int count;
	struct rgb range[4];
};

struct color_lines {
	double m[3];
	double b[3];
};

struct rgb get_color_linear(struct scale *s, double v);
