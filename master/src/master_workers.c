#include "../include/master_workers.h"

t_worker workers[MAX_WORKERS];
int cantidad_workers = 0;
t_log *logger;
t_queue *ready;
t_list *exec;

void registrar_worker(int socket, t_log *logger, int worker_id)
{
    if (cantidad_workers >= MAX_WORKERS)
        return;

    workers[cantidad_workers].socket = socket;
    workers[cantidad_workers].worker_id = worker_id;
    workers[cantidad_workers].ocupado = false;
    cantidad_workers++;

    log_info(logger, "## Se conecta el Worker %d - Cantidad total de Workers: %d",
             worker_id, cantidad_workers);
}

void enviar_query_worker(t_queue *ready, t_list *exec, t_log *logger)
{
    if (queue_is_empty(ready))
        return;

    if (strcmp(cfg->algoritmo_planificacion, "PRIORIDADES") == 0)
        ordenar_ready_por_prioridad(ready);

    for (int i = 0; i < cantidad_workers; i++)
    {
        if (!workers[i].ocupado && !queue_is_empty(ready))
        {
            t_query *query_pop = queue_pop(ready);
            query_pop->worker_id = workers[i].worker_id;
            char mensaje[512];
            sprintf(mensaje, "%d|%s|%d|%d", query_pop->query_id, query_pop->path_query, query_pop->prioridad, query_pop->program_counter);
            send(workers[i].socket, mensaje, strlen(mensaje), 0);
            workers[i].ocupado = true;
            list_add(exec, query_pop);
            log_info(logger, "## Se envía la Query %d (%d) al Worker %d",
                     query_pop->query_id, query_pop->prioridad, workers[i].worker_id);
            return;
        }
    }

    if (strcmp(cfg->algoritmo_planificacion, "PRIORIDADES") == 0)
    {
        t_query *query_ready = queue_peek(ready);
        t_query *query_baja = NULL;

        for (int i = 0; i < list_size(exec); i++)
        {
            t_query *q = list_get(exec, i);
            if (!query_baja || q->prioridad > query_baja->prioridad)
            {
                query_baja = q;
            }
        }

        if (query_baja && query_ready->prioridad < query_baja->prioridad)
        {
            for (int i = 0; i < cantidad_workers; i++)
            {
                if (workers[i].worker_id == query_baja->worker_id)
                {
                    char msg[64];
                    sprintf(msg, "DESALOJAR|%d", query_baja->query_id);
                    send(workers[i].socket, msg, strlen(msg), 0);

                    log_info(logger, "## Se desaloja la Query %d (%d) del Worker %d - Motivo: PRIORIDAD",
                             query_baja->query_id, query_baja->prioridad, workers[i].worker_id);

                    return;
                }
            }
        }
    }
    

    log_info(logger, "## No hay Workers libres ni Querys con menor prioridad para desalojar");
}

void limpiar_recursos_master(int listener, t_config_master *cfg, t_log *logger)
{
    close(listener);
    queue_destroy_and_destroy_elements(ready, query_destroy);
    list_destroy_and_destroy_elements(exec, query_destroy);
    list_destroy_and_destroy_elements(query_controls, free);
    log_destroy(logger);
    free(cfg);
}