#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bzip2.h"

BlockManager *divide_into_blocks(const char *filename, size_t block_size)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("divide_into_blocks: cannot open file");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size <= 0) {
        fprintf(stderr, "divide_into_blocks: file is empty or unreadable\n");
        fclose(f);
        return NULL;
    }

    int num_blocks = (int)(((long)file_size + (long)block_size - 1) / (long)block_size);

    BlockManager *mgr = (BlockManager *)malloc(sizeof(BlockManager));
    if (!mgr) { fclose(f); return NULL; }

    mgr->blocks     = (Block *)malloc((size_t)num_blocks * sizeof(Block));
    mgr->num_blocks = num_blocks;
    mgr->block_size = block_size;

    if (!mgr->blocks) {
        free(mgr);
        fclose(f);
        return NULL;
    }

    for (int i = 0; i < num_blocks; i++) {
        long remaining = file_size - (long)i * (long)block_size;
        size_t to_read = (remaining < (long)block_size) ? (size_t)remaining : block_size;

        mgr->blocks[i].data = (unsigned char *)malloc(to_read);
        if (!mgr->blocks[i].data) {
            fprintf(stderr, "divide_into_blocks: malloc failed for block %d\n", i);
            /* free already allocated blocks */
            for (int j = 0; j < i; j++) free(mgr->blocks[j].data);
            free(mgr->blocks);
            free(mgr);
            fclose(f);
            return NULL;
        }

        mgr->blocks[i].size          = fread(mgr->blocks[i].data, 1, to_read, f);
        mgr->blocks[i].original_size = mgr->blocks[i].size;
    }

    fclose(f);
    return mgr;
}

int reassemble_blocks(BlockManager *manager, const char *output_filename)
{
    if (!manager) return -1;

    FILE *f = fopen(output_filename, "wb");
    if (!f) {
        perror("reassemble_blocks: cannot open output file");
        return -1;
    }

    for (int i = 0; i < manager->num_blocks; i++) {
        size_t written = fwrite(manager->blocks[i].data, 1,
                                manager->blocks[i].size, f);
        if (written != manager->blocks[i].size) {
            perror("reassemble_blocks: write error");
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}

void free_block_manager(BlockManager *manager)
{
    if (!manager) return;
    for (int i = 0; i < manager->num_blocks; i++)
        free(manager->blocks[i].data);
    free(manager->blocks);
    free(manager);
}
