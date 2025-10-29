#ifndef WORKER_QUERY_INTERPRETER_H
#define WORKER_QUERY_INTERPRETER_H

#include <string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <unistd.h>

#include "../../utils/src/conexiones/conexiones.h"

#include "worker_memoria_interna.h"
#include "worker_config.h"

typedef enum
{
    INS_CREATE,
    INS_TRUNCATE,
    INS_WRITE,
    INS_READ,
    INS_TAG,
    INS_COMMIT,
    INS_FLUSH,
    INS_DELETE,
    INS_END,
    INS_UNKNOWN
} t_instruccion;

/**
 * @brief Lee e interpreta line para ejecutar la instruccion especificada en la misma.
 *
 * @param line Linea del archivo de Query a interpretar y ejecutar
 * @param query_id Identificador de la query.
 * @param path_query Ruta al archivo de la query.
 * @param logger Logger para imprimir el seguimiento de ejecucion de la instruccion.
 * @param memoria Memoria interna.
 * @param cfg Configuracion del worker.
 * @param sock_master Socket conectado al master.
 * @param archivos_modificados Lista de file:tags para llevar conteo de archivos modificados.
 *
 */
void query_interpretar(char *line, int query_id, char *path_query, t_log *logger, t_memoria_interna *memoria, t_worker_config *cfg, int sock_master, t_list *archivos_modificados);

/**
 * @brief Lee e interpreta line calificar la instruccion a realizar
 *
 * @param line Linea del archivo de Query a interpretar
 * @return El tipo que le corresponde a la instruccion dentro de t_instruccion;
 *
 */
t_instruccion instr_to_enum(char *line);

/**
 * @brief Envía una solicitud al módulo Storage
 *
 * @param cfg Configuración del worker.
 * @param logger Logger para mensajes.
 * @param query_id ID de la query.
 * @param comando Comando a enviar al storage.
 * @param respuesta Buffer para recibir la respuesta.
 * @param tam_respuesta Tamaño del buffer de respuesta.
 * @return int 1 si fue exitoso, 0 si hubo error.
 */
int enviar_comando_storage(t_worker_config *cfg, t_log *logger, int query_id, const char *comando, char *respuesta, int tam_respuesta);

/**
 * @brief Envía datos al módulo Master
 *
 * @param sock_master Socket conectado al master.
 * @param logger Logger para mensajes.
 * @param query_id ID de la query.
 * @param comando Comando a enviar.
 * @param datos Datos adicionales a enviar.
 * @return int 1 si fue exitoso, 0 si hubo error.
 */
int enviar_comando_master(int sock_master, t_log *logger, int query_id, const char *comando, const char *datos);

/**
 * @brief Escribe contenido en la memoria interna
 *
 * @param memoria Memoria interna.
 * @param direccion Dirección base donde escribir.
 * @param contenido Contenido a escribir.
 * @param logger Logger para mensajes.
 * @param query_id ID de la query.
 */
void escribir_memoria(t_memoria_interna *memoria, int direccion, const char *contenido, t_log *logger, int query_id);

/**
 * @brief Lee contenido de la memoria interna
 *
 * @param memoria Memoria interna.
 * @param direccion Dirección base de donde leer.
 * @param tamanio Tamaño a leer.
 * @param logger Logger para mensajes.
 * @param query_id ID de la query.
 * @param sock_master Socket para enviar datos al master.
 */
void leer_memoria(t_memoria_interna *memoria, int direccion, int tamanio, t_log *logger, int query_id, int sock_master);

/**
 * @brief Persiste modificaciones de un file:tag en storage
 *
 * @param cfg Configuración del worker.
 * @param logger Logger para mensajes.
 * @param query_id ID de la query.
 * @param memoria Memoria interna.
 * @param file Nombre del archivo.
 * @param tag Tag del archivo.
 */
void flush_file_to_storage(t_worker_config *cfg, t_log *logger, int query_id, t_memoria_interna *memoria, const char *file, const char *tag);

/**
 * @brief Carga una página de un archivo desde el módulo Storage hacia la memoria interna del Worker.
 *
 * @param cfg Configuración del Worker (contiene IP y puerto del Storage).
 * @param logger Logger del Worker para imprimir los eventos y errores.
 * @param query_id Identificador de la query actual en ejecución.
 * @param memoria Estructura de memoria interna del Worker donde se guardan las páginas.
 * @param marco Índice del marco de memoria donde se almacenará la página cargada.
 * @param file Nombre del archivo (File) que contiene la página solicitada.
 * @param tag Etiqueta (Tag) asociada al archivo.
 * @param numero_pagina Número de la página dentro del archivo que se desea cargar.
 *
 */
void cargar_pagina_desde_storage(t_worker_config *cfg, t_log *logger, int query_id, t_memoria_interna *memoria, int marco, const char *file, const char *tag, int numero_pagina);

#endif