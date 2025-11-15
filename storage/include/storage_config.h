#ifndef STORAGE_CONFIG_H
#define STORAGE_CONFIG_H

#include <commons/config.h>
#include <commons/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct
{
    int puerto_escucha;
    int fresh_start;
    char punto_montaje[256];
    int retardo_operacion;
    int retardo_acceso_bloque;
    char log_level[16];
    int block_size;
    int fs_size;
} t_storage_config;

/**
 * @brief Lee el archivo de configuración del módulo Storage
 *
 * @param path Ruta al archivo de configuración
 * @return t_storage_config* Puntero a la estructura con los datos cargados
 */
t_storage_config *leer_config_storage(const char *path);

/**
 * @brief Libera todos los recursos utilizados por el módulo Storage.
 *
 * @param cfg Puntero a la estructura de configuración del Storage.
 * @param logger Puntero al logger del Storage.
 */
void limpiar_recursos_storage(t_storage_config *cfg, t_log *logger);

#endif