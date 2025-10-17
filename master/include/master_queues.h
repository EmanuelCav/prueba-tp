#ifndef MASTER_QUEUES_H
#define MASTER_QUEUES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <commons/config.h>
#include <commons/collections/queue.h>

#include "master_config.h"

typedef struct
{
    int query_id;
    int prioridad;
    char *path_query;
    int worker_id;
    int program_counter;
    uint64_t timestamp_ready;
} t_query;

extern t_config_master *cfg;

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

/**
 * @brief Compara las prioridades de dos queries.
 */
bool comparar_prioridades(void *a, void *b);

/**
 * @brief Ordena la cola READY según las prioridades de las queries.
 */
void ordenar_ready_por_prioridad(t_queue *ready);

#endif