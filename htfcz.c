#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "htfc.h"

void usage() {
	printf("htfcz [-u] <file>\n");
	exit(1);
}

void compress(char *filename) {
	// probably not going to fix this, since we're dropping htfc for hfc.
	printf("fixme\n");
	exit(1);
}

// XXX error handling
void decompress(char *filename) {
	struct stat filestats;
	stat(filename, &filestats);
	off_t size = filestats.st_size;
	char *buff = malloc(size);
	FILE *fp = fopen(filename, "r");
	size = fread(buff, 1, size, fp);
	fclose(fp);

	struct htfc *htfc = htfc_new(buff, size);
	struct htfc_iter *iter = htfc_iter_init(htfc);
	char *s = htfc_iter_next(iter);
	while (s) {
		printf("%s\n", s);
		s = htfc_iter_next(iter);
	}

	htfc_iter_free(iter);
	htfc_free(htfc);
	free(buff);
}


int main(int argc, char *argv[]) {
	int d = 0;
	char *filename;
	if (argc < 2) {
		usage();
	}
	if (!strcmp(argv[1], "-u")) {
		if (argc < 3) {
			usage();
		}
		decompress(argv[2]);
	} else {
		compress(argv[1]);
	}
}
