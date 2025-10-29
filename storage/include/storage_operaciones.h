#ifndef STORAGE_OPERACIONES_H
#define STORAGE_OPERACIONES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>

#include "storage_config.h"

/**
 * @brief Crea un nuevo File:Tag en el sistema de archivos del Storage.
 *
 * @param nombre_file Nombre del archivo lógico (sin extensión).
 * @param tag Tag asociado al archivo (por ejemplo, "BASE").
 * @param logger Logger de storage para registrar la operación.
 */
void crear_file_tag(const char *nombre_file, const char *tag, t_log *logger);

/**
 * @brief Trunca o expande un File:Tag a un nuevo tamaño en bytes.
 *
 * @param nombre_file Nombre del archivo.
 * @param tag Tag asociado.
 * @param nuevo_tam Nuevo tamaño en bytes.
 * @param cfg Configuración del Storage (usa block_size y fs_size).
 * @param logger Logger de Storage.
 */
void truncar_file_tag(const char *nombre_file, const char *tag, int nuevo_tam, t_storage_config *cfg, t_log *logger);

/**
 * @brief Escribe datos en un bloque lógico de un File:Tag.
 *
 * @param nombre_file Nombre del archivo.
 * @param tag Tag asociado.
 * @param num_bloque Número de bloque lógico (a partir de 0).
 * @param contenido Contenido a escribir en el bloque.
 * @param cfg Configuración del Storage (usa block_size).
 * @param logger Logger de Storage.
 */
void escribir_bloque(const char *nombre_file, const char *tag, int num_bloque, const char *contenido, t_storage_config *cfg, t_log *logger);

/**
 * @brief Lee un bloque lógico de un File:Tag.
 *
 * @param nombre_file Nombre del archivo.
 * @param tag Tag asociado.
 * @param num_bloque Número de bloque lógico (a partir de 0).
 * @param cfg Configuración del Storage (usa block_size).
 * @param logger Logger de Storage.
 * @return char* Buffer con el contenido del bloque (terminado en '\0'), o NULL si ocurre un error.
 */
char *leer_bloque(const char *nombre_file, const char *tag, int num_bloque, t_storage_config *cfg, t_log *logger);

#endif