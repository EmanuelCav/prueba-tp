#include "../include/storage_fs.h"
#include <sys/mman.h>
#include <fcntl.h>

void inicializar_fs(t_storage_config *cfg, t_log *logger)
{
    if (cfg->fresh_start)
    {
        log_info(logger, "Inicializando FS desde cero (FRESH_START)");

        crear_estructura_base("./", logger);
        crear_superblock("./", cfg->fs_size, cfg->block_size, logger);
        crear_bitmap("./", cfg->fs_size, cfg->block_size, logger);
        crear_archivo_inicial("./", cfg->block_size, logger);

        log_info(logger, "FS inicializado correctamente en %s", "./");
    }
    else
    {
        log_info(logger, "Cargando FS existente desde %s", "./");
    }
}

void crear_estructura_base(const char *root_path, t_log *logger)
{
    char path_metadata[1024], path_files[1024];
    snprintf(path_metadata, sizeof(path_metadata), "%s/physical_blocks", root_path);
    snprintf(path_files, sizeof(path_files), "%s/files", root_path);
    mkdir(path_metadata, 0777);
    mkdir(path_files, 0777);
    log_info(logger, "Directorios creados: %s, %s", path_metadata, path_files);

    char path_hash[1024];
    snprintf(path_hash, sizeof(path_hash), "%s/blocks_hash_index.config", root_path);
    FILE *f = fopen(path_hash, "w");
    if (f)
    {
        fclose(f);
        log_info(logger, "Archivo creado: %s", path_hash);
    }
}

void crear_superblock(const char *root_path, int fs_size, int block_size, t_log *logger)
{
    char path_superblock[1024];
    snprintf(path_superblock, sizeof(path_superblock), "%s/superblock.config", root_path);
    FILE *f = fopen(path_superblock, "w");
    if (!f)
        return;
    fprintf(f, "FS_SIZE=%d\nBLOCK_SIZE=%d\n", fs_size, block_size);
    fclose(f);
    log_info(logger, "Archivo superblock creado: %s", path_superblock);
}

void crear_bitmap(const char *root_path, int fs_size, int block_size, t_log *logger)
{
    char path_bitmap[1024];
    snprintf(path_bitmap, sizeof(path_bitmap), "%s/bitmap.bin", root_path);
    int bitmap_fd = open(path_bitmap, O_CREAT | O_RDWR, 0666);
    int bloques = fs_size / block_size;
    ftruncate(bitmap_fd, bloques / 8);

    void *bitmap_map = mmap(NULL, bloques / 8, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_fd, 0);
    memset(bitmap_map, 0, bloques / 8);
    msync(bitmap_map, bloques / 8, MS_SYNC);
    munmap(bitmap_map, bloques / 8);
    close(bitmap_fd);

    log_info(logger, "Archivo bitmap creado: %s con %d bloques", path_bitmap, bloques);
}

void crear_archivo_inicial(const char *root_path, int block_size, t_log *logger)
{
    char path_file[4096], path_tag[4096], path_logical[4096];

    int len = snprintf(path_file, sizeof(path_file), "%s/files/initial_file", root_path);
    if (len >= sizeof(path_file))
    {
        log_error(logger, "Ruta demasiado larga: %s/files/initial_file", root_path);
        exit(EXIT_FAILURE);
    }

    len = snprintf(path_tag, sizeof(path_tag), "%s/BASE", path_file);
    if (len >= sizeof(path_tag))
    {
        log_error(logger, "Ruta demasiado larga: %s/BASE", path_file);
        exit(EXIT_FAILURE);
    }

    len = snprintf(path_logical, sizeof(path_logical), "%s/logical_blocks", path_tag);
    if (len >= sizeof(path_logical))
    {
        log_error(logger, "Ruta demasiado larga: %s/logical_blocks", path_tag);
        exit(EXIT_FAILURE);
    }

    mkdir(path_file, 0777);
    mkdir(path_tag, 0777);
    mkdir(path_logical, 0777);
    log_info(logger, "Directorios creados: %s, %s, %s", path_file, path_tag, path_logical);

    char path_metadata_config[4096];
    len = snprintf(path_metadata_config, sizeof(path_metadata_config), "%s/metadata.config", path_tag);
    if (len >= sizeof(path_metadata_config))
    {
        log_error(logger, "Ruta demasiado larga: %s/metadata.config", path_tag);
        exit(EXIT_FAILURE);
    }

    FILE *meta = fopen(path_metadata_config, "w");
    if (meta)
    {
        fprintf(meta, "TAMAÑO=%d\n", block_size);
        fprintf(meta, "BLOCKS=[0]\n");
        fprintf(meta, "ESTADO=COMMITED\n");
        fclose(meta);
        log_info(logger, "Archivo metadata creado: %s", path_metadata_config);
    }

    char path_block0[4096];
    len = snprintf(path_block0, sizeof(path_block0), "%s/physical_blocks/block0000.dat", root_path);
    if (len >= sizeof(path_block0))
    {
        log_error(logger, "Ruta demasiado larga: %s/physical_blocks/block0000.dat", root_path);
        exit(EXIT_FAILURE);
    }

    FILE *blk = fopen(path_block0, "w");
    if (blk)
    {
        for (int i = 0; i < block_size; i++)
            fputc('0', blk);
        fclose(blk);
        log_info(logger, "Bloque físico creado: %s", path_block0);
    }

    char path_logical_block0[4096];
    len = snprintf(path_logical_block0, sizeof(path_logical_block0), "%s/logical_blocks/block0000.dat", path_tag);
    if (len >= sizeof(path_logical_block0))
    {
        log_error(logger, "Ruta demasiado larga: %s/logical_blocks/block0000.dat", path_tag);
        exit(EXIT_FAILURE);
    }

    link(path_block0, path_logical_block0);
    log_info(logger, "Bloque lógico enlazado: %s -> %s", path_logical_block0, path_block0);
}