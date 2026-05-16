#include <stdlib.h>
#include <string.h>
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

/* ================================================================
 *  Extra credit §8.1 — Enhanced RLE-1
 * ================================================================
 *  Threshold RLE (bzip2-style):
 *    After seeing `T` identical consecutive bytes, the next byte is
 *    a 0..255 "additional repetitions" count. Runs shorter than T
 *    pass through as literals. Self-synchronising: the decoder only
 *    looks for a count byte after observing exactly T equal bytes.
 *
 *    Example, T=4:
 *      ABCD            -> ABCD                  (no run trigger)
 *      AAAA            -> AAAA \x00             (run = 4, 0 extra)
 *      AAAAA           -> AAAA \x01
 *      AAAAA...A (260) -> AAAA \xFF AAAA \x00   (split: 4+255, 4+0)
 *
 *  Adaptive bypass:
 *    If the encoded payload would not beat the raw block, store raw
 *    with a 1-byte sentinel and skip the transform.
 *
 *  Output framing:
 *      byte 0 : 0x00 = raw, 0x01 = threshold-encoded
 *      if 0x01:  byte 1 = threshold T, bytes 2.. = encoded payload
 *      if 0x00:  bytes 1.. = original block verbatim
 *
 *  Caller must allocate output to at least (len + len/T + 16) bytes.
 *  For safety: 2*len + 16 is always sufficient.
 * ---------------------------------------------------------------- */

/* Core threshold encoder — no framing, just the body bytes.
 * Returns number of bytes written to `out`. */
static size_t rle1_threshold_body(const unsigned char *input, size_t len,
                                  unsigned char *out, int threshold)
{
    size_t i = 0, o = 0;

    while (i < len) {
        unsigned char b = input[i];

        /* count run length, capped at threshold + 255 per chunk */
        size_t run = 1;
        while (i + run < len &&
               input[i + run] == b &&
               run < (size_t)threshold + 255u)
        {
            run++;
        }

        if (run < (size_t)threshold) {
            /* short run: emit literally */
            for (size_t k = 0; k < run; k++) out[o++] = b;
        } else {
            /* run: emit T copies, then (run - T) as a single byte */
            for (int k = 0; k < threshold; k++) out[o++] = b;
            out[o++] = (unsigned char)(run - (size_t)threshold);
        }
        i += run;
    }
    return o;
}

void rle1_encode_enh(unsigned char *input, size_t len,
                     unsigned char *output, size_t *out_len,
                     int threshold, int adaptive)
{
    if (threshold < 2)   threshold = 2;
    if (threshold > 255) threshold = 255;

    /* Encode into a scratch buffer (worst case ~ len bytes since
     * a run of T literals only ever expands by 1 byte). */
    size_t scratch_cap = len + (len / (size_t)threshold) + 16;
    unsigned char *scratch = (unsigned char *)malloc(scratch_cap);
    size_t encoded_sz = rle1_threshold_body(input, len, scratch, threshold);

    /* Adaptive bypass: framed-encoded size = encoded_sz + 2,
     * framed-raw size = len + 1. Pick the smaller. */
    if (adaptive && encoded_sz + 2 >= len + 1) {
        output[0] = 0x00;                       /* raw mode */
        memcpy(output + 1, input, len);
        *out_len = len + 1;
    } else {
        output[0] = 0x01;                       /* encoded mode */
        output[1] = (unsigned char)threshold;
        memcpy(output + 2, scratch, encoded_sz);
        *out_len = encoded_sz + 2;
    }

    free(scratch);
}

void rle1_decode_enh(unsigned char *input, size_t len,
                     unsigned char *output, size_t *out_len)
{
    if (len == 0) { *out_len = 0; return; }

    unsigned char mode = input[0];
    size_t o = 0;

    if (mode == 0x00) {                         /* raw bypass */
        memcpy(output, input + 1, len - 1);
        *out_len = len - 1;
        return;
    }

    if (mode != 0x01 || len < 2) {              /* malformed */
        *out_len = 0;
        return;
    }

    int threshold = input[1];
    if (threshold < 2) threshold = 2;

    /* Decode body starting at input[2]. Walk forward, watching for
     * a run of T identical bytes — the next byte is then the count. */
    size_t i = 2;
    while (i < len) {
        unsigned char b = input[i];

        /* count how many of `b` follow, but only up to T */
        int run = 1;
        while (run < threshold && i + run < len && input[i + run] == b)
            run++;

        if (run < threshold) {
            /* short run: just emit literally */
            for (int k = 0; k < run; k++) output[o++] = b;
            i += run;
        } else {
            /* full threshold-run: next byte is (extra) repetitions */
            if (i + threshold >= len) {
                /* truncated stream: emit what we have */
                for (int k = 0; k < threshold; k++) output[o++] = b;
                i += threshold;
                break;
            }
            unsigned char extra = input[i + threshold];
            int total = threshold + (int)extra;
            for (int k = 0; k < total; k++) output[o++] = b;
            i += threshold + 1;
        }
    }

    *out_len = o;
}
