#ifndef MASTER_QUEUES_H
#define MASTER_QUEUES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/collections/queue.h>

typedef struct
{
    int query_id;
    int prioridad;
    char* path_query;
} t_query;

/**
 * @brief Crear una query para la queue
 *
 * @param query_id Id de la query.
 * @param prioridad Nivel de prioridad en enteros >0, siendo 0 maxima prioridad.
 * @param path_query Ruta del archivo de la query.
 */
t_query *query_create(int query_id, int prioridad, char *path_query);

/**
 * @brief Elimina una query
 */
void query_destroy(void *query);

#endif