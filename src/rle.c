#include <stdlib.h>
#include "bzip2.h"

/*
 * RLE-1 encoding format: each run -> (count_byte)(value_byte)
 *
 * Example: ABBBCCCCD -> (1,A)(3,B)(4,C)(1,D)
 *   which as text looks like: A3B4C1D  (count first, then char)
 *
 * Runs longer than 255 are split automatically.
 * Worst-case output size = 2 × input size  (no repeated bytes).
 * The caller must pre-allocate output with at least 2*len+2 bytes.
 */
void rle1_encode(unsigned char *input,  size_t len,
                 unsigned char *output, size_t *out_len)
{
    size_t i = 0, out = 0;

    while (i < len) {
        unsigned char curr  = input[i];
        size_t        count = 1;

        while (i + count < len && input[i + count] == curr && count < 255)
            count++;

        output[out++] = (unsigned char)count;
        output[out++] = curr;
        i += count;
    }

    *out_len = out;
}

/*
 * RLE-1 decoding: reads (count, byte) pairs and expands them.
 * The caller must pre-allocate output with at least original_size bytes
 * (stored in the compressed file header).
 */
void rle1_decode(unsigned char *input,  size_t len,
                 unsigned char *output, size_t *out_len)
{
    size_t i = 0, out = 0;

    while (i + 1 < len) {
        unsigned char count = input[i++];
        unsigned char byte  = input[i++];

        for (unsigned char k = 0; k < count; k++)
            output[out++] = byte;
    }

    *out_len = out;
}
