#ifndef WORKER_QUERY_INTERPRETER_H
#define WORKER_QUERY_INTERPRETER_H

#include <string.h>
#include <commons/config.h>
#include <commons/log.h>

#include "worker_memoria_interna.h"

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
 * @param logger Memoria interna.
 *
 */
void query_interpretar(char *line, int query_id, char *path_query, t_log *logger, t_memoria_interna *memoria);

/**
 * @brief Lee e interpreta line calificar la instruccion a realizar
 *
 * @param line Linea del archivo de Query a interpretar
 * @return El tipo que le corresponde a la instruccion dentro de t_instruccion;
 *
 */
t_instruccion instr_to_enum(char *line);

#endif
