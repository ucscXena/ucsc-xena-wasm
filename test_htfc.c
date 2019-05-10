#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "htfc.h"

char *ok(int pass) {
	return pass ? "✓" : "✗";
}

#define BUFSIZE 1000
// one two three four
// four one three two
int main(int argc, char *argv[]) {
	FILE *fp = fopen("testdict", "r");
	uint8_t *buff = malloc(BUFSIZE);
	int len = fread(buff, 1, BUFSIZE, fp);
	struct htfc *htfc = htfc_new(buff, len);
	struct search_result result;

	printf("%s len %d expected 4\n", ok(htfc_count(htfc) == 4), htfc_count(htfc));

	htfc_search(htfc, "wo", &result);

	printf("%s results %ld expect 1\n", ok(result.count == 1), result.count);
	printf("%s match 0 %d expect 3\n", ok(result.matches[0] == 3), result.matches[0]);

	free(result.matches);
	htfc_free(htfc);
	free(buff);
	fclose(fp);
}
