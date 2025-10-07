#ifndef MASTER_WORKERS_H
#define MASTER_WORKERS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <commons/log.h>
#include "../include/master_queues.h"

#define MAX_WORKERS 50
#define MAX_BUFFER 1024

typedef struct
{
    int socket;
    int worker_id;
    bool ocupado;
} t_worker;

extern t_worker workers[MAX_WORKERS];
extern int cantidad_workers;
extern t_log *logger;
extern t_queue *ready;
extern t_list *exec;

extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_exec;
extern pthread_mutex_t mutex_query_controls;

/**
 * @brief Registra un nuevo Worker en la lista global
 */
void registrar_worker(int socket, t_log *logger, int worker_id);

/**
 * @brief Envía una query a un Worker disponible
 */
void enviar_query_worker(t_queue *ready, t_list *exec, t_log *logger);

#endif