#include <stdlib.h>
#include <string.h>
#include "bzip2.h"

/* Global state for qsort comparator (matrix BWT approach) */
static const unsigned char *g_bwt_input = NULL;
static size_t                g_bwt_len   = 0;

/*
 * Compare two cyclic rotations of the global input string.
 * Rotation 'a' starts at index *ia, rotation 'b' at index *ib.
 *
 * NOTE: This is O(n) per call -> O(n^2 log n) total sort.
 * Acceptable for the "matrix" bwt_type required by Stage 1.
 * The suffix-array method (extra credit) would be O(n log n).
 */
int compare_rotations(const void *a, const void *b)
{
    int ia = *(const int *)a;
    int ib = *(const int *)b;

    for (size_t k = 0; k < g_bwt_len; k++) {
        unsigned char ca = g_bwt_input[((size_t)ia + k) % g_bwt_len];
        unsigned char cb = g_bwt_input[((size_t)ib + k) % g_bwt_len];
        if (ca != cb) return (int)ca - (int)cb;
    }
    return 0;
}

/*
 * Forward BWT transform (matrix method).
 *
 * Algorithm:
 *   1. Create index array [0 .. len-1] representing all cyclic rotations.
 *   2. Sort indices by their corresponding cyclic rotation.
 *   3. Output[i] = last character of the i-th sorted rotation
 *            = input[(sorted_index[i] + len - 1) % len]
 *   4. Record the row whose rotation == original string (starting index 0).
 */
void bwt_encode(unsigned char *input, size_t len,
                unsigned char *output, int *primary_index)
{
    if (len == 0) { *primary_index = 0; return; }

    g_bwt_input = input;
    g_bwt_len   = len;

    int *indices = (int *)malloc(len * sizeof(int));
    for (size_t i = 0; i < len; i++) indices[i] = (int)i;

    qsort(indices, len, sizeof(int), compare_rotations);

    *primary_index = -1;
    for (size_t i = 0; i < len; i++) {
        /* last char of rotation starting at indices[i] */
        output[i] = input[((size_t)indices[i] + len - 1) % len];
        if (indices[i] == 0)
            *primary_index = (int)i;
    }

    free(indices);
    g_bwt_input = NULL;
    g_bwt_len   = 0;
}

/*
 * Inverse BWT transform using the LF-mapping.
 *
 * Given L (last column) and the primary index:
 *   1. Compute C[c] = number of symbols in L that are strictly < c.
 *   2. Build LF[i] = C[L[i]] + rank(L[i], i)
 *      where rank = number of occurrences of L[i] in L[0..i-1]
 *   3. Reconstruct original string by following LF from primary_index backwards.
 *
 * Verified with classic "BANANA" example:
 *   L = "NNBAAA", primary = 3  ->  original = "BANANA"
 */
void bwt_decode(unsigned char *input, size_t len,
                int primary_index, unsigned char *output)
{
    if (len == 0) return;

    /* Step 1 – frequency count and cumulative starts (C array) */
    int freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[(unsigned char)input[i]]++;

    int C[256];
    int total = 0;
    for (int c = 0; c < 256; c++) {
        C[c]  = total;
        total += freq[c];
    }

    /* Step 2 – build LF mapping */
    int *LF   = (int *)malloc(len * sizeof(int));
    int  rank[256] = {0};

    for (size_t i = 0; i < len; i++) {
        unsigned char c = input[i];
        LF[i] = C[(int)c] + rank[(int)c];
        rank[(int)c]++;
    }

    /* Step 3 – reconstruct by walking LF from primary_index */
    int idx = primary_index;
    for (int i = (int)len - 1; i >= 0; i--) {
        output[i] = input[idx];
        idx       = LF[idx];
    }

    free(LF);
}
