#ifndef MASTER_WORKERS_H
#define MASTER_WORKERS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <commons/log.h>

#define MAX_WORKERS 50

typedef struct
{
    int socket;
    int worker_id;
    bool ocupado;
} t_worker;

extern t_worker workers[MAX_WORKERS];
extern int cantidad_workers;

/**
 * @brief Registra un nuevo Worker en la lista global
 *
 * @param socket Socket del worker recién conectado
 * @param logger Logger para imprimir el log obligatorio
 */
void registrar_worker(int socket, t_log *logger);

/**
 * @brief Envía una query a un Worker disponible
 *
 * @param query_id Identificador de la query
 * @param path_query Ruta del archivo de la query
 * @param prioridad Prioridad de la query
 * @param logger Logger para imprimir el log obligatorio
 */
void enviar_query_worker(int query_id, const char *path_query, int prioridad, t_log *logger);

#endif