#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bzip2.h"

static char *str_trim(char *s)
{
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e)) e--;
    e[1] = '\0';
    return s;
}

Config *load_config(const char *filename)
{
    Config *cfg = (Config *)calloc(1, sizeof(Config));

    /* defaults */
    cfg->block_size      = 500000;
    cfg->rle1_enabled    = 1;
    cfg->mtf_enabled     = 1;
    cfg->rle2_enabled    = 1;
    cfg->huffman_enabled = 1;
    cfg->benchmark_mode  = 0;
    cfg->output_metrics  = 1;
    strncpy(cfg->bwt_type,         "matrix",        sizeof(cfg->bwt_type)         - 1);
    strncpy(cfg->input_directory,  "./benchmarks/", sizeof(cfg->input_directory)  - 1);
    strncpy(cfg->output_directory, "./results/",    sizeof(cfg->output_directory) - 1);

    /* extra-credit defaults: keep legacy behavior unless explicitly enabled */
    cfg->rle1_enhanced   = 0;
    cfg->rle1_threshold  = 4;
    cfg->rle1_adaptive   = 1;
    strncpy(cfg->entropy_coder, "huffman", sizeof(cfg->entropy_coder) - 1);

    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Warning: config file '%s' not found - using defaults.\n", filename);
        return cfg;
    }

    char line[512];
    char section[64] = "";

    while (fgets(line, sizeof(line), f)) {
        /* strip inline comment */
        char *comment = strchr(line, '#');
        if (comment) *comment = '\0';

        char *s = str_trim(line);
        if (*s == '\0') continue;

        if (*s == '[') {
            char *end = strchr(s, ']');
            if (end) {
                *end = '\0';
                strncpy(section, str_trim(s + 1), sizeof(section) - 1);
            }
            continue;
        }

        char *eq = strchr(s, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = str_trim(s);
        char *val = str_trim(eq + 1);

        if (strcmp(section, "General") == 0) {
            if      (strcmp(key, "block_size")      == 0) cfg->block_size      = (size_t)atol(val);
            else if (strcmp(key, "rle1_enabled")    == 0) cfg->rle1_enabled    = (strcmp(val, "true") == 0);
            else if (strcmp(key, "bwt_type")        == 0) strncpy(cfg->bwt_type, val, sizeof(cfg->bwt_type) - 1);
            else if (strcmp(key, "mtf_enabled")     == 0) cfg->mtf_enabled     = (strcmp(val, "true") == 0);
            else if (strcmp(key, "rle2_enabled")    == 0) cfg->rle2_enabled    = (strcmp(val, "true") == 0);
            else if (strcmp(key, "huffman_enabled") == 0) cfg->huffman_enabled = (strcmp(val, "true") == 0);
            /* extra credit §8.1 / §8.3 */
            else if (strcmp(key, "rle1_enhanced")   == 0) cfg->rle1_enhanced   = (strcmp(val, "true") == 0);
            else if (strcmp(key, "rle1_threshold")  == 0) cfg->rle1_threshold  = atoi(val);
            else if (strcmp(key, "rle1_adaptive")   == 0) cfg->rle1_adaptive   = (strcmp(val, "true") == 0);
            else if (strcmp(key, "entropy_coder")   == 0) strncpy(cfg->entropy_coder, val, sizeof(cfg->entropy_coder) - 1);
        } else if (strcmp(section, "Performance") == 0) {
            if      (strcmp(key, "benchmark_mode") == 0) cfg->benchmark_mode = (strcmp(val, "true") == 0);
            else if (strcmp(key, "output_metrics") == 0) cfg->output_metrics = (strcmp(val, "true") == 0);
        } else if (strcmp(section, "Paths") == 0) {
            if      (strcmp(key, "input_directory")  == 0) strncpy(cfg->input_directory,  val, sizeof(cfg->input_directory)  - 1);
            else if (strcmp(key, "output_directory") == 0) strncpy(cfg->output_directory, val, sizeof(cfg->output_directory) - 1);
        }
    }

    fclose(f);

    /* clamp block_size to spec range 100 KB - 900 KB */
    if (cfg->block_size < 102400)  cfg->block_size = 102400;
    if (cfg->block_size > 921600)  cfg->block_size = 921600;

    /* clamp threshold to encoder's valid range */
    if (cfg->rle1_threshold < 2)   cfg->rle1_threshold = 2;
    if (cfg->rle1_threshold > 255) cfg->rle1_threshold = 255;

    return cfg;
}

void free_config(Config *cfg)
{
    free(cfg);
}

void print_config(const Config *cfg)
{
    printf("Configuration:\n");
    printf("  block_size      : %zu bytes\n",  cfg->block_size);
    printf("  rle1_enabled    : %s\n",          cfg->rle1_enabled    ? "true" : "false");
    printf("  bwt_type        : %s\n",          cfg->bwt_type);
    printf("  mtf_enabled     : %s\n",          cfg->mtf_enabled     ? "true" : "false");
    printf("  rle2_enabled    : %s\n",          cfg->rle2_enabled    ? "true" : "false");
    printf("  huffman_enabled : %s\n",          cfg->huffman_enabled ? "true" : "false");
    printf("  benchmark_mode  : %s\n",          cfg->benchmark_mode  ? "true" : "false");
    printf("  output_metrics  : %s\n",          cfg->output_metrics  ? "true" : "false");
    printf("  input_directory : %s\n",          cfg->input_directory);
    printf("  output_directory: %s\n",          cfg->output_directory);
    printf("  rle1_enhanced   : %s\n",          cfg->rle1_enhanced   ? "true" : "false");
    printf("  rle1_threshold  : %d\n",          cfg->rle1_threshold);
    printf("  rle1_adaptive   : %s\n",          cfg->rle1_adaptive   ? "true" : "false");
    printf("  entropy_coder   : %s\n",          cfg->entropy_coder);
}
