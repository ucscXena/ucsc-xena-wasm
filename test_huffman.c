#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <check.h>
#include "baos.h"
#include "huffman.h"
#include "bytes.h"
#include "queue.h"

START_TEST(test_byte_freqs)
{
	struct bytes *bins[2];
	bins[0] = bytes_new(4, (uint8_t *)"foo");
	bins[1] = bytes_new(4, (uint8_t *)"bar");
	int *freqs = byte_freqs(2, bins);
	free(bins[0]);
	free(bins[1]);
	ck_assert_int_eq(freqs[0], 2);
	ck_assert_int_eq(freqs['a'], 1);
	ck_assert_int_eq(freqs['b'], 1);
	ck_assert_int_eq(freqs['f'], 1);
	ck_assert_int_eq(freqs['o'], 2);
	ck_assert_int_eq(freqs['r'], 1);
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
				ck_assert_int_eq(freqs[i], 0);
		}
	}
	free(freqs);
}
END_TEST

START_TEST(test_encode_tree)
{
	struct bytes *bins[2];
	bins[0] = bytes_new(4, (uint8_t *)"foo");
	bins[1] = bytes_new(4, (uint8_t *)"bar");
	struct huffman_encoder *enc = huffman_bytes_encoder(2, bins);
	huffman_encoder_free(enc);
	free(bins[0]);
	free(bins[1]);
}
END_TEST

START_TEST(test_encode_decode)
{
	struct decoder dec;
	struct bytes *bins[2];
	bins[0] = bytes_new(4, (uint8_t *)"foo");
	bins[1] = bytes_new(4, (uint8_t *)"bar");
	struct huffman_encoder *enc = huffman_bytes_encoder(2, bins);
	free(bins[0]);
	free(bins[1]);
	{
		struct baos *output = baos_new();
		huffman_serialize(output, enc);
		uint8_t *huff = baos_to_array(output);
		huffman_decoder_init(&dec, huff, 0);
		free(huff);
	}

	struct baos *output = baos_new();
	uint8_t in[] = {'f', 'o', 'o', 0, 'b', 'a', 'r', 0};
	huffman_encode_bytes(output, enc, sizeof(in), in);
	int len = baos_count(output);
	uint8_t *out = baos_to_array(output);
	huffman_encoder_free(enc);

	struct baos *result_buff = baos_new();
	huffman_canonical_decode(&dec, out, 0, len, result_buff);
	int result_len = baos_count(result_buff);
	uint8_t *result = baos_to_array(result_buff);

	for (int i = 0; i < sizeof(in) / sizeof(in[0]); ++i) {
		ck_assert_msg(result[i] == in[i],
				"Assertion failed: %02x (%c) expected %02x (%c)",
				result[i], result[i], in[i], in[i]);
	}
	free(out);
	free(result);
	huffman_decoder_free(&dec);
}
END_TEST

void add_huffman(TCase *tc) {
	tcase_add_test(tc, test_byte_freqs);
	tcase_add_test(tc, test_encode_tree);
	tcase_add_test(tc, test_encode_decode);
}
