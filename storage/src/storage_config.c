#include <include/storage_config.h>

t_storage_config *leer_config_storage(const char *path)
{
    t_config *cfg = config_create(path);
    if (!cfg)
    {
        printf("Error leyendo archivo config\n");
        exit(1);
    }

    t_storage_config *scfg = malloc(sizeof(t_storage_config));
    scfg->puerto_escucha = config_get_int_value(cfg, "PUERTO_ESCUCHA");
    scfg->fresh_start = config_get_boolean_value(cfg, "FRESH_START");
    strcpy(scfg->punto_montaje, config_get_string_value(cfg, "PUNTO_MONTAJE"));
    scfg->retardo_operacion = config_get_int_value(cfg, "RETARDO_OPERACION");
    scfg->retardo_acceso_bloque = config_get_int_value(cfg, "RETARDO_ACCESO_BLOQUE");
    strcpy(scfg->log_level, config_get_string_value(cfg, "LOG_LEVEL"));
    scfg->fs_size = 4096;
    scfg->block_size = 128;
    config_destroy(cfg);
    return scfg;
}