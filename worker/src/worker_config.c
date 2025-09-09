#include "../include/worker_config.h"

t_worker_config *leer_config_worker(char *path_config)
{
    t_config *cfg = config_create(path_config);

    if (!cfg)
    {
        printf("Error leyendo archivo de configuración\n");
        exit(EXIT_FAILURE);
    }

    t_worker_config *wcfg = malloc(sizeof(t_worker_config));

    strcpy(wcfg->ip_master, config_get_string_value(cfg, "IP_MASTER"));
    wcfg->puerto_master = config_get_int_value(cfg, "PUERTO_MASTER");
    strcpy(wcfg->ip_storage, config_get_string_value(cfg, "IP_STORAGE"));
    wcfg->puerto_storage = config_get_int_value(cfg, "PUERTO_STORAGE");
    wcfg->tam_memoria = config_get_int_value(cfg, "TAM_MEMORIA");
    wcfg->retardo_memoria = config_get_int_value(cfg, "RETARDO_MEMORIA");
    strcpy(wcfg->algoritmo_reemplazo, config_get_string_value(cfg, "ALGORITMO_REEMPLAZO"));
    strcpy(wcfg->path_queries, config_get_string_value(cfg, "PATH_SCRIPTS"));
    strcpy(wcfg->log_level, config_get_string_value(cfg, "LOG_LEVEL"));

    config_destroy(cfg);

    return wcfg;
}

void limpiar_recursos_worker(int sock_master, t_worker_config *cfg, t_log *logger)
{
    close(sock_master);
    free(cfg);
    log_destroy(logger);
}