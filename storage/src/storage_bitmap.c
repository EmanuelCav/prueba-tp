#include "../include/storage_bitmap.h"

int buscar_bloque_libre(t_storage_config *cfg, t_log *logger)
{
    const char *path_bitmap = "./bitmap.bin";

    int bitmap_fd = open(path_bitmap, O_RDWR);
    if (bitmap_fd < 0)
    {
        log_error(logger, "No se pudo abrir el bitmap: %s", strerror(errno));
        return -1;
    }

    int bloques = cfg->fs_size / cfg->block_size;
    size_t bytes = (bloques + 7) / 8;

    void *bitmap_map = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_fd, 0);
    if (bitmap_map == MAP_FAILED)
    {
        log_error(logger, "mmap fallo: %s", strerror(errno));
        close(bitmap_fd);
        return -1;
    }

    for (int i = 0; i < bloques; i++)
    {
        int byte = i / 8;
        int bit = i % 8;
        unsigned char mask = (1u << bit);
        if (!(((unsigned char *)bitmap_map)[byte] & mask))
        {
            ((unsigned char *)bitmap_map)[byte] |= mask;
            if (msync(bitmap_map, bytes, MS_SYNC) != 0)
                log_warning(logger, "msync fallo: %s", strerror(errno));
            munmap(bitmap_map, bytes);
            close(bitmap_fd);
            log_info(logger, "##<QUERY_ID> - Bloque Físico Reservado - Número de Bloque: %d", i);
            return i;
        }
    }

    munmap(bitmap_map, bytes);
    close(bitmap_fd);
    log_error(logger, "No hay bloques libres disponibles");
    return -1;
}

void liberar_bloque(int bloque, t_storage_config *cfg, t_log *logger)
{
    const char *path_bitmap = "./bitmap.bin";

    int bitmap_fd = open(path_bitmap, O_RDWR);
    if (bitmap_fd < 0)
    {
        log_error(logger, "No se pudo abrir el bitmap para liberar: %s", strerror(errno));
        return;
    }

    int bloques = cfg->fs_size / cfg->block_size;
    size_t bytes = (bloques + 7) / 8;
    void *bitmap_map = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_fd, 0);
    if (bitmap_map == MAP_FAILED)
    {
        log_error(logger, "mmap fallo: %s", strerror(errno));
        close(bitmap_fd);
        return;
    }

    int byte = bloque / 8;
    int bit = bloque % 8;
    ((unsigned char *)bitmap_map)[byte] &= ~(1u << bit);

    if (msync(bitmap_map, bytes, MS_SYNC) != 0)
        log_warning(logger, "msync fallo: %s", strerror(errno));

    munmap(bitmap_map, bytes);
    close(bitmap_fd);
    log_info(logger, "##<QUERY_ID> - Bloque Físico Liberado - Número de Bloque: %d", bloque);
}