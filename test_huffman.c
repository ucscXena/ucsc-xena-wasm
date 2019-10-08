#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "baos.h"
#include "huffman.h"

char *ok(int pass) {
	return pass ? "✓" : "✗";
}

int main(int argc, char *argv[]) {
	char *s[] = {"foo", "bar"};
	int *freqs = byte_freqs(2, s);
	printf("byte_freqs\n");
	printf("input [\"foo\", \"bar\"]\n");
	printf("%s 0: %d expected 2\n", ok(freqs[0] == 2), freqs[0]);
	printf("%s a: %d expected 1\n", ok(freqs['a'] == 1), freqs['a']);
	printf("%s b: %d expected 1\n", ok(freqs['b'] == 1), freqs['b']);
	printf("%s f: %d expected 1\n", ok(freqs['f'] == 1), freqs['f']);
	printf("%s o: %d expected 2\n", ok(freqs['o'] == 2), freqs['o']);
	printf("%s r: %d expected 1\n", ok(freqs['r'] == 1), freqs['r']);
	for (int i = 0; i < NBYTE; ++i) {
		switch (i) {
			case 0:
			case 'a':
			case 'b':
			case 'f':
			case 'o':
			case 'r':
				continue;
			default:
				if (freqs[i] != 0) {
					printf("%s %d: %d expected 0\n", ok(freqs[i] == 0), i, freqs[i]);
				}
		}
	}

	struct encode_tree *t;
	t = encode_tree_build(freqs);
	//encode_tree_dump(t);
	encode_tree_free(t);
	free(freqs);


	struct decoder dec;
	struct huffman_encoder *enc = strings_encoder(2, s);
	{
		struct baos *output = baos_new();
		huffman_serialize(output, enc);
		char *huff = baos_to_array(output);
		huffman_decoder_init(&dec, huff, 0);
		free(huff);
	}

	struct baos *output = baos_new();
	char in[] = {'f', 'o', 'o', 0, 'b', 'a', 'r', 0};
	encode_bytes(output, enc, sizeof(in), in);
	int len = baos_count(output);
	uint8_t *out = baos_to_array(output);
	huffman_encoder_free(enc);

	struct baos *result_buff = baos_new();
	huffman_canonical_decode(&dec, out, 0, len, result_buff);
	int result_len = baos_count(result_buff);
	char *result = baos_to_array(result_buff);

	for (int i = 0; i < sizeof(in) / sizeof(in[0]); ++i) {
		printf("%s %02x (%c) expected %02x (%c)\n", ok(result[i] == in[i]), result[i], result[i], in[i], in[i]);
	}
	free(out);
	free(result);
	huffman_decoder_free(&dec);
}

