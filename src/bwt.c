#include <stdlib.h>
#include <string.h>
#include "bzip2.h"

/* ================================================================
 *  Burrows-Wheeler Transform — two encoder backends
 *
 *    1. Matrix method   (Stage 1, spec §3.3):  O(n^2 log n)
 *    2. Suffix array    (Extra credit §8.2):    O(n log^2 n)
 *
 *  The two share an identical decoder (LF-mapping below).  They
 *  may emit different (L, primary_index) pairs on degenerate
 *  periodic input, but the LF decoder reconstructs the original
 *  in either case.
 *
 *  bwt_encode() dispatches based on a runtime flag set by
 *  bwt_set_use_suffix_array(), which main.c configures from
 *  config.ini's bwt_type setting.
 * ================================================================ */

/* ----------------------------------------------------------------
 *  Backend selection
 * ---------------------------------------------------------------- */

static int g_use_sa = 0;

void bwt_set_use_suffix_array(int use_sa)
{
    g_use_sa = use_sa ? 1 : 0;
}

/* ----------------------------------------------------------------
 *  Matrix method (Stage 1 spec, §3.3)
 * ---------------------------------------------------------------- */

static const unsigned char *g_bwt_input = NULL;
static size_t                g_bwt_len   = 0;

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

static void bwt_encode_matrix(unsigned char *input, size_t len,
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
        output[i] = input[((size_t)indices[i] + len - 1) % len];
        if (indices[i] == 0)
            *primary_index = (int)i;
    }

    free(indices);
    g_bwt_input = NULL;
    g_bwt_len   = 0;
}

/* ----------------------------------------------------------------
 *  Suffix-array method (Extra credit §8.2)
 *
 *  Prefix-doubling suffix-array construction (Manber-Myers style)
 *  using qsort.  At iteration k = 1, 2, 4, 8, ... each suffix is
 *  ranked by its first k characters; suffixes with identical
 *  k-prefix are then re-ranked using their (k+1 .. 2k) range, by
 *  comparing (rank[i], rank[i+k]) tuples.  After O(log n) rounds
 *  every suffix has a unique rank and SA is sorted.
 *
 *  Per-round qsort makes this O(n log^2 n) — for n = 500 KB:
 *    500K · log²(500K)  ≈  180 M ops, runs in well under a second.
 * ---------------------------------------------------------------- */

static const int *g_sa_rank = NULL;
static int        g_sa_n    = 0;
static int        g_sa_k    = 0;

static int sa_compare(const void *a, const void *b)
{
    int ia = *(const int *)a;
    int ib = *(const int *)b;

    int ra = g_sa_rank[ia];
    int rb = g_sa_rank[ib];
    if (ra != rb) return (ra > rb) - (ra < rb);

    int ra2 = (ia + g_sa_k < g_sa_n) ? g_sa_rank[ia + g_sa_k] : -1;
    int rb2 = (ib + g_sa_k < g_sa_n) ? g_sa_rank[ib + g_sa_k] : -1;
    return (ra2 > rb2) - (ra2 < rb2);
}

int *build_suffix_array(unsigned char *text, int n)
{
    if (n <= 0) return NULL;

    int *sa       = (int *)malloc((size_t)n * sizeof(int));
    int *rank     = (int *)malloc((size_t)n * sizeof(int));
    int *new_rank = (int *)malloc((size_t)n * sizeof(int));

    for (int i = 0; i < n; i++) {
        sa[i]   = i;
        rank[i] = (int)text[i];
    }

    g_sa_rank = rank;
    g_sa_n    = n;

    for (int k = 1; ; k *= 2) {
        g_sa_k = k;
        qsort(sa, (size_t)n, sizeof(int), sa_compare);

        new_rank[sa[0]] = 0;
        for (int i = 1; i < n; i++) {
            int prev = sa[i - 1], curr = sa[i];
            int r = new_rank[prev] + (sa_compare(&prev, &curr) ? 1 : 0);
            new_rank[curr] = r;
        }
        memcpy(rank, new_rank, (size_t)n * sizeof(int));

        if (rank[sa[n - 1]] == n - 1) break;   /* all suffixes uniquely ranked */
        if (k >= n) break;                     /* defensive — shouldn't trigger */
    }

    free(rank);
    free(new_rank);
    g_sa_rank = NULL;
    return sa;
}

/* Cyclic suffix array via the doubled-string trick.
 *
 * The matrix BWT sorts cyclic rotations of T.  A linear suffix array
 * of T sorts linear suffixes — these orderings DIFFER whenever T has
 * any internal periodicity (very common after RLE-1 of repeated runs),
 * because shorter suffixes become prefixes of longer ones and end up
 * earlier in lex order than the corresponding cyclic rotation would.
 *
 * Trick: build the linear SA of T||T (T concatenated with itself), then
 * keep only the entries with original index in [0, n).  Each such
 * suffix has length >= n in T||T, so its first-n-chars window is
 * exactly the cyclic rotation of T starting at that position — and
 * the linear lex order of those windows is the cyclic rotation order
 * we want.
 *
 * Cost: 2x memory and 2x time vs the linear SA, but still O(n log^2 n)
 * and well within budget for spec-sized blocks (100 KB – 900 KB).
 */
static int *build_cyclic_suffix_array(unsigned char *text, int n)
{
    if (n <= 0) return NULL;
    if (n == 1) {
        int *sa = (int *)malloc(sizeof(int));
        sa[0] = 0;
        return sa;
    }

    int twice = 2 * n;
    unsigned char *doubled = (unsigned char *)malloc((size_t)twice);
    memcpy(doubled,         text, (size_t)n);
    memcpy(doubled + n,     text, (size_t)n);

    int *sa_doubled = build_suffix_array(doubled, twice);

    int *sa = (int *)malloc((size_t)n * sizeof(int));
    int j = 0;
    for (int i = 0; i < twice && j < n; i++) {
        if (sa_doubled[i] < n) sa[j++] = sa_doubled[i];
    }

    free(doubled);
    free(sa_doubled);
    return sa;
}

void bwt_encode_sa(unsigned char *input, size_t len,
                   unsigned char *output, int *primary_index)
{
    if (len == 0) { *primary_index = 0; return; }
    if (len == 1) { *primary_index = 0; output[0] = input[0]; return; }

    int *sa = build_cyclic_suffix_array(input, (int)len);

    *primary_index = -1;
    for (size_t i = 0; i < len; i++) {
        int idx = sa[i];
        output[i] = (idx == 0) ? input[len - 1] : input[(size_t)idx - 1];
        if (idx == 0) *primary_index = (int)i;
    }

    free(sa);
}

/* ----------------------------------------------------------------
 *  Public dispatcher (matches Stage 1 spec signature)
 * ---------------------------------------------------------------- */

void bwt_encode(unsigned char *input, size_t len,
                unsigned char *output, int *primary_index)
{
    if (g_use_sa) bwt_encode_sa     (input, len, output, primary_index);
    else          bwt_encode_matrix (input, len, output, primary_index);
}

/* ----------------------------------------------------------------
 *  Inverse BWT works for either encoder backend
 *
 *  Given L (last column) and primary_index:
 *    1. C[c] = number of symbols in L that are strictly less than c.
 *    2. LF[i] = C[L[i]] + rank(L[i], i)
 *    3. Walk LF from primary_index backwards to recover the original.
 *
 *  Verified with the classic BANANA example:
 *    L = "NNBAAA", primary = 3  ->  "BANANA"
 * ---------------------------------------------------------------- */

void bwt_decode(unsigned char *input, size_t len,
                int primary_index, unsigned char *output)
{
    if (len == 0) return;

    int freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[(unsigned char)input[i]]++;

    int C[256];
    int total = 0;
    for (int c = 0; c < 256; c++) {
        C[c]  = total;
        total += freq[c];
    }

    int *LF   = (int *)malloc(len * sizeof(int));
    int  rank[256] = {0};

    for (size_t i = 0; i < len; i++) {
        unsigned char c = input[i];
        LF[i] = C[(int)c] + rank[(int)c];
        rank[(int)c]++;
    }

    int idx = primary_index;
    for (int i = (int)len - 1; i >= 0; i--) {
        output[i] = input[idx];
        idx       = LF[idx];
    }

    free(LF);
}
 