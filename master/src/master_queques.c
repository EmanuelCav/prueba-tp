#include "../include/master_queues.h"

t_query *query_create(int query_id, int prioridad, char *path_query)
{
    t_query* query_ret = malloc(sizeof(t_query));
    query_ret->query_id = query_id;
    query_ret->prioridad = prioridad;
    query_ret->path_query = strdup(path_query);
    
    return query_ret;
}

void query_destroy(void *arg){
t_query *query= (t_query*) arg;
free(arg);
free(query->path_query);
free(query);
}