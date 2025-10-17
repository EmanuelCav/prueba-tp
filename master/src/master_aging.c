#include "../include/master_aging.h"

void *aplicar_aging(void *arg)
{
    
    t_config_master *cfg = (t_config_master *)arg;

    while (1)
    {
        sleep(cfg->tiempo_aging);

        pthread_mutex_lock(&mutex_ready);

        if (!queue_is_empty(ready))
        {
            t_list *lista_temp = list_create();

            while (!queue_is_empty(ready))
                list_add(lista_temp, queue_pop(ready));

            for (int i = 0; i < list_size(lista_temp); i++)
            {
                t_query *q = list_get(lista_temp, i);
                time_t ahora = time(NULL);
                double espera = difftime(ahora, (time_t)q->timestamp_ready);

                if (espera > cfg->tiempo_aging && q->prioridad > 0)
                {
                    q->prioridad--;
                    q->timestamp_ready = (uint64_t)ahora;
                    log_info(logger, "## %d Cambio de prioridad: %d - %d", q->query_id, q->prioridad + 1, q->prioridad);
                }

                queue_push(ready, q);
            }

            list_destroy(lista_temp);
        }

        pthread_mutex_unlock(&mutex_ready);
    }

    return NULL;
}