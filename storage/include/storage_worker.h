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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <openssl/md5.h>
#include <dirent.h>

#include "../include/storage_config.h"

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
 */
int actualizar_metadata(char *op, char path_metadata[4096]);

/**
 * @brief Deslinkea un bloque logico de su bloque fisico
 *
 * @param client_sock Sock del worker que hizo la peticion.
 * @param file Nombre del archivo
 * @param tag Nombre del tag
 * @param path_metadata Path al archivo de metadata del filetag
 * @param path_logical_block Path del bloque logico que se deslinkeara
 */
void sacar_bloque_filetag(int client_sock, char file[64], char tag[64], char path_metadata[4096], char path_logical_block[4096], t_log *logger);

/**
 * @brief Marca un bloque como libre en el bitmap (lo pone en 0).
 *
 * @param num_bloque Número del bloque FISICO que se quiere marcar como libre.
 * @param logger Logger para registrar la operación.
 */
void marcar_bloque_libre(int num_bloque, t_log *logger);

/**
 * @brief Busca el primer bloque libre en el bitmap y lo marca como ocupado.
 *
 * @param logger Logger para registrar la operación.
 */
int asignar_bloque_libre(t_log *logger);

/**
 * @brief Calcula el hash MD5 de un bloque físico y lo registra en blocks_hash_index.config.
 *
 * @param num_bloque Bloque físico cuyo contenido debe ser hasheado.
 * @param logger Logger para registrar la operación.
 */
void actualizar_blocks_hash_index(int num_bloque, t_log *logger);

/**
 * @brief Agrega un bloque físico al arreglo BLOCKS dentro de un metadata.config.
 *
 * @param path_metadata Ruta absoluta al archivo metadata.config.
 * @param bloque_fisico Número del bloque FISICO a agregar.
 * @return 0 si se actualiza correctamente, -1 si hubo error.
 */
int actualizar_metadata_agregar(const char *path_metadata, int bloque_fisico);

/**
 * @brief Realiza Copy-On-Write: copia el contenido de un bloque físico y devuelve uno nuevo.
 *
 * @param bloque_original Número de bloque FISICO a copiar.
 * @param logger Logger para registrar la operación.
 */
int copiar_bloque_fisico(int bloque_original, t_log *logger);

/**
 * @brief Reemplaza un bloque físico en una posición específica dentro del metadata.config.
 *
 * @param path_metadata Ruta al archivo metadata.config.
 * @param num_bloque_logico Índice del bloque lógico dentro del File:Tag.
 * @param nuevo_bloque Número del nuevo bloque físico que reemplaza al anterior.
 */
void metadata_reemplazar_bloque(const char *path_metadata, int num_bloque_logico, int nuevo_bloque);

/**
 * @brief Marca un bloque como ocupado en el bitmap (pone su bit en 1).
 *
 * @param num_bloque Número del bloque físico a marcar como ocupado.
 * @param logger Logger para registrar la operación.
 */
void marcar_bloque_ocupado(int num_bloque, t_log *logger);

int obtener_siguiente_version(const char *path_file_dir);

#endif