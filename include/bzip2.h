#ifndef BZIP2_H
#define BZIP2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

/* ================================================================
 *  Data Structures  (provided by spec)
 * ================================================================ */

typedef struct {
    unsigned char *data;
    size_t size;
    size_t original_size;
} Block;

typedef struct {
    Block  *blocks;
    int     num_blocks;
    size_t  block_size;
} BlockManager;

/* BWT rotation descriptor */
typedef struct {
    char *rotation;   /* kept for spec compliance; not stored in practice */
    int   index;
} Rotation;

/* Huffman structures (Stage 3) */
typedef struct {
    unsigned int   code;    /* widened from unsigned short — codes can exceed 16 bits */
    unsigned char  length;
} HuffmanCode;

typedef struct Node {
    unsigned char  symbol;
    int            freq;
    struct Node   *left;
    struct Node   *right;
} HuffmanNode;

/* Runtime configuration parsed from config.ini */
typedef struct {
    size_t block_size;
    int    rle1_enabled;
    char   bwt_type[32];
    int    mtf_enabled;
    int    rle2_enabled;
    int    huffman_enabled;
    int    benchmark_mode;
    int    output_metrics;
    char   input_directory[256];
    char   output_directory[256];

    /* Extra-credit (§8): enhanced RLE-1 and alternative entropy coder. */
    int    rle1_enhanced;    /* §8.1 — use threshold + adaptive RLE-1   */
    int    rle1_threshold;   /* minimum run length for encoding (>=2)   */
    int    rle1_adaptive;    /* fall back to raw block when expanding   */
    char   entropy_coder[16];/* §8.3 — "huffman" (default) or "range"   */
} Config;

/* ================================================================
 *  Block Management  (Stage 1)
 * ================================================================ */
BlockManager *divide_into_blocks(const char *filename, size_t block_size);
int           reassemble_blocks(BlockManager *manager, const char *output_filename);
void          free_block_manager(BlockManager *manager);

/* ================================================================
 *  RLE-1  (Stage 1)
 * ================================================================ */
void rle1_encode(unsigned char *input,  size_t len,
                 unsigned char *output, size_t *out_len);
void rle1_decode(unsigned char *input,  size_t len,
                 unsigned char *output, size_t *out_len);

/* Enhanced RLE-1 (extra credit §8.1):
 *   - Threshold encoding: runs >= `threshold` are followed by a 0..255
 *     "additional repetitions" count byte (bzip2-style). Shorter runs
 *     pass through as literals — no expansion on random data.
 *   - Adaptive bypass: if the encoded payload would be no smaller than
 *     the raw block, the block is stored uncompressed with a sentinel.
 *
 * Self-describing output:
 *     byte 0 : 0x00 = raw block, 0x01 = threshold-encoded
 *     if 0x01:  byte 1 = threshold, bytes 2.. = encoded payload
 *     if 0x00:  bytes 1.. = original data verbatim
 *
 * Threshold is clamped to [2, 255].
 */
void rle1_encode_enh(unsigned char *input,  size_t len,
                     unsigned char *output, size_t *out_len,
                     int threshold, int adaptive);
void rle1_decode_enh(unsigned char *input,  size_t len,
                     unsigned char *output, size_t *out_len);

/* ================================================================
 *  Burrows-Wheeler Transform  (Stage 1)
 * ================================================================ */
int  compare_rotations(const void *a, const void *b);
void bwt_encode(unsigned char *input,  size_t len,
                unsigned char *output, int *primary_index);
void bwt_decode(unsigned char *input,  size_t len,
                int primary_index, unsigned char *output);

/* Suffix-array based BWT (extra credit §8.2).  O(n log^2 n) vs
 * matrix BWT's O(n^2 log n) — enables block sizes 100KB–900KB. */
int *build_suffix_array(unsigned char *text, int n);
void bwt_encode_sa(unsigned char *input, size_t len,
                   unsigned char *output, int *primary_index);

/* Selects which BWT encoder bwt_encode dispatches to.
 * 0 = matrix (default, Stage 1 spec), 1 = suffix array. */
void bwt_set_use_suffix_array(int use_sa);

/* ================================================================
 *  Move-to-Front  (Stage 2 – stubs in stage 1)
 * ================================================================ */
void mtf_encode(unsigned char *input, size_t len, unsigned char *output);
void mtf_decode(unsigned char *input, size_t len, unsigned char *output);

/* ================================================================
 *  RLE-2  (Stage 2 – stubs in stage 1)
 * ================================================================ */
void rle2_encode(unsigned char *input,  size_t len,
                 unsigned char *output, size_t *out_len);
void rle2_decode(unsigned char *input,  size_t len,
                 unsigned char *output, size_t *out_len);

/* ================================================================
 *  Canonical Huffman Coding  (Stage 3 – stubs in stage 1)
 * ================================================================ */
void build_huffman_tree(int *frequencies, HuffmanNode **root);
void generate_canonical_codes(HuffmanNode *root, HuffmanCode *codes);
void huffman_encode(unsigned char *input,  size_t len,
                    unsigned char *output, size_t *out_len);
void huffman_decode(unsigned char *input,  size_t len,
                    unsigned char *output, size_t *out_len);
void write_header(HuffmanCode *codes, unsigned char *output, size_t *out_len);
void encode_data(unsigned char *input, size_t len,
                 HuffmanCode *codes,
                 unsigned char *output, size_t *out_len);

/* ================================================================
 *  Range Coding  (extra credit §8.3 — alternative entropy coder)
 * ================================================================
 *  Static order-0 model. Payload layout:
 *      1024 bytes : 256 * uint32_t LE symbol frequencies
 *       N   bytes : range-coded data
 *  *out_len for decode must be pre-set to the expected output size
 *  (same convention as huffman_decode).
 */
void range_encode(unsigned char *input,  size_t len,
                  unsigned char *output, size_t *out_len);
void range_decode(unsigned char *input,  size_t len,
                  unsigned char *output, size_t *out_len);

/* ================================================================
 *  Configuration
 * ================================================================ */
Config *load_config(const char *filename);
void    free_config(Config *cfg);
void    print_config(const Config *cfg);

#endif /* BZIP2_H */
