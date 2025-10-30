#ifndef STORAGE_WORKER_H
#define STORAGE_WORKER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/crypto.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

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

extern int cantidad_workers;
extern pthread_mutex_t mutex_workers;

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

/**
 * @brief Saca el ultimo bloque dentro del archivo metadata de un file o agrega
 * un bloque asociado al bloque fisico 0
 *
 * @param op "AGREGAR" o "SACAR" dependiendo de la operacion 
 * @param path_metadata El path al archivo de metadata del filetag
 * @return El bloque fisico asociado del bloque logico agregado/sacado o -1 de haber error
 */
int actualizar_metadata(char *op, char path_metadata[4096]);

/**
 * @brief Deslinkea un bloque logico de su bloque fisico, comprueba si luego sigue manteniendo 
 * links activos y, de caso contrario, lo marca libre en el bitmap
 *
 * @param client_sock Sock del worker que hizo la peticion.
 * @param file Nombre del archivo
 * @param tag Nombre del tag
 * @param path_metadata Path al archivo de metadata del filetag
 * @param path_logical_block Path del bloque logico que se deslinkeara
 */
void sacar_bloque_filetag(int client_sock, char file[64], char tag[64], char path_metadata[4096], char path_logical_block[4096], t_log *logger);

/**
 * @brief Marca un bloque como libre en el bitmap, cambiando su bit a 0
 *
 * @param num_bloque Numero del bloque FISICO a marcar como libre
 */
void marcar_bloque_libre(int num_bloque, t_log *logger);

#endif
