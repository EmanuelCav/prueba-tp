#ifndef MASTER_CONFIG_H
#define MASTER_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>

typedef struct
{
    int puerto_escucha;
    char algoritmo_planificacion[20];
    int tiempo_aging;
    char log_level[10];
} t_config_master;

/**
 * @brief Lee la configuración del Master desde un archivo
 *
 * @param path Ruta al archivo de configuración
 * @return t_config_master* Estructura con los valores cargados
 */
t_config_master *master_leer_config(const char *path);

#endif
