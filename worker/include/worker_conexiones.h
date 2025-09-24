#ifndef WORKER_CONEXIONES_H
#define WORKER_CONEXIONES_H

#include <commons/log.h>
#include <commons/string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "worker_config.h"

#include "../../utils/src/conexiones/conexiones.h"

#define MAX_BUFFER 1024

/**
 * @brief Conecta el Worker al Master y envía la identificación inicial.
 *
 * @param cfg Configuración del Worker.
 * @param logger Logger para registrar la información de conexión.
 * @param worker_id ID a fin de que el Master pueda identificarlo.
 * @return int Socket conectado al Master.
 *
 * @example
 * int sock_master = conectar_al_master(cfg, logger, 1);
 */
int conectar_al_master(t_worker_config *cfg, t_log *logger, int worker_id);

/**
 * @brief Recibe los datos de la Query enviada por el Master.
 *
 * @param sock_master Socket conectado al Master.
 * @param query_id Puntero para almacenar el ID de la query.
 * @param path_query Buffer para almacenar el path de la query.
 * @param prioridad Puntero para almacenar la prioridad de la query.
 * @param logger Logger para imprimir mensajes de información.
 *
 * @example
 * int query_id, prioridad;
 * char path_query[512];
 * recibir_query(sock_master, &query_id, path_query, &prioridad, logger);
 */
void recibir_query(int sock_master, int *query_id, char *path_query, int *prioridad, t_log *logger);

/**
 * @brief Conecta al Storage y solicita información, por ejemplo, tamaño de bloque.
 *
 * @param cfg Configuración del Worker.
 * @param logger Logger para imprimir mensajes.
 * @param query_id ID de la query en ejecución.
 *
 * @example
 * consultar_storage(cfg, logger, query_id);
 */
void consultar_storage(t_worker_config *cfg, t_log *logger, int query_id);

#endif
