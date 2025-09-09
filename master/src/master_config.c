#include "../include/master_config.h"

t_config_master *master_leer_config(const char *path)
{
    t_config *cfg = config_create(path);
    if (!cfg)
    {
        printf("Error leyendo configuración\n");
        exit(1);
    }
    t_config_master *conf = malloc(sizeof(t_config_master));
    conf->puerto_escucha = config_get_int_value(cfg, "PUERTO_ESCUCHA");
    strcpy(conf->algoritmo_planificacion, config_get_string_value(cfg, "ALGORITMO_PLANIFICACION"));
    conf->tiempo_aging = config_get_int_value(cfg, "TIEMPO_AGING");
    strcpy(conf->log_level, config_get_string_value(cfg, "LOG_LEVEL"));
    config_destroy(cfg);
    return conf;
}