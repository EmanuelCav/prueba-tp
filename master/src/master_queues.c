#include "../include/master_queues.h"

t_query *query_create(int query_id, int prioridad, char *path_query)
{
    t_query *query_ret = malloc(sizeof(t_query));
    query_ret->query_id = query_id;
    query_ret->prioridad = prioridad;
    query_ret->path_query = strdup(path_query);
    query_ret->worker_id = -1;
    query_ret->program_counter = 0;
    query_ret->timestamp_ready = (uint64_t) time(NULL);
    return query_ret;
}

void query_destroy(void *arg)
{
    t_query *query = (t_query *)arg;
    free(query->path_query);
    free(query);
}

int comparar_prioridades(void *a, void *b)
{
    t_query *q1 = (t_query *)a;
    t_query *q2 = (t_query *)b;
    return q1->prioridad - q2->prioridad;
}

void ordenar_ready_por_prioridad(t_queue *ready)
{
    if (queue_is_empty(ready)) return;
    t_list *lista_temp = list_create();
    while (!queue_is_empty(ready))
        list_add(lista_temp, queue_pop(ready));
    list_sort(lista_temp, comparar_prioridades);
    for (int i = 0; i < list_size(lista_temp); i++)
        queue_push(ready, list_get(lista_temp, i));
    list_destroy(lista_temp);
}
