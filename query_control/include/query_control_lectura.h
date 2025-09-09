#ifndef QUERY_CONTROL_LECTURA_H
#define QUERY_CONTROL_LECTURA_H

#include <commons/log.h>
#include <commons/string.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

#define MAX_BUFFER 1024

/**
 * @brief Envía una solicitud de ejecución de query al Master.
 *
 * @param socket_master Socket conectado al Master.
 * @param path_query Ruta al archivo de query a ejecutar.
 * @param prioridad Prioridad asignada a la ejecución del query.
 * @param logger Logger para registrar la operación realizada.
 *
 * @example
 * enviar_solicitud_query(socket_master, "./queries/q1.sql", 5, logger);
 */
void enviar_solicitud_query(int socket_master, char *path_query, int prioridad, t_log *logger);

/**
 * @brief Escucha y procesa las respuestas enviadas por el Master.
 *
 * @param socket_master Socket conectado al Master.
 * @param logger Logger para registrar los mensajes recibidos y eventos.
 *
 * @example
 * escuchar_respuestas_master(socket_master, logger);
 */
void escuchar_respuestas_master(int socket_master, t_log *logger);

#endif
