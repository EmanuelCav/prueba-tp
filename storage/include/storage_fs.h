#ifndef STORAGE_FS_H
#define STORAGE_FS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/crypto.h>

#include "storage_config.h"

/**
 * @brief Inicializa la estructura del File System.
 *
 * @param cfg Configuración cargada del módulo Storage.
 * @param logger Puntero al logger del módulo.
 */
void inicializar_fs(t_storage_config *cfg, t_log *logger);

/**
 * @brief Elimina el contenido previo del File System y crea la carpeta raíz.
 *
 * @param root_path Ruta raíz donde se montará el FS.
 */
void limpiar_fs(const char *root_path);

/**
 * @brief Crea las carpetas y archivos básicos del FS.
 *
 *
 * @param root_path Ruta raíz donde se montará el FS.
 */
void crear_estructura_base(const char *root_path);

/**
 * @brief Crea el archivo superblock de configuración del FS.
 *
 * @param root_path Ruta raíz del FS.
 * @param fs_size Tamaño total del File System en bytes.
 * @param block_size Tamaño de cada bloque en bytes.
 */
void crear_superblock(const char *root_path, int fs_size, int block_size);

/**
 * @brief Crea el archivo de bitmap del FS.
 *
 * @param root_path Ruta raíz del FS.
 * @param fs_size Tamaño total del File System en bytes.
 * @param block_size Tamaño de cada bloque en bytes.
 */
void crear_bitmap(const char *root_path, int fs_size, int block_size);

/**
 * @brief Crea un archivo inicial en el FS con un bloque físico y lógico.
 *
 * @param root_path Ruta raíz del FS.
 * @param block_size Tamaño de bloque a utilizar en el archivo inicial.
 * @param logger Puntero al logger del módulo.
 */
void crear_archivo_inicial(const char *root_path, int block_size, t_log *logger);

#endif