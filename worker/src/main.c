#include "../include/worker.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Uso: %s <archivo_config> <ID Worker>\n", argv[0]);
        return 1;
    }

    int worker_id = atoi(argv[2]);

    t_worker_config *cfg = leer_config_worker(argv[1]);
    logger = log_create(WORKER_LOG_PATH, WORKER_MODULE_NAME, 1, log_level_from_string(cfg->log_level));

    int sock_master = conectar_al_master(cfg, logger, worker_id);

    int tamanio_pagina = 64;
    t_memoria_interna *memoria = memoria_crear(cfg->tam_memoria, tamanio_pagina);
    log_info(logger, "Memoria interna inicializada: %d bytes, %d marcos de %d bytes",
             cfg->tam_memoria, memoria->cant_marcos, memoria->tamanio_pagina);

    int query_id, prioridad;
    char path_query[512];

    recibir_query(sock_master, &query_id, path_query, &prioridad, logger);
    consultar_storage(cfg, logger, query_id);

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "../query_control/%s", path_query);

    FILE *f = fopen(full_path, "r");
    if (!f)
    {
        log_error(logger, "Worker: no se pudo abrir el archivo de la query: %s", full_path);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    read = getline(&line, &len, f);
    while (read != -1)
    {
        if (line[read - 1] == '\n')
        {
            line[read - 1] = '\0';
        }
        query_interpretar(line, query_id, path_query, logger, memoria, cfg, sock_master);

        read = getline(&line, &len, f);
    }

    limpiar_recursos_worker(sock_master, cfg, logger, memoria, line, f);

    return 0;
}