#ifndef STORAGE_WORKER_H
#define STORAGE_WORKER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <commons/log.h>
#include <pthread.h>

#define MAX_BUFFER 1024

typedef enum
{
    CMD_UNKNOWN,
    CMD_GET_BLOCK_SIZE,
    CMD_GET_FS_SIZE,
    CMD_CREATE,
    CMD_TRUNCATE,
    CMD_WRITE,
    CMD_READ,
    CMD_TAG,
    CMD_COMMIT,
    CMD_DELETE,
    CMD_END
} comando_t;

static int cantidad_workers = 0;
pthread_mutex_t mutex_workers = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Función que maneja la comunicación con un Worker conectado al Master.
 *
 * @param arg Puntero a un entero que representa el socket del Worker.
 *            Debe ser liberado dentro de la función.
 * @return void* Siempre retorna NULL al finalizar.
 */
void *manejar_worker(void *arg);

/**
 * @brief Convierte un comando recibido como string a un valor enumerado.
 *
 * @param cmd Puntero a un string que contiene el comando recibido.
 * @return comando_t Valor enumerado correspondiente al comando
 */
comando_t parse_comando(const char *cmd);

#endif
