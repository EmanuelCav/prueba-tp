#ifndef STORAGE_BITMAP_H
#define STORAGE_BITMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <commons/log.h>

#include "storage_config.h"

/**
 * @brief Busca el primer bloque libre disponible en el bitmap y lo marca como ocupado.
 *
 * @param cfg Puntero a la estructura de configuración del storage (t_storage_config),
 * @param logger Puntero al logger utilizado para registrar mensajes e información de errores.
 */
int buscar_bloque_libre(t_storage_config *cfg, t_log *logger);

/**
 * @brief Libera un bloque físico previamente ocupado en el bitmap.
 *
 * @param bloque Número del bloque físico a liberar.
 * @param cfg Puntero a la estructura de configuración del storage (t_storage_config),
 * @param logger Puntero al logger utilizado para registrar mensajes e información de errores.
 */
void liberar_bloque(int bloque, t_storage_config *cfg, t_log *logger);

#endif