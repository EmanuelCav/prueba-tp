#include <include/storage_fs.h>

void inicializar_fs(t_storage_config *cfg)
{
    if (cfg->fresh_start)
    {
        mkdir(cfg->punto_montaje, 0755);

        char path[512];
        snprintf(path, sizeof(path), "%s/superblock.config", cfg->punto_montaje);
        FILE *f = fopen(path, "w");
        fprintf(f, "FS_SIZE=%d\nBLOCK_SIZE=%d\n", cfg->fs_size, cfg->block_size);
        fclose(f);

        snprintf(path, sizeof(path), "%s/bitmap.bin", cfg->punto_montaje);
        int fd = open(path, O_CREAT | O_RDWR, 0644);
        int total_blocks = cfg->fs_size / cfg->block_size;
        for (int i = 0; i < total_blocks; i++)
            write(fd, "\0", 1);
        close(fd);

        snprintf(path, sizeof(path), "%s/blocks_hash_index.config", cfg->punto_montaje);
        f = fopen(path, "w");
        fclose(f);

        snprintf(path, sizeof(path), "%s/physical_blocks", cfg->punto_montaje);
        mkdir(path, 0755);
        snprintf(path, sizeof(path), "%s/files", cfg->punto_montaje);
        mkdir(path, 0755);
    }
}