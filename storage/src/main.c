#include <include/storage.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Uso: %s <archivo_config>\n", argv[0]);
        return 1;
    }

    t_storage_config *cfg = leer_config_storage(argv[1]);
    logger = log_create(STORAGE_LOG_PATH, STORAGE_MODULE_NAME, 1, log_level_from_string(cfg->log_level));

    inicializar_fs(cfg);

    int listener = iniciar_servidor(cfg->puerto_escucha);
    log_info(logger, "## Storage escuchando en puerto %d", cfg->puerto_escucha);

    while (1)
    {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(listener, (struct sockaddr *)&addr, &addrlen);
        if (*client_sock < 0)
        {
            perror("accept");
            free(client_sock);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, manejar_worker, client_sock);
        pthread_detach(tid);
    }

    log_destroy(logger);
    free(cfg);
    return 0;
}