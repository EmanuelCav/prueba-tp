#include <include/master_workers.h>

void registrar_worker(int socket, t_log *logger)
{
    if (cantidad_workers >= MAX_WORKERS)
        return;
    int worker_id = cantidad_workers;
    workers[cantidad_workers].socket = socket;
    workers[cantidad_workers].worker_id = worker_id;
    workers[cantidad_workers].ocupado = false;
    cantidad_workers++;

    log_info(logger, "## Se conecta el Worker %d - Cantidad total de Workers: %d",
             worker_id, cantidad_workers);
}

void enviar_query_worker(int query_id, const char *path_query, int prioridad, t_log *logger)
{
    for (int i = 0; i < cantidad_workers; i++)
    {
        if (!workers[i].ocupado)
        {
            char mensaje[512];
            sprintf(mensaje, "%d|%s|%d", query_id, path_query, prioridad);
            send(workers[i].socket, mensaje, strlen(mensaje), 0);
            workers[i].ocupado = true;

            log_info(logger, "## Se envía la Query %d al Worker %d", query_id, workers[i].worker_id);
            return;
        }
    }
    printf("No hay Workers disponibles\n");
}