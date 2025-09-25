#ifndef MASTER_WORKERS_H
#define MASTER_WORKERS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <commons/log.h>
#include "../include/master_queues.h"

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
 * @param worker_id id del worker
 */
void registrar_worker(int socket, t_log *logger, int worker_id)

/**
 * @brief Envía una query a un Worker disponible, busca en la cola de ready
 * 
 * @param ready Cola de querys en ready
 * @param exec Cola de querys en executing
 * @param logger Logger para imprimir el log obligatorio
 */
void enviar_query_worker(t_queue* ready,t_list* exec, t_log *logger);

#endif