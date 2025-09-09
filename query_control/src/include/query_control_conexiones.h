#ifndef QUERY_CONTROL_CONEXIONES_H
#define QUERY_CONTROL_CONEXIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "utils/conexiones/conexiones.h"

/**
 * @brief Conecta el proceso Query Control al proceso Master.
 *
 * @param ip Dirección IP del proceso Master.
 * @param puerto Puerto del proceso Master.
 * @return int Descriptor de socket conectado al Master. En caso de error no retorna, aborta.
 *
 * @example
 * int sock = conectar_master("127.0.0.1", 9001);
 */
int conectar_master(char *ip, int puerto);

#endif