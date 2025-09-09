#ifndef STORAGE_WORKER_H
#define STORAGE_WORKER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <commons/log.h>

#define MAX_BUFFER 1024

/**
 * @brief Función que maneja la comunicación con un Worker conectado al Master.
 *
 * @param arg Puntero a un entero que representa el socket del Worker.
 *            Debe ser liberado dentro de la función.
 * @return void* Siempre retorna NULL al finalizar.
 */
void *manejar_worker(void *arg);

extern t_log *logger;

#endif
