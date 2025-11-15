#include "../include/storage_config.h"

t_storage_config *leer_config_storage(const char *path)
{
    t_config *cfg = config_create((char *)path);

    if (!cfg)
    {
        printf("Error leyendo archivo config\n");
        exit(1);
    }

    t_storage_config *scfg = malloc(sizeof(t_storage_config));
    scfg->puerto_escucha = config_get_int_value(cfg, "PUERTO_ESCUCHA");
    scfg->fresh_start = config_get_int_value(cfg, "FRESH_START");
    strcpy(scfg->punto_montaje, config_get_string_value(cfg, "PUNTO_MONTAJE"));
    scfg->fs_size = config_get_int_value(cfg, "FS_SIZE");
    scfg->block_size = config_get_int_value(cfg, "BLOCK_SIZE");
    scfg->retardo_operacion = config_get_int_value(cfg, "RETARDO_OPERACION");
    scfg->retardo_acceso_bloque = config_get_int_value(cfg, "RETARDO_ACCESO_BLOQUE");
    strcpy(scfg->log_level, config_get_string_value(cfg, "LOG_LEVEL"));
    config_destroy(cfg);
    return scfg;
}

void limpiar_recursos_storage(t_storage_config *cfg, t_log *logger)
{
    log_destroy(logger);
    free(cfg);
}