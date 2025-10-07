// master_workers.h
#ifndef MASTER_WORKERS_H
#define MASTER_WORKERS_H

#include <pthread.h>
#include <commons/log.h>
#include "master_queues.h"

#define MAX_WORKERS 50

typedef struct {
    int socket;
    int worker_id;
    bool ocupado;
} t_worker;

extern t_worker workers[MAX_WORKERS];
extern int cantidad_workers;
extern t_log *logger;
extern t_queue *ready;
extern t_list *exec;
extern t_list *query_controls;

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