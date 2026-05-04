#include <string.h>
#include "bzip2.h"

/* ================================================================
 *  Stage 2 – Move-to-Front transform
 * ================================================================
 *
 *  MTF maintains a list of all 256 possible byte values.  For each
 *  input byte b we:
 *      1. find its current index i in the list
 *      2. emit i
 *      3. move b to the front of the list
 *
 *  Because the BWT clusters identical bytes together, the indices
 *  produced here are heavily skewed toward small numbers (and zero
 *  in particular) – which is exactly what RLE-2 and Huffman exploit.
 *
 *  Output size == input size (one index byte per input byte).
 * ================================================================ */

static void mtf_init_list(unsigned char list[256])
{
    for (int i = 0; i < 256; i++) list[i] = (unsigned char)i;
}

void mtf_encode(unsigned char *input, size_t len, unsigned char *output)
{
    unsigned char list[256];
    mtf_init_list(list);

    for (size_t n = 0; n < len; n++) {
        unsigned char b = input[n];

        /* find b in list */
        int idx = 0;
        while (list[idx] != b) idx++;

        output[n] = (unsigned char)idx;

        /* move b to front (shift list[0..idx-1] down by one) */
        if (idx != 0) {
            memmove(&list[1], &list[0], (size_t)idx);
            list[0] = b;
        }
    }
}

void mtf_decode(unsigned char *input, size_t len, unsigned char *output)
{
    unsigned char list[256];
    mtf_init_list(list);

    for (size_t n = 0; n < len; n++) {
        unsigned char idx = input[n];
        unsigned char b   = list[idx];

        output[n] = b;

        if (idx != 0) {
            memmove(&list[1], &list[0], (size_t)idx);
            list[0] = b;
        }
    }
}

/* ================================================================
 *  Stage 2 – RLE-2  (zero-run escape encoding)
 * ================================================================
 *
 *  Targets the MTF output, which is dominated by zero runs.
 *
 *  Encoding rules:
 *    - any non-zero byte is emitted as-is
 *    - a run of N zeros (1 <= N <= 255) is emitted as:  0x00 N
 *    - runs longer than 255 are split into multiple chunks
 *
 *  Examples:
 *      [7,0,0,0,0,5]      -> [7, 0, 4, 5]
 *      [0]                -> [0, 1]
 *      [0]*300            -> [0, 255, 0, 45]
 *
 *  Worst case  : input of all isolated zeros doubles in size
 *                (rare in practice on MTF output).
 *  Best case   : long zero-runs compress to two bytes.
 *
 *  The caller must allocate the output buffer to at least
 *  (2 * input_len + 16) bytes to be safe.
 * ================================================================ */

void rle2_encode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len)
{
    size_t i = 0, out = 0;

    while (i < len) {
        if (input[i] != 0) {
            output[out++] = input[i++];
            continue;
        }

        /* count the zero run, capped at 255 per chunk */
        size_t run = 0;
        while (i < len && input[i] == 0 && run < 255) {
            run++;
            i++;
        }
        output[out++] = 0;
        output[out++] = (unsigned char)run;
    }

    *out_len = out;
}

void rle2_decode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len)
{
    size_t i = 0, out = 0;

    while (i < len) {
        unsigned char b = input[i++];

        if (b != 0) {
            output[out++] = b;
            continue;
        }

        /* zero marker -> next byte is the run length (1..255) */
        if (i >= len) break;          /* malformed input – ignore */
        unsigned char run = input[i++];
        for (unsigned char k = 0; k < run; k++)
            output[out++] = 0;
    }

    *out_len = out;
}
