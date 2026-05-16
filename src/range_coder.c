#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "bzip2.h"

/* ================================================================
 *  Extra credit §8.3 — Static order-0 Range Coder
 * ================================================================
 *
 *  Drop-in replacement for the Huffman entropy coder.  Same calling
 *  convention as huffman_encode / huffman_decode:
 *      - encoder writes a fixed-size header followed by the payload
 *      - decoder is told the expected output size via *out_len
 *
 *  Algorithm:  carry-propagating range coder (Schindler-style),
 *  32-bit `range`, 33-bit virtual `low` (held in a uint64).  Output
 *  one byte at a time, deferring writes when the top byte is FF so
 *  that a future carry can ripple through.
 *
 *  Model:  static order-0 over 256 symbols.  The frequency table
 *  (256 * uint32_t LE) is written verbatim to the payload header,
 *  exactly like the canonical Huffman 256-byte length header — the
 *  decoder reconstructs the same cumulative table from it.
 *
 *  Payload layout:
 *      0    1024   256 frequencies (uint32_t LE)
 *      1024 N      range-coded data
 * ================================================================ */

#define RC_BOTTOM      ((uint32_t)0x01000000u)   /* renormalize threshold (2^24) */
#define RC_HEADER_SIZE 1024                      /* 256 * sizeof(uint32_t)        */

/* ----------------------------------------------------------------
 *  Encoder
 * ---------------------------------------------------------------- */

typedef struct {
    uint64_t       low;
    uint32_t       range;
    uint32_t       cache;       /* most-recent buffered high byte    */
    uint64_t       cache_size;  /* count of deferred 0xFF bytes + 1  */
    unsigned char *out;
    size_t         out_pos;
} RangeEncoder;

static void re_init(RangeEncoder *e, unsigned char *out)
{
    e->low        = 0;
    e->range      = 0xFFFFFFFFu;
    e->cache      = 0;
    e->cache_size = 1;
    e->out        = out;
    e->out_pos    = 0;
}

/* Shift the top byte out of `low` into the output stream.
 * If a carry has occurred (low >= 2^32) or top byte is < 0xFF, we
 * can safely emit the deferred bytes.  Otherwise defer one more. */
static void re_shift_low(RangeEncoder *e)
{
    if ((uint32_t)e->low < 0xFF000000u || (uint32_t)(e->low >> 32) != 0) {
        uint8_t carry = (uint8_t)(e->low >> 32);
        uint8_t temp  = (uint8_t)e->cache;
        do {
            e->out[e->out_pos++] = (uint8_t)(temp + carry);
            temp = 0xFF;
        } while (--e->cache_size != 0);
        e->cache = (uint32_t)(((uint32_t)e->low) >> 24);
    }
    e->cache_size++;
    e->low = ((uint32_t)e->low) << 8;   /* mask high carry bit, then shift */
}

static void re_encode_sym(RangeEncoder *e,
                          uint32_t cum, uint32_t freq, uint32_t total)
{
    e->range /= total;
    e->low   += (uint64_t)cum * e->range;
    e->range *= freq;
    while (e->range < RC_BOTTOM) {
        e->range <<= 8;
        re_shift_low(e);
    }
}

static void re_finish(RangeEncoder *e)
{
    for (int i = 0; i < 5; i++) re_shift_low(e);
}

/* ----------------------------------------------------------------
 *  Decoder
 * ---------------------------------------------------------------- */

typedef struct {
    uint32_t             range;
    uint32_t             code;
    const unsigned char *in;
    size_t               in_pos;
    size_t               in_len;
} RangeDecoder;

static uint8_t rd_get_byte(RangeDecoder *d)
{
    return (d->in_pos < d->in_len) ? d->in[d->in_pos++] : 0;
}

static void rd_init(RangeDecoder *d, const unsigned char *in, size_t in_len)
{
    d->in     = in;
    d->in_len = in_len;
    d->in_pos = 0;
    d->range  = 0xFFFFFFFFu;
    d->code   = 0;
    rd_get_byte(d);                          /* discard encoder's leading 0 */
    for (int i = 0; i < 4; i++)
        d->code = (d->code << 8) | rd_get_byte(d);
}

static uint32_t rd_get_cum(RangeDecoder *d, uint32_t total)
{
    d->range /= total;
    uint32_t cf = d->code / d->range;
    return cf < total ? cf : total - 1;      /* clamp for safety */
}

static void rd_advance(RangeDecoder *d, uint32_t cum, uint32_t freq)
{
    d->code  -= cum * d->range;
    d->range *= freq;
    while (d->range < RC_BOTTOM) {
        d->range <<= 8;
        d->code = (d->code << 8) | rd_get_byte(d);
    }
}

/* ----------------------------------------------------------------
 *  Public API  (matches huffman_encode / huffman_decode contract)
 * ---------------------------------------------------------------- */

void range_encode(unsigned char *input, size_t len,
                  unsigned char *output, size_t *out_len)
{
    /* 1. frequency count */
    uint32_t freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[input[i]]++;

    /* 2. write 1024-byte frequency header (uint32 LE per symbol) */
    for (int s = 0; s < 256; s++) {
        uint32_t f = freq[s];
        output[4*s + 0] = (unsigned char)( f        & 0xFFu);
        output[4*s + 1] = (unsigned char)((f >>  8) & 0xFFu);
        output[4*s + 2] = (unsigned char)((f >> 16) & 0xFFu);
        output[4*s + 3] = (unsigned char)((f >> 24) & 0xFFu);
    }

    if (len == 0) {
        *out_len = RC_HEADER_SIZE;
        return;
    }

    /* 3. cumulative frequencies */
    uint32_t cum[257];
    cum[0] = 0;
    for (int s = 0; s < 256; s++) cum[s + 1] = cum[s] + freq[s];
    uint32_t total = cum[256];                 /* == len, fits in 32 bits */

    /* 4. range-encode every byte */
    RangeEncoder e;
    re_init(&e, output + RC_HEADER_SIZE);
    for (size_t i = 0; i < len; i++) {
        unsigned char b = input[i];
        re_encode_sym(&e, cum[b], freq[b], total);
    }
    re_finish(&e);

    *out_len = RC_HEADER_SIZE + e.out_pos;
}

void range_decode(unsigned char *input, size_t len,
                  unsigned char *output, size_t *out_len)
{
    /* Caller pre-sets *out_len to the expected number of output bytes,
     * mirroring huffman_decode's contract. */
    size_t expected = *out_len;

    if (len < RC_HEADER_SIZE) {
        fprintf(stderr, "range_decode: payload shorter than freq header\n");
        *out_len = 0;
        return;
    }

    /* 1. read frequency header */
    uint32_t freq[256];
    for (int s = 0; s < 256; s++) {
        freq[s] =  (uint32_t)input[4*s + 0]
                | ((uint32_t)input[4*s + 1] <<  8)
                | ((uint32_t)input[4*s + 2] << 16)
                | ((uint32_t)input[4*s + 3] << 24);
    }

    if (expected == 0) { *out_len = 0; return; }

    /* 2. cumulative table */
    uint32_t cum[257];
    cum[0] = 0;
    for (int s = 0; s < 256; s++) cum[s + 1] = cum[s] + freq[s];
    uint32_t total = cum[256];

    if (total == 0) {
        fprintf(stderr,
            "range_decode: empty freq table but %zu bytes expected\n", expected);
        *out_len = 0;
        return;
    }

    /* 3. decode `expected` symbols */
    RangeDecoder d;
    rd_init(&d, input + RC_HEADER_SIZE, len - RC_HEADER_SIZE);

    for (size_t i = 0; i < expected; i++) {
        uint32_t cf = rd_get_cum(&d, total);

        /* binary search: find sym with cum[sym] <= cf < cum[sym+1] */
        int lo = 0, hi = 256;
        while (hi - lo > 1) {
            int mid = (lo + hi) >> 1;
            if (cum[mid] <= cf) lo = mid;
            else                hi = mid;
        }
        int sym = lo;

        output[i] = (unsigned char)sym;
        rd_advance(&d, cum[sym], freq[sym]);
    }

    *out_len = expected;
}
