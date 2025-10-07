#ifndef MASTER_CONEXIONES_H
#define MASTER_CONEXIONES_H

#include "master_workers.h"

#define MAX_BUFFER 1024

/**
 * @brief Estructura que representa un Query Control activo conectado al Master.
 */
typedef struct
{
    int socket;
    int query_id;
    int prioridad;
    char path[512];
    bool activo;
} t_query_control_activo;

/**
 * @brief Maneja la desconexión de un Worker.
 *
 * @param socket Socket del Worker que se desconectó.
 */
void desconectar_worker(int socket);

/**
 * @brief Maneja la desconexión de un Query Control.
 *
 * @param qc Puntero a la estructura del Query Control que se desconecta.
 */
void desconectar_query_control(t_query_control_activo *qc);

/**
 * @brief Atiende una conexión entrante (Worker o Query Control).
 *
 * @param arg Puntero al socket del cliente conectado.
 * @return void* No retorna valor útil (usado por pthread_create).
 */
void *atender_conexion(void *arg);

#endif