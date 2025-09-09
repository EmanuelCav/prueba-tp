#ifndef QUERY_CONTROL_CONFIG_H
#define QUERY_CONTROL_CONFIG_H

#include <commons/config.h>
#include <commons/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * @struct t_config_config
 * @brief Estructura que representa la configuración del módulo Query Control.
 */
typedef struct
{
    char *ip_master;
    int puerto_master;
    char *log_level;
} t_config_config;

/**
 * @brief Función para leer el archivo de configuración del módulo Query Control.
 *
 * @param path Ruta hacia el archivo de configuración.
 * @return t_config_config* Instancia creada e inicializada de la estructura de configuración.
 *
 * @example
 * t_config_config *cfg = leer_config("./config/query_control.config");
 */
t_config_config *leer_config(char *path);

/**
 * @brief Libera recursos y cierra las conexiones utilizadas por Query Control.
 *
 * @param socket_master Socket conectado al Master.
 * @param logger Logger utilizado en el proceso.
 * @param cfg Estructura de configuración cargada previamente.
 *
 * @example
 * limpiar_recursos(socket_master, logger, cfg);
 */
void limpiar_recursos(int socket_master, t_log *logger, t_config_config *cfg);

#endif
