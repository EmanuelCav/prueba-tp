#include "../include/master.h"

int main(int argc, char *argv[])

{
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <archivo_config>\n", argv[0]);
        return 1;
    }

    t_config_master *cfg = master_leer_config(argv[1]);
    logger = log_create(MASTER_LOG_PATH, MASTER_MODULE_NAME, 1, log_level_from_string(cfg->log_level));

    char puerto_str[6];
    sprintf(puerto_str, "%d", cfg->puerto_escucha);
    int listener = iniciar_servidor(puerto_str);
    log_info(logger, "## Master escuchando en puerto %d con algoritmo %s (Aging=%d ms)",
             cfg->puerto_escucha, cfg->algoritmo_planificacion, cfg->tiempo_aging);

    int query_id_counter = 0;

    while (1)
    {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        int newfd = accept(listener, (struct sockaddr *)&addr, &addrlen);
        if (newfd < 0)
        {
            perror("accept");
            continue;
        }

        char buffer[MAX_BUFFER];
        int bytes = recv(newfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0)
        {
            close(newfd);
            continue;
        }
        buffer[bytes] = '\0';

        if (strcmp(buffer, "WORKER") == 0)
        {
            registrar_worker(newfd);
        }
        else
        {
            char path_query[512];
            int prioridad;
            sscanf(buffer, "%[^|]|%d", path_query, &prioridad);
            int query_id = query_id_counter++;

            log_info(logger,
                     "## Se conecta un Query Control para ejecutar la Query %s con prioridad %d - Id asignado: %d. Nivel multiprocesamiento %d",
                     path_query, prioridad, query_id, cantidad_workers);

            enviar_query_worker(query_id, path_query, prioridad);
        }
    }

    log_destroy(logger);
    free(cfg);
    return 0;
}