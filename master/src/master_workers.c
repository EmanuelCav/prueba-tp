#include "../include/master_workers.h"

t_worker workers[MAX_WORKERS];
int cantidad_workers = 0;

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

void enviar_query_worker(t_queue* ready,t_list *exec, t_log *logger)
{
    for (int i = 0; i < cantidad_workers; i++)
    {
        if (!workers[i].ocupado && !queue_is_empty(ready))
        {
            t_query* query_pop = queue_pop(ready);
            char mensaje[512];
            sprintf(mensaje, "%d|%s|%d", query_pop->query_id, query_pop->path_query, query_pop->prioridad);
            send(workers[i].socket, mensaje, strlen(mensaje), 0);
            workers[i].ocupado = true;
            list_add(exec, query_pop);

            log_info(logger, "## Se envía la Query %d al Worker %d", query_pop->query_id, workers[i].worker_id);
            return;
        }
    }
     if (queue_is_empty(ready)) {
        printf("No hay querys en espera\n");
    } else {
        printf("No hay Workers disponibles\n");
    }
}