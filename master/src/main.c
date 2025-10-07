#include "../include/master.h"

t_worker workers[MAX_WORKERS];
int cantidad_workers = 0;
t_log *logger;
t_queue *ready;
t_list *exec;
t_list *query_controls;

pthread_mutex_t mutex_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_exec = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_query_controls = PTHREAD_MUTEX_INITIALIZER;

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
    log_info(logger, "## Master escuchando en puerto %d con algoritmo FIFO", cfg->puerto_escucha);

    ready = queue_create();
    exec = list_create();
    query_controls = list_create();

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

    return 0;
}