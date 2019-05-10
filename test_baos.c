#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // uint32_t
#include <string.h>
#include "baos.h"

char *ok(int pass) {
	return pass ? "✓" : "✗";
}

int main(int argc, char *argv[]) {
	struct baos *baos;
	char *s = "abcdef";
	char *result;
	int len;


	baos = baos_new();
	len = baos_count(baos);
	result = baos_to_array(baos);
	printf("%s length %d expected 0\n", ok(len == 0), len);
	printf("%s array %p expected NULL\n", ok(result == NULL), result);
	free(result);

	baos = baos_new();
	baos_push(baos, s[0]);
	len = baos_count(baos);
	result = baos_to_array(baos);
	printf("%s length %d expected 1\n", ok(len == 1), len);
	printf("%s array %p expected !NULL\n", ok(result != NULL), result);
	printf("%s buff %.*s expected a\n", ok(!strncmp(result, "a", 1)), 1, result);
	free(result);

	baos = baos_new();
	baos_push(baos, s[0]);
	baos_push(baos, s[1]);
	len = baos_count(baos);
	result = baos_to_array(baos);
	printf("%s length %d expected 2\n", ok(len == 2), len);
	printf("%s array %p expected !NULL\n", ok(result != NULL), result);
	printf("%s buff %.*s expected ab\n", ok(!strncmp(result, "ab", 2)), 2, result);
	free(result);

	for (int N = 4095; N < 4099; ++N) {
		uint8_t *expected;

		baos = baos_new();
		for (int i = 0; i < N; ++i) {
			baos_push(baos, s[i % sizeof(s)]);
		}
		len = baos_count(baos);
		result = baos_to_array(baos);
		printf("%s length %d expected %d\n", ok(len == N), len, N);
		printf("%s array %p expected !NULL\n", ok(result != NULL), result);
		expected = malloc(N);
		for (int i = 0; i < N; ++i) {
			expected[i] = s[i % sizeof(s)];
		}
		printf("%s buff == expected\n", ok(!strncmp(result, "ab", 2)));
		free(expected);
		free(result);
	}
}
