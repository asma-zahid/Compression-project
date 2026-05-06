#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bzip2.h"

/* ================================================================
 *  Stage 3 – Canonical Huffman Coding
 * ================================================================
 *
 *  Pipeline position:  RLE-2 output  ->  Huffman encode  ->  file
 *
 *  Algorithm summary:
 *    1. Count byte frequencies in the RLE-2 stream.
 *    2. Build a Huffman tree via a min-heap.
 *    3. Extract code lengths from the tree (depth of each leaf).
 *    4. Convert lengths into canonical codes (RFC 1951 §3.2.2).
 *    5. Emit a 256-byte header (one length per symbol, 0 = unused).
 *    6. Emit the data as a bit-stream, MSB-first within each byte.
 *
 *  Decoding rebuilds the canonical codes from the 256-byte header
 *  and decodes the bit-stream symbol-by-symbol.  The number of
 *  output symbols expected is supplied by the caller via *out_len
 *  (set before the call); this is the rle2_size stored in the
 *  per-block record.
 *
 *  Bit-ordering convention: MSB-first inside each byte.  The first
 *  bit written goes to position 0x80 of the first output byte.
 * ================================================================ */

/* Practical upper bound on Huffman code length for a 256-symbol
 * alphabet.  Real-world inputs after MTF+RLE-2 stay well below 16. */
#define HUFF_MAX_LEN 32

/* ----------------------------------------------------------------
 *  Bit I/O (internal helpers)
 * ---------------------------------------------------------------- */

typedef struct {
    unsigned char *buf;
    size_t  byte_pos;
    int     bit_pos;   /* 0..7  (0 = highest bit of current byte) */
} BitWriter;

static void bw_init(BitWriter *w, unsigned char *buf)
{
    w->buf      = buf;
    w->byte_pos = 0;
    w->bit_pos  = 0;
    w->buf[0]   = 0;
}

static void bw_write_bits(BitWriter *w, unsigned int code, int nbits)
{
    /* Emit `nbits` bits of `code` MSB-first. */
    for (int i = nbits - 1; i >= 0; i--) {
        unsigned int bit = (code >> i) & 1u;
        if (bit)
            w->buf[w->byte_pos] |= (unsigned char)(1u << (7 - w->bit_pos));

        if (++w->bit_pos == 8) {
            w->bit_pos = 0;
            w->byte_pos++;
            w->buf[w->byte_pos] = 0;
        }
    }
}

/* Returns total number of bytes written (rounded up to byte boundary). */
static size_t bw_finish(BitWriter *w)
{
    return w->byte_pos + (w->bit_pos > 0 ? 1 : 0);
}

typedef struct {
    const unsigned char *buf;
    size_t  cap;
    size_t  byte_pos;
    int     bit_pos;
} BitReader;

static void br_init(BitReader *r, const unsigned char *buf, size_t cap)
{
    r->buf      = buf;
    r->cap      = cap;
    r->byte_pos = 0;
    r->bit_pos  = 0;
}

static int br_read_bit(BitReader *r)
{
    if (r->byte_pos >= r->cap) return 0;
    int bit = (r->buf[r->byte_pos] >> (7 - r->bit_pos)) & 1;
    if (++r->bit_pos == 8) {
        r->bit_pos = 0;
        r->byte_pos++;
    }
    return bit;
}

/* ----------------------------------------------------------------
 *  Min-heap (internal helper for tree construction)
 * ---------------------------------------------------------------- */

typedef struct {
    HuffmanNode *node;
    int          freq;
} HeapEntry;

static void heap_swap(HeapEntry *a, HeapEntry *b)
{
    HeapEntry t = *a; *a = *b; *b = t;
}

static void heap_push(HeapEntry *h, int *size, HuffmanNode *node, int freq)
{
    int i = (*size)++;
    h[i].node = node;
    h[i].freq = freq;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h[p].freq <= h[i].freq) break;
        heap_swap(&h[p], &h[i]);
        i = p;
    }
}

static HeapEntry heap_pop(HeapEntry *h, int *size)
{
    HeapEntry top = h[0];
    h[0] = h[--(*size)];
    int i = 0;
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, best = i;
        if (l < *size && h[l].freq < h[best].freq) best = l;
        if (r < *size && h[r].freq < h[best].freq) best = r;
        if (best == i) break;
        heap_swap(&h[i], &h[best]);
        i = best;
    }
    return top;
}

/* ----------------------------------------------------------------
 *  Tree allocation helpers
 * ---------------------------------------------------------------- */

static HuffmanNode *new_leaf(unsigned char symbol, int freq)
{
    HuffmanNode *n = (HuffmanNode *)malloc(sizeof(HuffmanNode));
    n->symbol = symbol;
    n->freq   = freq;
    n->left   = n->right = NULL;
    return n;
}

static HuffmanNode *new_internal(HuffmanNode *left, HuffmanNode *right)
{
    HuffmanNode *n = (HuffmanNode *)malloc(sizeof(HuffmanNode));
    n->symbol = 0;
    n->freq   = (left ? left->freq : 0) + (right ? right->freq : 0);
    n->left   = left;
    n->right  = right;
    return n;
}

static void free_tree(HuffmanNode *n)
{
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}

static void extract_lengths(HuffmanNode *n, int depth, unsigned char *lengths)
{
    if (!n) return;
    if (!n->left && !n->right) {
        /* leaf: depth==0 only happens for a single-symbol input where
         * we wrap it in a parent above, so we never write 0 here. */
        lengths[n->symbol] = (unsigned char)(depth > 0 ? depth : 1);
        return;
    }
    extract_lengths(n->left,  depth + 1, lengths);
    extract_lengths(n->right, depth + 1, lengths);
}

/* ----------------------------------------------------------------
 *  Public functions  (Stage 3 spec)
 * ---------------------------------------------------------------- */

void build_huffman_tree(int *frequencies, HuffmanNode **root)
{
    HeapEntry heap[512];                /* max 256 leaves + 255 internals */
    int size = 0;

    for (int s = 0; s < 256; s++) {
        if (frequencies[s] > 0) {
            HuffmanNode *leaf = new_leaf((unsigned char)s, frequencies[s]);
            heap_push(heap, &size, leaf, frequencies[s]);
        }
    }

    if (size == 0) { *root = NULL; return; }

    /* Single-symbol input: wrap so the lone leaf has depth 1. */
    if (size == 1) {
        HuffmanNode *only = heap_pop(heap, &size).node;
        *root = new_internal(only, NULL);
        return;
    }

    while (size > 1) {
        HeapEntry a = heap_pop(heap, &size);
        HeapEntry b = heap_pop(heap, &size);
        HuffmanNode *parent = new_internal(a.node, b.node);
        heap_push(heap, &size, parent, parent->freq);
    }

    *root = heap_pop(heap, &size).node;
}

void generate_canonical_codes(HuffmanNode *root, HuffmanCode *codes)
{
    unsigned char lengths[256] = {0};
    if (root) extract_lengths(root, 0, lengths);

    for (int s = 0; s < 256; s++) {
        codes[s].code   = 0;
        codes[s].length = lengths[s];
    }

    /* Histogram of code lengths (0 ignored for code generation). */
    int bl_count[HUFF_MAX_LEN + 1] = {0};
    int max_len = 0;
    for (int s = 0; s < 256; s++) {
        int l = lengths[s];
        if (l > HUFF_MAX_LEN) {
            fprintf(stderr,
                "huffman: code length %d exceeds %d (symbol 0x%02x)\n",
                l, HUFF_MAX_LEN, s);
            l = HUFF_MAX_LEN;
            codes[s].length = (unsigned char)l;
        }
        bl_count[l]++;
        if (l > max_len) max_len = l;
    }
    bl_count[0] = 0;
    if (max_len == 0) return;

    /* RFC 1951 §3.2.2: first canonical code at each length. */
    unsigned int next_code[HUFF_MAX_LEN + 1] = {0};
    unsigned int code = 0;
    for (int len = 1; len <= max_len; len++) {
        code = (code + (unsigned int)bl_count[len - 1]) << 1;
        next_code[len] = code;
    }

    /* Assign codes in (length, symbol) order – sequential within each length. */
    for (int s = 0; s < 256; s++) {
        int l = lengths[s];
        if (l > 0) {
            codes[s].code = next_code[l];
            next_code[l]++;
        }
    }
}

void write_header(HuffmanCode *codes, unsigned char *output, size_t *out_len)
{
    for (int s = 0; s < 256; s++)
        output[s] = codes[s].length;
    *out_len = 256;
}

void encode_data(unsigned char *input, size_t len,
                 HuffmanCode *codes,
                 unsigned char *output, size_t *out_len)
{
    BitWriter w;
    bw_init(&w, output);

    for (size_t i = 0; i < len; i++) {
        unsigned char b = input[i];
        bw_write_bits(&w, codes[b].code, codes[b].length);
    }

    *out_len = bw_finish(&w);
}

void huffman_encode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len)
{
    /* Edge case: empty input – emit a 256-byte zero header and stop. */
    if (len == 0) {
        memset(output, 0, 256);
        *out_len = 256;
        return;
    }

    /* 1. Frequency count */
    int freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[input[i]]++;

    /* 2. Build tree */
    HuffmanNode *root = NULL;
    build_huffman_tree(freq, &root);

    /* 3. Generate canonical codes */
    HuffmanCode codes[256];
    generate_canonical_codes(root, codes);

    /* 4. Write 256-byte length header */
    size_t header_sz = 0;
    write_header(codes, output, &header_sz);

    /* 5. Emit encoded data immediately after the header */
    size_t data_sz = 0;
    encode_data(input, len, codes, output + header_sz, &data_sz);

    *out_len = header_sz + data_sz;

    free_tree(root);
}

void huffman_decode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len)
{
    /* Caller pre-sets *out_len to the expected number of output bytes
     * (= rle2_size from the per-block record).  We stop when produced. */
    size_t expected = *out_len;

    if (len < 256) {
        fprintf(stderr, "huffman_decode: payload shorter than header\n");
        *out_len = 0;
        return;
    }

    /* 1. Read code-length header */
    unsigned char lengths[256];
    memcpy(lengths, input, 256);

    /* 2. Histogram + rebuild canonical first-code table */
    int bl_count[HUFF_MAX_LEN + 1] = {0};
    int max_len = 0;
    for (int s = 0; s < 256; s++) {
        int l = lengths[s];
        if (l > HUFF_MAX_LEN) {
            fprintf(stderr,
                "huffman_decode: corrupt header (length %d > %d)\n",
                l, HUFF_MAX_LEN);
            *out_len = 0;
            return;
        }
        bl_count[l]++;
        if (l > max_len) max_len = l;
    }
    bl_count[0] = 0;

    if (expected > 0 && max_len == 0) {
        fprintf(stderr, "huffman_decode: empty code table but %zu bytes expected\n", expected);
        *out_len = 0;
        return;
    }

    unsigned int first_code[HUFF_MAX_LEN + 1] = {0};
    int          first_idx [HUFF_MAX_LEN + 1] = {0};

    /* Symbols sorted by (length, symbol) – this is the canonical order. */
    int sorted_syms[256];
    int idx = 0;
    {
        unsigned int code = 0;
        for (int l = 1; l <= max_len; l++) {
            code = (code + (unsigned int)bl_count[l - 1]) << 1;
            first_code[l] = code;
            first_idx[l]  = idx;
            for (int s = 0; s < 256; s++) {
                if (lengths[s] == l) sorted_syms[idx++] = s;
            }
        }
    }

    /* 3. Decode bit-stream after header */
    BitReader r;
    br_init(&r, input + 256, len - 256);

    size_t produced = 0;
    while (produced < expected) {
        unsigned int v = 0;
        int l = 0;
        for (;;) {
            v = (v << 1) | (unsigned int)br_read_bit(&r);
            l++;
            if (l > max_len) {
                fprintf(stderr,
                    "huffman_decode: malformed bit-stream at output byte %zu\n",
                    produced);
                *out_len = produced;
                return;
            }
            if (bl_count[l] > 0 &&
                v >= first_code[l] &&
                v <  first_code[l] + (unsigned int)bl_count[l]) {
                int sidx = first_idx[l] + (int)(v - first_code[l]);
                output[produced++] = (unsigned char)sorted_syms[sidx];
                break;
            }
        }
    }

    *out_len = produced;
}
