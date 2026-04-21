#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "bzip2.h"

/* ----------------------------------------------------------------
 * Compressed-file header layout
 *
 * Offset  Size  Field
 *      0     4  Magic bytes: "MYBZ"
 *      4     1  Stage version (1 = Stage-1: RLE-1 + BWT)
 *      5     4  block_size used during compression (uint32_t LE)
 *      9     4  num_blocks (uint32_t LE)
 *
 * Per-block record (repeated num_blocks times):
 *      0     4  original block size, bytes (uint32_t LE)
 *      4     4  rle1_encoded size, bytes   (uint32_t LE)
 *      8     4  BWT primary index          (int32_t  LE)
 *     12  rle1  BWT-encoded payload
 * ---------------------------------------------------------------- */

#define MAGIC         "MYBZ"
#define STAGE_VERSION ((uint8_t)1)

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

        /* --- BWT encode --- */
        unsigned char *bwt_buf = (unsigned char *)malloc(rle1_sz + 16);
        int primary_index = 0;
        bwt_encode(rle1_buf, rle1_sz, bwt_buf, &primary_index);

        /* --- write block record --- */
        uint32_t orig32  = (uint32_t)orig_size;
        uint32_t rle1_32 = (uint32_t)rle1_sz;
        int32_t  pidx    = (int32_t)primary_index;

        fwrite(&orig32,  sizeof(orig32),  1,        fout);
        fwrite(&rle1_32, sizeof(rle1_32), 1,        fout);
        fwrite(&pidx,    sizeof(pidx),    1,        fout);
        fwrite(bwt_buf,  1,               rle1_sz,  fout);

        if (cfg->output_metrics)
            printf("  block %3d: orig=%7u  rle1=%7u  ratio=%.3f  pidx=%d\n",
                   i, orig32, rle1_32,
                   (rle1_sz > 0) ? (double)orig_size / (double)rle1_sz : 0.0,
                   primary_index);

        free(rle1_buf);
        free(bwt_buf);
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
    fread(&ver, sizeof(ver), 1, fin);
    fread(&bsz, sizeof(bsz), 1, fin);
    fread(&nb,  sizeof(nb),  1, fin);

    if (ver != STAGE_VERSION) {
        fprintf(stderr, "decompress: unsupported stage version %u\n", ver);
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
        uint32_t orig32, rle1_32;
        int32_t  pidx;

        if (fread(&orig32,  sizeof(orig32),  1, fin) != 1 ||
            fread(&rle1_32, sizeof(rle1_32), 1, fin) != 1 ||
            fread(&pidx,    sizeof(pidx),    1, fin) != 1) {
            fprintf(stderr, "decompress: unexpected end of file at block %u\n", i);
            fclose(fin); fclose(fout);
            return -1;
        }

        unsigned char *bwt_buf  = (unsigned char *)malloc(rle1_32);
        unsigned char *rle1_buf = (unsigned char *)malloc(rle1_32 + 16);
        unsigned char *orig_buf = (unsigned char *)malloc(orig32  + 16);

        if (fread(bwt_buf, 1, rle1_32, fin) != rle1_32) {
            fprintf(stderr, "decompress: short read at block %u\n", i);
            free(bwt_buf); free(rle1_buf); free(orig_buf);
            fclose(fin); fclose(fout);
            return -1;
        }

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
                "Warning: block %u decoded %zu bytes, expected %u\n",
                i, decoded, orig32);

        fwrite(orig_buf, 1, decoded, fout);

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
    printf("=== BZip2 Implementation - Stage 1 (RLE-1 + BWT) ===\n\n");

    if (argc < 4) { print_usage(argv[0]); return 1; }

    Config *cfg = load_config("config.ini");
    if (cfg->output_metrics) { print_config(cfg); printf("\n"); }

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
