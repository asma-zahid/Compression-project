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
    unsigned short code;
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

/* ================================================================
 *  Burrows-Wheeler Transform  (Stage 1)
 * ================================================================ */
int  compare_rotations(const void *a, const void *b);
void bwt_encode(unsigned char *input,  size_t len,
                unsigned char *output, int *primary_index);
void bwt_decode(unsigned char *input,  size_t len,
                int primary_index, unsigned char *output);

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
 *  Configuration
 * ================================================================ */
Config *load_config(const char *filename);
void    free_config(Config *cfg);
void    print_config(const Config *cfg);

#endif /* BZIP2_H */
