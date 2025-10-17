#include "../include/master.h" +

t_config_master *cfg;

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <archivo_config>\n", argv[0]);
        return 1;
    }

    cfg = master_leer_config(argv[1]);
    logger = log_create(MASTER_LOG_PATH, MASTER_MODULE_NAME, 1, log_level_from_string(cfg->log_level));

    char puerto_str[6];
    sprintf(puerto_str, "%d", cfg->puerto_escucha);
    int listener = iniciar_servidor(puerto_str);
    log_info(logger, "## Master escuchando en puerto %d con algoritmo %s",
             cfg->puerto_escucha, cfg->algoritmo_planificacion);

    ready = queue_create();
    exec = list_create();
    query_controls = list_create();

    if (strcmp(cfg->algoritmo_planificacion, "PRIORIDADES") == 0 && cfg->tiempo_aging > 0)
    {
        pthread_t hilo_aging;
        pthread_create(&hilo_aging, NULL, aplicar_aging, cfg);
        pthread_detach(hilo_aging);
        log_info(logger, "## Aging habilitado (cada %d segundos)", cfg->tiempo_aging * 1000);
    }

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

        int *socket_cliente = malloc(sizeof(int));
        *socket_cliente = newfd;
        pthread_t hilo;
        pthread_create(&hilo, NULL, atender_conexion, socket_cliente);
        pthread_detach(hilo);
    }

    limpiar_recursos_master(listener, cfg, logger);

    return 0;
}