#include "../include/query_control_config.h"

t_config_query *leer_config(char *path)
{

    t_config *config = config_create(path);

    if (config == NULL)
    {
        printf("Error al leer el archivo de configuración\n");
        exit(EXIT_FAILURE);
    }

    t_config_query *cfg = malloc(sizeof(t_config_query));

    cfg->ip_master = strdup(config_get_string_value(config, "IP_MASTER"));
    cfg->puerto_master = config_get_int_value(config, "PUERTO_MASTER");
    cfg->log_level = strdup(config_get_string_value(config, "LOG_LEVEL"));

    config_destroy(config);

    return cfg;
}

void limpiar_recursos_query(int socket_master, t_log *logger, t_config_query *cfg)
{
    close(socket_master);
    log_destroy(logger);
    free(cfg->ip_master);
    free(cfg->log_level);
    free(cfg);
}