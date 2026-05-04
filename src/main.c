#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "bzip2.h"

/* ----------------------------------------------------------------
 * Compressed-file header layout  (Stage 3)
 *
 * File header (13 bytes)
 *   0   4   Magic bytes: "MYBZ"
 *   4   1   Stage version (3 = full pipeline: RLE-1 + BWT + MTF + RLE-2 + Huffman)
 *   5   4   block_size used during compression  (uint32_t LE)
 *   9   4   num_blocks                          (uint32_t LE)
 *
 * Per-block record (20-byte header + payload)
 *   0   4   original block size, bytes              (uint32_t LE)
 *   4   4   rle1-encoded size = BWT/MTF buffer size (uint32_t LE)
 *   8   4   BWT primary index                       (int32_t  LE)
 *  12   4   rle2-encoded size, bytes                (uint32_t LE)
 *  16   4   huffman payload size, bytes             (uint32_t LE)
 *  20   N   Huffman payload  (N = huffman_size)
 *           [first 256 bytes are the canonical code-length header,
 *            remainder is the bit-stream MSB-first]
 * ---------------------------------------------------------------- */

#define MAGIC         "MYBZ"
#define STAGE_VERSION ((uint8_t)3)

/* Returns file size in bytes, or -1 on error */
static long file_size(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz;
}

/* ------------------------------------------------------------------ */
static int compress_file(const char *in_path, const char *out_path,
                          Config *cfg)
{
    clock_t t0 = clock();

    BlockManager *mgr = divide_into_blocks(in_path, cfg->block_size);
    if (!mgr) return -1;

    FILE *fout = fopen(out_path, "wb");
    if (!fout) {
        perror("compress: cannot open output file");
        free_block_manager(mgr);
        return -1;
    }

    /* --- file header --- */
    fwrite(MAGIC, 1, 4, fout);
    uint8_t  ver  = STAGE_VERSION;
    uint32_t bsz  = (uint32_t)cfg->block_size;
    uint32_t nb   = (uint32_t)mgr->num_blocks;
    fwrite(&ver,  sizeof(ver),  1, fout);
    fwrite(&bsz,  sizeof(bsz),  1, fout);
    fwrite(&nb,   sizeof(nb),   1, fout);

    for (int i = 0; i < mgr->num_blocks; i++) {
        Block *blk       = &mgr->blocks[i];
        size_t orig_size = blk->size;

        /* --- RLE-1 encode --- */
        unsigned char *rle1_buf = (unsigned char *)malloc(orig_size * 2 + 16);
        size_t rle1_sz = 0;

        if (cfg->rle1_enabled)
            rle1_encode(blk->data, orig_size, rle1_buf, &rle1_sz);
        else {
            memcpy(rle1_buf, blk->data, orig_size);
            rle1_sz = orig_size;
        }

        /* --- BWT encode (output size == input size) --- */
        unsigned char *bwt_buf = (unsigned char *)malloc(rle1_sz + 16);
        int primary_index = 0;
        bwt_encode(rle1_buf, rle1_sz, bwt_buf, &primary_index);

        /* --- MTF encode (output size == input size) --- */
        unsigned char *mtf_buf = (unsigned char *)malloc(rle1_sz + 16);
        if (cfg->mtf_enabled)
            mtf_encode(bwt_buf, rle1_sz, mtf_buf);
        else
            memcpy(mtf_buf, bwt_buf, rle1_sz);

        /* --- RLE-2 encode (worst case 2x expansion) --- */
        unsigned char *rle2_buf = (unsigned char *)malloc(rle1_sz * 2 + 16);
        size_t rle2_sz = 0;
        if (cfg->rle2_enabled)
            rle2_encode(mtf_buf, rle1_sz, rle2_buf, &rle2_sz);
        else {
            memcpy(rle2_buf, mtf_buf, rle1_sz);
            rle2_sz = rle1_sz;
        }

        /* --- Huffman encode --- */
        /* Worst-case Huffman output = 256-byte header + ceil(rle2_sz * max_len / 8).
         * With max_len capped at 32 bits per symbol, allocate generously. */
        unsigned char *huff_buf = (unsigned char *)malloc(256 + rle2_sz * 4 + 16);
        size_t huff_sz = 0;
        if (cfg->huffman_enabled)
            huffman_encode(rle2_buf, rle2_sz, huff_buf, &huff_sz);
        else {
            /* No Huffman: write a sentinel zero header + raw payload. */
            memset(huff_buf, 0, 256);
            memcpy(huff_buf + 256, rle2_buf, rle2_sz);
            huff_sz = 256 + rle2_sz;
        }

        /* --- write block record --- */
        uint32_t orig32  = (uint32_t)orig_size;
        uint32_t rle1_32 = (uint32_t)rle1_sz;
        int32_t  pidx    = (int32_t)primary_index;
        uint32_t rle2_32 = (uint32_t)rle2_sz;
        uint32_t huff_32 = (uint32_t)huff_sz;

        fwrite(&orig32,  sizeof(orig32),  1,        fout);
        fwrite(&rle1_32, sizeof(rle1_32), 1,        fout);
        fwrite(&pidx,    sizeof(pidx),    1,        fout);
        fwrite(&rle2_32, sizeof(rle2_32), 1,        fout);
        fwrite(&huff_32, sizeof(huff_32), 1,        fout);
        fwrite(huff_buf, 1,               huff_sz,  fout);

        if (cfg->output_metrics)
            printf("  block %3d: orig=%7u  rle1=%7u  rle2=%7u  huff=%7u  "
                   "ratio=%.3f  pidx=%d\n",
                   i, orig32, rle1_32, rle2_32, huff_32,
                   (huff_sz > 0) ? (double)orig_size / (double)huff_sz : 0.0,
                   primary_index);

        free(rle1_buf);
        free(bwt_buf);
        free(mtf_buf);
        free(rle2_buf);
        free(huff_buf);
    }

    fclose(fout);
    free_block_manager(mgr);

    double elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
    long in_sz  = file_size(in_path);
    long out_sz = file_size(out_path);

    if (cfg->output_metrics && in_sz > 0 && out_sz > 0) {
        printf("Compression done: %ld -> %ld bytes  ratio=%.3f  time=%.3f s",
               in_sz, out_sz,
               (double)in_sz / (double)out_sz,
               elapsed);
        if (elapsed > 1e-6)
            printf("  speed=%.2f MB/s", (double)in_sz / (1024.0 * 1024.0) / elapsed);
        printf("\n");
    }

    return 0;
}

/* ------------------------------------------------------------------ */
static int decompress_file(const char *in_path, const char *out_path,
                             Config *cfg)
{
    clock_t t0 = clock();

    FILE *fin = fopen(in_path, "rb");
    if (!fin) {
        perror("decompress: cannot open input file");
        return -1;
    }

    /* --- verify header --- */
    char magic[4];
    if (fread(magic, 1, 4, fin) != 4 || memcmp(magic, MAGIC, 4) != 0) {
        fprintf(stderr, "decompress: not a valid MYBZ file\n");
        fclose(fin);
        return -1;
    }

    uint8_t  ver;
    uint32_t bsz, nb;
    if (fread(&ver, sizeof(ver), 1, fin) != 1 ||
        fread(&bsz, sizeof(bsz), 1, fin) != 1 ||
        fread(&nb,  sizeof(nb),  1, fin) != 1) {
        fprintf(stderr, "decompress: truncated file header\n");
        fclose(fin);
        return -1;
    }

    if (ver != STAGE_VERSION) {
        fprintf(stderr,
            "decompress: unsupported stage version %u (this build expects %u)\n",
            ver, STAGE_VERSION);
        fclose(fin);
        return -1;
    }

    if (cfg->output_metrics)
        printf("  file info: stage=%u  block_size=%u  blocks=%u\n\n",
               ver, bsz, nb);

    FILE *fout = fopen(out_path, "wb");
    if (!fout) {
        perror("decompress: cannot open output file");
        fclose(fin);
        return -1;
    }

    for (uint32_t i = 0; i < nb; i++) {
        uint32_t orig32, rle1_32, rle2_32, huff_32;
        int32_t  pidx;

        if (fread(&orig32,  sizeof(orig32),  1, fin) != 1 ||
            fread(&rle1_32, sizeof(rle1_32), 1, fin) != 1 ||
            fread(&pidx,    sizeof(pidx),    1, fin) != 1 ||
            fread(&rle2_32, sizeof(rle2_32), 1, fin) != 1 ||
            fread(&huff_32, sizeof(huff_32), 1, fin) != 1) {
            fprintf(stderr, "decompress: unexpected end of file at block %u\n", i);
            fclose(fin); fclose(fout);
            return -1;
        }

        unsigned char *huff_buf = (unsigned char *)malloc(huff_32 + 16);
        unsigned char *rle2_buf = (unsigned char *)malloc(rle2_32 + 16);
        unsigned char *mtf_buf  = (unsigned char *)malloc(rle1_32 + 16);
        unsigned char *bwt_buf  = (unsigned char *)malloc(rle1_32 + 16);
        unsigned char *rle1_buf = (unsigned char *)malloc(rle1_32 + 16);
        unsigned char *orig_buf = (unsigned char *)malloc(orig32  + 16);

        if (fread(huff_buf, 1, huff_32, fin) != huff_32) {
            fprintf(stderr, "decompress: short read at block %u\n", i);
            free(huff_buf); free(rle2_buf); free(mtf_buf);
            free(bwt_buf); free(rle1_buf); free(orig_buf);
            fclose(fin); fclose(fout);
            return -1;
        }

        /* --- inverse Huffman --- */
        size_t produced = (size_t)rle2_32;     /* expected output */
        if (cfg->huffman_enabled) {
            huffman_decode(huff_buf, huff_32, rle2_buf, &produced);
        } else {
            /* Bypass: skip 256-byte sentinel header, copy raw. */
            if (huff_32 < 256) {
                fprintf(stderr, "decompress: bypassed huffman payload too small\n");
                free(huff_buf); free(rle2_buf); free(mtf_buf);
                free(bwt_buf); free(rle1_buf); free(orig_buf);
                fclose(fin); fclose(fout);
                return -1;
            }
            memcpy(rle2_buf, huff_buf + 256, huff_32 - 256);
            produced = huff_32 - 256;
        }

        if (produced != rle2_32)
            fprintf(stderr,
                "Warning: block %u Huffman decoded %zu bytes, expected %u\n",
                i, produced, rle2_32);

        /* --- inverse RLE-2 --- */
        size_t mtf_sz = 0;
        if (cfg->rle2_enabled) {
            rle2_decode(rle2_buf, rle2_32, mtf_buf, &mtf_sz);
        } else {
            memcpy(mtf_buf, rle2_buf, rle2_32);
            mtf_sz = rle2_32;
        }

        if (mtf_sz != rle1_32)
            fprintf(stderr,
                "Warning: block %u RLE-2 decoded %zu bytes, expected %u\n",
                i, mtf_sz, rle1_32);

        /* --- inverse MTF --- */
        if (cfg->mtf_enabled)
            mtf_decode(mtf_buf, rle1_32, bwt_buf);
        else
            memcpy(bwt_buf, mtf_buf, rle1_32);

        /* --- inverse BWT --- */
        bwt_decode(bwt_buf, rle1_32, (int)pidx, rle1_buf);

        /* --- inverse RLE-1 --- */
        size_t decoded = 0;
        if (cfg->rle1_enabled)
            rle1_decode(rle1_buf, rle1_32, orig_buf, &decoded);
        else {
            memcpy(orig_buf, rle1_buf, rle1_32);
            decoded = rle1_32;
        }

        if (decoded != orig32)
            fprintf(stderr,
                "Warning: block %u RLE-1 decoded %zu bytes, expected %u\n",
                i, decoded, orig32);

        fwrite(orig_buf, 1, decoded, fout);

        free(huff_buf);
        free(rle2_buf);
        free(mtf_buf);
        free(bwt_buf);
        free(rle1_buf);
        free(orig_buf);
    }

    fclose(fin);
    fclose(fout);

    double elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
    long out_sz = file_size(out_path);

    if (cfg->output_metrics && out_sz > 0) {
        printf("Decompression done: %ld bytes  time=%.3f s", out_sz, elapsed);
        if (elapsed > 1e-6)
            printf("  speed=%.2f MB/s", (double)out_sz / (1024.0 * 1024.0) / elapsed);
        printf("\n");
    }

    return 0;
}

/* ------------------------------------------------------------------ */
static void print_usage(const char *prog)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s compress   <input>  <output.bz>\n"
        "  %s decompress <input.bz>  <output>\n",
        prog, prog);
}

int main(int argc, char *argv[])
{
    printf("=== BZip2 Implementation - Stage 3 (RLE-1 + BWT + MTF + RLE-2 + Huffman) ===\n\n");

    if (argc < 4) { print_usage(argv[0]); return 1; }

    Config *cfg = load_config("config.ini");
    if (cfg->output_metrics) { print_config(cfg); printf("\n"); }

    /* select BWT backend from config */
    bwt_set_use_suffix_array(strcmp(cfg->bwt_type, "suffix_array") == 0);

    int ret = 1;
    if (strcmp(argv[1], "compress")   == 0 || strcmp(argv[1], "-c") == 0) {
        printf("Compressing:   %s  ->  %s\n\n", argv[2], argv[3]);
        ret = compress_file(argv[2], argv[3], cfg);
    } else if (strcmp(argv[1], "decompress") == 0 || strcmp(argv[1], "-d") == 0) {
        printf("Decompressing: %s  ->  %s\n\n", argv[2], argv[3]);
        ret = decompress_file(argv[2], argv[3], cfg);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
    }

    free_config(cfg);
    printf("\n%s\n", ret == 0 ? "Done." : "Error - see messages above.");
    return ret;
}
