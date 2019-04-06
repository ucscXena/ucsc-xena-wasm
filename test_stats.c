#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // uint32_t
#include <math.h>
#include "stats.h"
#include "fradix16.h"

char *ok(int pass) {
	return pass ? "✓" : "✗";
}

int main(int argc, char *argv[]) {
	fameanmedian_init();
	fradixSort16_init();
	float vals[] = {-1., NAN, -2., NAN, -3., -1., -4., -1.};

	float *r = fameanmedian(vals, 8);
	printf("%s mean %f expected -2\n", ok(r[0] == -2), r[0]);
	printf("%s median %f expected -1.5\n", ok(r[1] == -1.5), r[1]);

	vals[0] = NAN;
	vals[1] = NAN;
	r = fameanmedian(vals, 2);
	printf("%s mean %f expected nan\n", ok(r[0] != r[0]), r[0]);
	printf("%s median %f expected nan\n", ok(r[1] != r[1]), r[1]);

	r = fameanmedian(vals, 0);
	printf("%s mean %f expected nan\n", ok(r[0] != r[0]), r[0]);
	printf("%s median %f expected nan\n", ok(r[1] != r[1]), r[1]);

	faminmax_init();
	float vals2[] = {-2., -3., -4., -1.};
	r = faminmax(vals2, sizeof(vals2) / sizeof(float));
	printf("%s min %f expected -4\n", ok(r[0] == -4.), r[0]);
	printf("%s max %f expected -1\n", ok(r[1] == -1.), r[1]);
}
