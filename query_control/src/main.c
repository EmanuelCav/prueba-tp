#include "../include/query_control.h"

t_log* logger;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Uso: %s <archivo_config> <archivo_query> <prioridad>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *archivo_config = argv[1];
    char *archivo_query = argv[2];
    int prioridad = atoi(argv[3]);

    t_config_config *cfg = leer_config(archivo_config);
    t_log *logger = log_create(QUERY_CONTROL_LOG_PATH, QUERY_CONTROL_MODULE_NAME, 1, log_level_from_string(cfg->log_level));

    int socket_master = conectar_master(cfg->ip_master, cfg->puerto_master);
    log_info(logger, "## Conexión al Master exitosa. IP: %s, Puerto: %d", cfg->ip_master, cfg->puerto_master);

    enviar_solicitud_query(socket_master, archivo_query, prioridad, logger);
    escuchar_respuestas_master(socket_master, logger);

    limpiar_recursos(socket_master, logger, cfg);

    return 0;
}