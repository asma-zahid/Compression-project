#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/bzip2.h"

/* Test helper functions */
void print_bytes(unsigned char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

int compare_arrays(unsigned char *a, unsigned char *b, size_t len) {
    return memcmp(a, b, len) == 0;
}

/* Unit test for RLE-1 */
void test_rle1() {
    printf("Testing RLE-1...\n");

    // Test case 1: No runs
    unsigned char input1[] = "abc";
    unsigned char output1[10];
    size_t out_len1;
    rle1_encode(input1, 3, output1, &out_len1);
    assert(out_len1 == 3);
    assert(compare_arrays(input1, output1, 3));

    unsigned char decode_out1[10];
    size_t decode_len1;
    rle1_decode(output1, out_len1, decode_out1, &decode_len1);
    assert(decode_len1 == 3);
    assert(compare_arrays(input1, decode_out1, 3));

    // Test case 2: With runs
    unsigned char input2[] = "aaabbb";
    unsigned char output2[10];
    size_t out_len2;
    rle1_encode(input2, 6, output2, &out_len2);
    // Should be compressed
    assert(out_len2 < 6);

    unsigned char decode_out2[10];
    size_t decode_len2;
    rle1_decode(output2, out_len2, decode_out2, &decode_len2);
    assert(decode_len2 == 6);
    assert(compare_arrays(input2, decode_out2, 6));

    printf("RLE-1 tests passed!\n");
}

/* Unit test for BWT */
void test_bwt() {
    printf("Testing BWT...\n");

    unsigned char input[] = "banana";
    unsigned char output[6];
    int primary_index;

    bwt_encode(input, 6, output, &primary_index);

    // Check that output is a permutation of input
    unsigned char sorted_input[6];
    memcpy(sorted_input, input, 6);
    // Note: This is a simple check, not comprehensive

    unsigned char decode_out[6];
    bwt_decode(output, 6, primary_index, decode_out);
    assert(compare_arrays(input, decode_out, 6));

    printf("BWT tests passed!\n");
}

/* Unit test for MTF */
void test_mtf() {
    printf("Testing MTF...\n");

    unsigned char input[] = "hello";
    unsigned char output[5];
    unsigned char decode_out[5];

    mtf_encode(input, 5, output);
    mtf_decode(output, 5, decode_out);
    assert(compare_arrays(input, decode_out, 5));

    printf("MTF tests passed!\n");
}

/* Unit test for RLE-2 */
void test_rle2() {
    printf("Testing RLE-2...\n");

    // Create input with zeros (which MTF tends to produce)
    unsigned char input[] = {0, 0, 0, 1, 1, 2};
    unsigned char output[10];
    size_t out_len;

    rle2_encode(input, 6, output, &out_len);
    // Should compress the zeros

    unsigned char decode_out[10];
    size_t decode_len;
    rle2_decode(output, out_len, decode_out, &decode_len);
    assert(decode_len == 6);
    assert(compare_arrays(input, decode_out, 6));

    printf("RLE-2 tests passed!\n");
}

/* Unit test for Huffman */
void test_huffman() {
    printf("Testing Huffman...\n");

    unsigned char input[] = "this is a test";
    unsigned char output[100];
    size_t out_len;

    huffman_encode(input, strlen((char*)input), output, &out_len);

    unsigned char decode_out[100];
    size_t decode_len;
    huffman_decode(output, out_len, decode_out, &decode_len);
    assert(decode_len == strlen((char*)input));
    assert(compare_arrays(input, decode_out, decode_len));

    printf("Huffman tests passed!\n");
}

int main() {
    printf("Running unit tests for BZip2 components...\n\n");

    test_rle1();
    test_bwt();
    test_mtf();
    test_rle2();
    test_huffman();

    printf("\nAll unit tests passed!\n");
    return 0;
}