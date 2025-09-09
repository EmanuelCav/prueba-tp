#include "../include/worker.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Uso: %s <archivo_config>\n", argv[0]);
        return 1;
    }

    t_worker_config *cfg = leer_config_worker(argv[1]);
    logger = log_create(WORKER_LOG_PATH, WORKER_MODULE_NAME, 1, log_level_from_string(cfg->log_level));

    int sock_master = conectar_al_master(cfg, logger);

    int query_id, prioridad;
    char path_query[512];

    recibir_query(sock_master, &query_id, path_query, &prioridad, logger);
    consultar_storage(cfg, logger, query_id);

    limpiar_recursos_worker(sock_master, cfg, logger);

    return 0;
}