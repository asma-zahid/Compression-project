#include "bzip2.h"

/* ================================================================
 *  Stage 3 – Canonical Huffman Coding  (to be implemented)
 * ================================================================ */

void build_huffman_tree(int *frequencies, HuffmanNode **root)
{
    /* TODO Stage 3 */
    (void)frequencies; (void)root;
}

void generate_canonical_codes(HuffmanNode *root, HuffmanCode *codes)
{
    /* TODO Stage 3 */
    (void)root; (void)codes;
}

void huffman_encode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len)
{
    /* TODO Stage 3 */
    (void)input; (void)len; (void)output;
    *out_len = 0;
}

void huffman_decode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len)
{
    /* TODO Stage 3 */
    (void)input; (void)len; (void)output;
    *out_len = 0;
}

void write_header(HuffmanCode *codes, unsigned char *output, size_t *out_len)
{
    /* TODO Stage 3 */
    (void)codes; (void)output;
    *out_len = 0;
}

void encode_data(unsigned char *input, size_t len,
                 HuffmanCode *codes,
                 unsigned char *output, size_t *out_len)
{
    /* TODO Stage 3 */
    (void)input; (void)len; (void)codes; (void)output;
    *out_len = 0;
}
