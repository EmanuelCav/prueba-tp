#ifndef CONEXION_H_
#define CONEXION_H_

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define BACKLOG 10

/**
 * @brief Inicia un servidor en la ip y puerto correspondientes
 *
 * @param ip: ip del servidor que estamos creando
 * @param puerto: puerto del servidor que estamos creando
 * @return int: ID del file descriptor del socket del servidor
 *
 * @example int socketServidor = iniciar_servidor(2340);
 */
int iniciar_servidor(char *puerto);

/**
 * @brief Inicia un cliente conectandose a un servidor en la ip y puerto correspondientes
 *
 * @param ip: ip del servidor al que nos conectamos
 * @param puerto: puerto del servidor al cual nos conectamos
 * @return int: ID del file descriptor del socket creado con el servidor
 *
 * @example int socketCliente = conectar_a_servidor("192.168.0.1", 2340);
 */
int conectar_servidor(char *ip, char *puerto);

#endif