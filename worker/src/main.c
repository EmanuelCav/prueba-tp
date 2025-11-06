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

    int tamanio_pagina = consultar_storage(cfg, logger, worker_id);
    t_memoria_interna *memoria = memoria_crear(cfg->tam_memoria, tamanio_pagina);

    log_info(logger, "Memoria interna inicializada: %d bytes, %d marcos de %d bytes",
             cfg->tam_memoria, memoria->cant_marcos, memoria->tamanio_pagina);

    int sock_master = conectar_al_master(cfg, logger, worker_id);

    int query_id, prioridad, program_counter = 0;
    char path_query[512];

    recibir_query(sock_master, &query_id, path_query, &prioridad, &program_counter, logger);

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", cfg->path_queries, path_query);

    FILE *f = fopen(full_path, "r");
    if (!f)
    {
        log_error(logger, "Worker: no se pudo abrir el archivo de la query: %s", full_path);
        return 1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    t_list *archivos_modificados = list_create();

    for (int i = 0; i < program_counter && getline(&line, &len, f) != -1; i++)
    {
    }
    log_info(logger, "## Query %d: Reanudando ejecución desde línea %d", query_id, program_counter);

    while ((read = getline(&line, &len, f)) != -1)
    {
        if (line[read - 1] == '\n')
            line[read - 1] = '\0';

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock_master, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        int activity = select(sock_master + 1, &readfds, NULL, NULL, &timeout);

        if (activity > 0 && FD_ISSET(sock_master, &readfds))
        {
            char buffer_msg[64] = {0};
            recv(sock_master, buffer_msg, sizeof(buffer_msg) - 1, 0);

            if (strncmp(buffer_msg, "DESALOJAR", 9) == 0)
            {
                int id_desalojo = atoi(buffer_msg + 10);
                if (id_desalojo == query_id)
                {
                    log_info(logger, "## Query %d: Desalojada por pedido del Master", query_id);

                    char respuesta[64];
                    sprintf(respuesta, "DESALOJO_OK|%d|%d", query_id, program_counter);
                    send(sock_master, respuesta, strlen(respuesta), 0);

                    limpiar_recursos_worker(sock_master, cfg, logger, memoria, line, f);
                    return 0;
                }
            }
        }

        log_info(logger, "## Query %d: FETCH - Program Counter: %d - %s",
                 query_id, program_counter, line);

        query_interpretar(line, query_id, path_query, logger, memoria, cfg, sock_master, archivos_modificados, worker_id);

        program_counter++;

        char msg_pc[64];
        sprintf(msg_pc, "PC_UPDATE|%d|%d", query_id, program_counter);
        enviar_comando_master(sock_master, logger, query_id, msg_pc, "");
    }

    limpiar_recursos_worker(sock_master, cfg, logger, memoria, line, f);
    return 0;
}