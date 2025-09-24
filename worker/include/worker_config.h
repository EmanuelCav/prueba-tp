#ifndef WORKER_CONFIG_H
#define WORKER_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>

#include "worker_memoria_interna.h"

/**
 * @brief Estructura de configuración del Worker.
 *
 * Contiene todos los parámetros necesarios para inicializar un Worker
 */
typedef struct
{
    char ip_master[16];
    int puerto_master;
    char ip_storage[16];
    int puerto_storage;
    int tam_memoria;
    int retardo_memoria;
    char algoritmo_reemplazo[16];
    char path_queries[256];
    char log_level[16];
} t_worker_config;

/**
 * @brief Lee el archivo de configuración del Worker y devuelve una estructura con los datos.
 *
 * @param path_config Ruta al archivo de configuración.
 * @return t_worker_config* Instancia creada e inicializada de la configuración del Worker.
 *
 * @example
 * t_worker_config* cfg = leer_config_worker("./config/worker.config");
 */
t_worker_config *leer_config_worker(char *path_config);

/**
 * @brief Libera recursos al finalizar la ejecución del Worker.
 */
void limpiar_recursos_worker(int sock_master, t_worker_config *cfg, t_log *logger, t_memoria_interna *memoria, char *line, FILE *f)

#endif