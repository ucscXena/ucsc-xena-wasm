#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "hfc.h"
#include "queue.h"
#include "bytes.h"

void usage() {
	printf("hfcz [-u] <file>\n");
	exit(1);
}

int compress(const char *filename) {
	int fd;
	struct stat stat;
	int r;

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		return -1;
	}

	r = fstat(fd, &stat);
	if (r == -1) {
		return -1;
	}

	size_t size = stat.st_size;
	uint8_t *buff = malloc(size + 1);
	if (buff == NULL) {
		return -1;
	}

	r = read(fd, buff, size);
	if (r == -1) {
		return -1;
	}
	buff[size] = '\0'; // handle missing final newline
	int lines = 0;
	for (int i = 0; i < size; ++i) {
		if (buff[i] == '\n') {
			lines++;
		}
	}
	uint8_t **strs = malloc(sizeof(char *) * lines);
	if (strs == NULL) {
		return -1;
	}
	uint8_t *c = buff;
	uint8_t **s = strs;
	for (int i = 0; i < size; ++i) {
		if (buff[i] == '\n') {
			*s = c;
			++s;
			buff[i] = '\0';
			c = buff + i + 1;
		}
	}
	struct bytes *cmp = hfc_compress(lines, strs);
	r = write(1, cmp->bytes, cmp->len);
	if (r == -1) {
		return -1;
	}
	free(strs);
	free(buff);
	bytes_free(cmp);
	return 0;
}

// XXX error handling
int decompress(char *filename) {
	struct stat filestats;
	int r;
	r = stat(filename, &filestats);
	if (r == -1) {
		return -1;
	}
	off_t size = filestats.st_size;
	char *buff = malloc(size);
	if (buff == NULL) {
		return -1;
	}
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		return -1;
	}
	r = fread(buff, 1, size, fp);
	if (r < size) {
		return -1;
	}
	fclose(fp);

	struct hfc *hfc = hfc_new(buff, size);
	struct hfc_iter *iter = hfc_iter_init(hfc);
	char *s = hfc_iter_next(iter);
	while (s) {
		printf("%s\n", s);
		s = hfc_iter_next(iter);
	}

	hfc_iter_free(iter);
	hfc_free(hfc);
	free(buff);
	return 0;
}


int main(int argc, char *argv[]) {
	int d = 0;
	int r;
	char *filename;
	if (argc < 2) {
		usage();
	}

	if (!strcmp(argv[1], "-u")) {
		if (argc < 3) {
			usage();
		}
		r = decompress(argv[2]);
	} else {
		r = compress(argv[1]);
	}

	if (r != 0) {
		perror("Failed with error");
		exit(-1);
	}
	exit(0);
}
