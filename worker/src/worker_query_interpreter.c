#include "../include/worker_query_interpreter.h"

t_instruccion instr_to_enum(char *line)
{
    char buffer[256];
    strcpy(buffer, line);
    char *instruccion = strtok(buffer, " ");

    if (strcmp(instruccion, "CREATE") == 0)
    {
        return INS_CREATE;
    }
    else if (strcmp(instruccion, "TRUNCATE") == 0)
    {
        return INS_TRUNCATE;
    }
    else if (strcmp(instruccion, "WRITE") == 0)
    {
        return INS_WRITE;
    }
    else if (strcmp(instruccion, "READ") == 0)
    {
        return INS_READ;
    }
    else if (strcmp(instruccion, "TAG") == 0)
    {
        return INS_TAG;
    }
    else if (strcmp(instruccion, "COMMIT") == 0)
    {
        return INS_COMMIT;
    }
    else if (strcmp(instruccion, "FLUSH") == 0)
    {
        return INS_FLUSH;
    }
    else if (strcmp(instruccion, "DELETE") == 0)
    {
        return INS_DELETE;
    }
    else if (strcmp(instruccion, "END") == 0)
    {
        return INS_END;
    }
    else
    {
        return INS_UNKNOWN;
    }
}

void query_interpretar(char *line, int query_id, char *path_query, t_log *logger, t_memoria_interna *memoria)
{
    static int program_counter = 0;

    log_info(logger, "## Query %d: FETCH - Program Counter: %d - %s",
             query_id, program_counter, line);

    program_counter++;

    char buffer[512];
    strcpy(buffer, line);
    char *params = strtok(NULL, "");

    char file[64], tag[64];
    int direccion, tamanio;
    char contenido[256];
    int marco_libre;

    switch (instr_to_enum(line))
    {
    case INS_CREATE:
        sscanf(params, "%[^:]:%s", file, tag);
        log_info(logger, "## Query %d: - Instrucción realizada: CREATE %s:%s", query_id, file, tag);
        break;

    case INS_TRUNCATE:
        sscanf(params, "%[^:]:%s %d", file, tag, &tamanio);
        log_info(logger, "## Query %d: - Instrucción realizada: TRUNCATE %s:%s tamaño %d", query_id, file, tag, tamanio);
        break;

    case INS_WRITE:
        sscanf(params, "%[^:]:%s %d %[^\n]", file, tag, &direccion, contenido);

        marco_libre = buscar_marco_libre(memoria);
        if (marco_libre == -1)
        {
            int victima = seleccionar_victima_LRU(memoria);
            log_info(logger, "## Query %d: Se reemplaza la página %s:%s por %s:%s",
                     query_id, memoria->marcos[victima].file, memoria->marcos[victima].tag, file, tag);
            liberar_marco(memoria, victima);
            marco_libre = victima;
        }

        asignar_pagina(memoria, marco_libre, direccion / memoria->tamanio_pagina, file, tag);

        log_info(logger, "Query %d: Acción: ESCRIBIR - Dirección Física: %d - Valor: %s", query_id, direccion, contenido);
        log_info(logger, "## Query %d: - Instrucción realizada: WRITE", query_id);
        break;

    case INS_READ:
        sscanf(params, "%[^:]:%s %d %d", file, tag, &direccion, &tamanio);
        marco_libre = buscar_marco_libre(memoria);
        if (marco_libre == -1)
        {
            int victima = seleccionar_victima_LRU(memoria);
            log_info(logger, "## Query %d: Se reemplaza la página %s:%s por %s:%s",
                     query_id, memoria->marcos[victima].file, memoria->marcos[victima].tag, file, tag);
            liberar_marco(memoria, victima);
            marco_libre = victima;
        }

        asignar_pagina(memoria, marco_libre, direccion / memoria->tamanio_pagina, file, tag);

        log_info(logger, "Query %d: Acción: LEER - Dirección Física: %d - Valor: ...", query_id, direccion);
        log_info(logger, "## Query %d: - Instrucción realizada: READ", query_id);
        break;

    case INS_TAG:
    {
        char file_origen[64], tag_origen[64], file_dest[64], tag_dest[64];
        sscanf(params, "%[^:]:%s %[^:]:%s", file_origen, tag_origen, file_dest, tag_dest);
        log_info(logger, "## Query %d: - Instrucción realizada: TAG %s:%s -> %s:%s",
                 query_id, file_origen, tag_origen, file_dest, tag_dest);
    }
    break;

    case INS_COMMIT:
        sscanf(params, "%[^:]:%s", file, tag);
        log_info(logger, "## Query %d: - Instrucción realizada: COMMIT %s:%s", query_id, file, tag);
        break;

    case INS_FLUSH:
        sscanf(params, "%[^:]:%s", file, tag);
        log_info(logger, "## Query %d: - Instrucción realizada: FLUSH %s:%s", query_id, file, tag);
        break;

    case INS_DELETE:
        sscanf(params, "%[^:]:%s", file, tag);
        log_info(logger, "## Query %d: - Instrucción realizada: DELETE %s:%s", query_id, file, tag);
        break;

    case INS_END:
        log_info(logger, "## Query %d: - Instrucción realizada: END", query_id);
        break;

    case INS_UNKNOWN:
        log_error(logger, "Query: %d, File: %s instrucción desconocida", query_id, path_query);
        break;

    default:
        break;
    }
}