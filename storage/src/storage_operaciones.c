#include "../include/storage_operaciones.h"

void crear_file_tag(const char *nombre_file, const char *tag, t_log *logger) {
    
    char path_file[256], path_tag[256], path_logical[256], path_meta[256];

    sprintf(path_file, "./files/%s", nombre_file);
    sprintf(path_tag, "%s/%s", path_file, tag);
    sprintf(path_logical, "%s/logical_blocks", path_tag);
    sprintf(path_meta, "%s/metadata.config", path_tag);

    mkdir(path_file, 0777);
    mkdir(path_tag, 0777);
    mkdir(path_logical, 0777);

    FILE *meta = fopen(path_meta, "w");
    fprintf(meta, "TAMAÑO=0\nBLOCKS=[]\nESTADO=WORK_IN_PROGRESS\n");
    fclose(meta);

    log_info(logger, "## File Creado %s:%s", nombre_file, tag);
}

void truncar_file_tag(const char *nombre_file, const char *tag, int nuevo_tam, t_storage_config *cfg, t_log *logger) {
    char path_meta[256];
    sprintf(path_meta, "./files/%s/%s/metadata.config", nombre_file, tag);

    t_config *meta = config_create(path_meta);
    int tam_actual = config_get_int_value(meta, "TAMAÑO");
    int block_size = cfg->block_size;
    int bloques_actuales = tam_actual / block_size;
    int bloques_nuevos = nuevo_tam / block_size;

    for (int i = bloques_actuales; i < bloques_nuevos; i++) {
        char path_fisico[256], path_logico[256];
        sprintf(path_fisico, "./physical_blocks/block%04d.dat", i);
        sprintf(path_logico, "./files/%s/%s/logical_blocks/%06d.dat", nombre_file, tag, i);
        link(path_fisico, path_logico);
    }

    FILE *meta_file = fopen(path_meta, "w");
    fprintf(meta_file, "TAMAÑO=%d\n", nuevo_tam);
    fprintf(meta_file, "BLOCKS=[");
    for (int i = 0; i < bloques_nuevos; i++)
        fprintf(meta_file, "%d%s", i, i < bloques_nuevos - 1 ? "," : "");
    fprintf(meta_file, "]\nESTADO=WORK_IN_PROGRESS\n");
    fclose(meta_file);

    config_destroy(meta);
    log_info(logger, "## File Truncado %s:%s - Tamaño: %d", nombre_file, tag, nuevo_tam);
}

char *leer_bloque(const char *nombre_file, const char *tag, int bloque, t_storage_config *cfg, t_log *logger) {
    char path_logico[256];
    sprintf(path_logico, "./files/%s/%s/logical_blocks/%06d.dat", nombre_file, tag, bloque);

    FILE *f = fopen(path_logico, "r");
    if (!f) return NULL;

    char *buffer = malloc(cfg->block_size + 1);
    fread(buffer, 1, cfg->block_size, f);
    buffer[cfg->block_size] = '\0';
    fclose(f);

    log_info(logger, "## Bloque Lógico Leído %s:%s - Número de Bloque: %d", nombre_file, tag, bloque);
    return buffer;
}

void escribir_bloque(const char *nombre_file, const char *tag, int bloque, const char *contenido, t_storage_config *cfg, t_log *logger) {
    char path_logico[256];
    sprintf(path_logico, "./files/%s/%s/logical_blocks/%06d.dat", nombre_file, tag, bloque);

    FILE *f = fopen(path_logico, "w");
    fwrite(contenido, 1, strlen(contenido), f);
    fclose(f);

    log_info(logger, "## Bloque Lógico Escrito %s:%s - Número de Bloque: %d", nombre_file, tag, bloque);
}