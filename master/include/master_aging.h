#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <commons/collections/queue.h>
#include <commons/log.h>

#include "master_config.h"
#include "master_queues.h"
#include "master_workers.h"
#include "master_conexiones.h"

/**
 * @brief Hilo encargado de aplicar aging sobre las queries en READY.
 *
 * @param arg Puntero a una estructura de configuración del master.
 */
void *aplicar_aging(void *arg);