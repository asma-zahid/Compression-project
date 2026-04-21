#include "bzip2.h"

/* ================================================================
 *  Stage 2 – Move-to-Front transform  (to be implemented)
 * ================================================================ */

void mtf_encode(unsigned char *input, size_t len, unsigned char *output)
{
    /* TODO Stage 2 */
    (void)input; (void)len; (void)output;
}

void mtf_decode(unsigned char *input, size_t len, unsigned char *output)
{
    /* TODO Stage 2 */
    (void)input; (void)len; (void)output;
}

/* ================================================================
 *  Stage 2 – RLE-2  (to be implemented)
 * ================================================================ */

void rle2_encode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len)
{
    /* TODO Stage 2 */
    (void)input; (void)len; (void)output;
    *out_len = 0;
}

void rle2_decode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len)
{
    /* TODO Stage 2 */
    (void)input; (void)len; (void)output;
    *out_len = 0;
}
