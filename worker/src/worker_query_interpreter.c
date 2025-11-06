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

void query_interpretar(char *line, int query_id, char *path_query, t_log *logger, t_memoria_interna *memoria, t_worker_config *cfg, int sock_master, t_list *archivos_modificados)
{
    usleep(cfg->retardo_memoria * 1000);

    char buffer[512];
    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *instr = strtok(buffer, " ");
    char *params = strtok(NULL, "");

    t_instruccion instr_enum = instr_to_enum(instr);
    int instr_val = instr_enum;

    char file[64], tag[64];
    int direccion, tamanio;
    char contenido[256];

    if (instr_val != INS_TAG && instr_val != INS_COMMIT && instr_val != INS_FLUSH && instr_val != INS_READ)
    {
        if (params != NULL)
        {
            if (sscanf(params, "%63[^:]:%63s", file, tag) == 2)
            {
                char *file_tag = string_from_format("%s:%s", file, tag);
                if (!existe_file_tag(archivos_modificados, file_tag))
                {
                    list_add(archivos_modificados, string_duplicate(file_tag));
                }
                free(file_tag);
            }
            else
            {
                log_warning(logger, "## Query %d: Parámetros incompletos al intentar registrar file:tag: '%s'", query_id, params);
            }
        }
    }

    switch (instr_val)
    {
    case INS_CREATE:
        if (params == NULL)
        {
            log_error(logger, "## Query %d: CREATE sin parámetros (se esperaba file:tag)", query_id);
            break;
        }
        if (sscanf(params, "%63[^:]:%63s", file, tag) != 2)
        {
            log_error(logger, "## Query %d: CREATE parámetros inválidos: '%s'", query_id, params);
            break;
        }

        {
            char comando_create[256];
            sprintf(comando_create, "CREATE|%d|%s|%s|0", query_id, file, tag);
            char respuesta_create[64];

            if (enviar_comando_storage(cfg, logger, query_id, comando_create, respuesta_create, sizeof(respuesta_create)))
                log_info(logger, "## Query %d: - Instrucción realizada: CREATE %s:%s", query_id, file, tag);
            else
                log_error(logger, "## Query %d: Error en CREATE %s:%s", query_id, file, tag);
        }
        break;

    case INS_TRUNCATE:
        if (params == NULL)
        {
            log_error(logger, "## Query %d: TRUNCATE sin parámetros", query_id);
            break;
        }
        if (sscanf(params, "%63[^:]:%63s %d", file, tag, &tamanio) != 3)
        {
            log_error(logger, "## Query %d: TRUNCATE parámetros inválidos: '%s'", query_id, params);
            break;
        }

        {
            char comando_truncate[256];
            sprintf(comando_truncate, "TRUNCATE|%d|%s|%s|%d", query_id, file, tag, tamanio);
            char respuesta_truncate[64];

            if (enviar_comando_storage(cfg, logger, query_id, comando_truncate, respuesta_truncate, sizeof(respuesta_truncate)))
                log_info(logger, "## Query %d: - Instrucción realizada: TRUNCATE %s:%s tamaño %d", query_id, file, tag, tamanio);
            else
                log_error(logger, "## Query %d: Error en TRUNCATE %s:%s tamaño %d", query_id, file, tag, tamanio);
        }
        break;

    case INS_WRITE:
        if (params == NULL)
        {
            log_error(logger, "## Query %d: WRITE sin parámetros", query_id);
            break;
        }
        if (sscanf(params, "%63[^:]:%63s %d %[^\n]", file, tag, &direccion, contenido) < 3)
        {
            log_error(logger, "## Query %d: WRITE parámetros inválidos: '%s'", query_id, params);
            break;
        }

        {
            int numero_pagina = direccion / memoria->tamanio_pagina;
            int marco_existente = buscar_pagina_en_memoria(memoria, file, tag, numero_pagina);

            if (marco_existente == -1)
            {
                int marco_libre = buscar_marco_libre(memoria);
                if (marco_libre == -1)
                {
                    int victima = -1;
                    if (strcmp(cfg->algoritmo_reemplazo, "LRU") == 0)
                        victima = seleccionar_victima_LRU(memoria);
                    else if (strcmp(cfg->algoritmo_reemplazo, "CLOCK-M") == 0)
                        victima = seleccionar_victima_CLOCKM(memoria);

                    log_info(logger,
                             "## \nQuery %d:\nSe reemplaza la página %s:%s/%d por la %s:%s/%d",
                             query_id,
                             memoria->marcos[victima].file,
                             memoria->marcos[victima].tag,
                             memoria->marcos[victima].numero_pagina,
                             file,
                             tag,
                             numero_pagina);

                    if (memoria->marcos[victima].modificada)
                        flush_file_to_storage(cfg, logger, query_id, memoria,
                                              memoria->marcos[victima].file, memoria->marcos[victima].tag);

                    liberar_marco(memoria, victima, logger);
                    marco_libre = victima;
                }

                asignar_pagina(memoria, marco_libre, numero_pagina, file, tag, logger);
                cargar_pagina_desde_storage(cfg, logger, query_id, memoria, marco_libre, file, tag, numero_pagina);
                marco_existente = marco_libre;
            }

            escribir_memoria(memoria, direccion, contenido, logger, query_id);
            log_info(logger, "## Query %d: - Instrucción realizada: WRITE %s:%s (%s)", query_id, file, tag, contenido);
        }
        break;

    case INS_READ:
        if (params == NULL)
        {
            log_error(logger, "## Query %d: READ sin parámetros", query_id);
            break;
        }
        if (sscanf(params, "%63[^:]:%63s %d %d", file, tag, &direccion, &tamanio) != 4)
        {
            log_error(logger, "## Query %d: READ parámetros inválidos: '%s'", query_id, params);
            break;
        }

        {
            int numero_pagina = direccion / memoria->tamanio_pagina;
            int marco_existente = buscar_pagina_en_memoria(memoria, file, tag, numero_pagina);

            if (marco_existente == -1)
            {
                int marco_libre = buscar_marco_libre(memoria);
                if (marco_libre == -1)
                {
                    int victima = -1;
                    if (strcmp(cfg->algoritmo_reemplazo, "LRU") == 0)
                        victima = seleccionar_victima_LRU(memoria);
                    else if (strcmp(cfg->algoritmo_reemplazo, "CLOCK-M") == 0)
                        victima = seleccionar_victima_CLOCKM(memoria);

                    log_info(logger,
                             "## \nQuery %d:\nSe reemplaza la página %s:%s/%d por la %s:%s/%d",
                             query_id,
                             memoria->marcos[victima].file,
                             memoria->marcos[victima].tag,
                             memoria->marcos[victima].numero_pagina,
                             file,
                             tag,
                             numero_pagina);

                    if (memoria->marcos[victima].modificada)
                        flush_file_to_storage(cfg, logger, query_id, memoria,
                                              memoria->marcos[victima].file, memoria->marcos[victima].tag);

                    liberar_marco(memoria, victima, logger);
                    marco_libre = victima;
                }

                asignar_pagina(memoria, marco_libre, numero_pagina, file, tag, logger);
                cargar_pagina_desde_storage(cfg, logger, query_id, memoria, marco_libre, file, tag, numero_pagina);
                marco_existente = marco_libre;
            }

            leer_memoria(memoria, direccion, tamanio, logger, query_id, sock_master);
            log_info(logger, "## Query %d: - Instrucción realizada: READ %s:%s [%d bytes]", query_id, file, tag, tamanio);
        }
        break;

    case INS_TAG:
        if (params == NULL)
        {
            log_error(logger, "## Query %d: TAG sin parámetros", query_id);
            break;
        }
        {
            char file_origen[64], tag_origen[64], file_dest[64], tag_dest[64];
            if (sscanf(params, "%63[^:]:%63s %63[^:]:%63s", file_origen, tag_origen, file_dest, tag_dest) != 4)
            {
                log_error(logger, "## Query %d: TAG parámetros inválidos: '%s'", query_id, params);
                break;
            }

            char *file_tag = string_from_format("%s:%s", file_dest, tag_dest);

            char comando_tag[512];
            sprintf(comando_tag, "TAG|%d|%s|%s|%s|%s", query_id, file_origen, tag_origen, file_dest, tag_dest);
            char respuesta_tag[64];

            if (enviar_comando_storage(cfg, logger, query_id, comando_tag, respuesta_tag, sizeof(respuesta_tag)))
                log_info(logger, "## Query %d: - Instrucción realizada: TAG %s:%s -> %s:%s", query_id, file_origen, tag_origen, file_dest, tag_dest);
            else
                log_error(logger, "## Query %d: Error en TAG %s:%s -> %s:%s", query_id, file_origen, tag_origen, file_dest, tag_dest);

            if (!existe_file_tag(archivos_modificados, file_tag))
                list_add(archivos_modificados, string_duplicate(file_tag));
            free(file_tag);
        }
        break;

    case INS_COMMIT:
        if (params == NULL || sscanf(params, "%63[^:]:%63s", file, tag) != 2)
        {
            log_error(logger, "## Query %d: COMMIT parámetros inválidos: '%s'", query_id, params ? params : "");
            break;
        }

        flush_file_to_storage(cfg, logger, query_id, memoria, file, tag);

        {
            char comando_commit[256];
            sprintf(comando_commit, "COMMIT|%d|%s|%s", query_id, file, tag);
            char respuesta_commit[64];

            if (enviar_comando_storage(cfg, logger, query_id, comando_commit, respuesta_commit, sizeof(respuesta_commit)))
                log_info(logger, "## Query %d: - Instrucción realizada: COMMIT %s:%s", query_id, file, tag);
            else
                log_error(logger, "## Query %d: Error en COMMIT %s:%s", query_id, file, tag);
        }
        break;

    case INS_FLUSH:
        if (params == NULL || sscanf(params, "%63[^:]:%63s", file, tag) != 2)
        {
            log_error(logger, "## Query %d: FLUSH parámetros inválidos: '%s'", query_id, params ? params : "");
            break;
        }

        flush_file_to_storage(cfg, logger, query_id, memoria, file, tag);
        log_info(logger, "## Query %d: - Instrucción realizada: FLUSH %s:%s", query_id, file, tag);
        break;

    case INS_DELETE:
        if (params == NULL || sscanf(params, "%63[^:]:%63s", file, tag) != 2)
        {
            log_error(logger, "## Query %d: DELETE parámetros inválidos: '%s'", query_id, params ? params : "");
            break;
        }

        {
            char comando_delete[256];
            sprintf(comando_delete, "DELETE|%d|%s|%s", query_id, file, tag);
            char respuesta_delete[64];

            if (enviar_comando_storage(cfg, logger, query_id, comando_delete, respuesta_delete, sizeof(respuesta_delete)))
                log_info(logger, "## Query %d: - Instrucción realizada: DELETE %s:%s", query_id, file, tag);
            else
                log_error(logger, "## Query %d: Error en DELETE %s:%s", query_id, file, tag);
        }
        break;

    case INS_END:
        enviar_comando_master(sock_master, logger, query_id, "FIN_QUERY", "");
        if (archivos_modificados && !list_is_empty(archivos_modificados))
        {
            for (int i = 0; i < list_size(archivos_modificados); i++)
            {
                char *file_tag = list_get(archivos_modificados, i);
                char file_local[64], tag_local[64];
                if (sscanf(file_tag, "%63[^:]:%63s", file_local, tag_local) == 2)
                {
                    flush_file_to_storage(cfg, logger, query_id, memoria, file_local, tag_local);
                }
            }
            list_destroy_and_destroy_elements(archivos_modificados, free);
        }

        log_info(logger, "## Query %d: - Instrucción realizada: END", query_id);
        break;

    case INS_UNKNOWN:
        log_error(logger, "Query: %d, File: %s instrucción desconocida", query_id, path_query);
        break;

    default:
        break;
    }
}

int enviar_comando_storage(t_worker_config *cfg, t_log *logger, int query_id, const char *comando, char *respuesta, int tam_respuesta)
{
    char puerto_str[6];
    sprintf(puerto_str, "%d", cfg->puerto_storage);

    int sock_storage = conectar_servidor(cfg->ip_storage, puerto_str);
    if (sock_storage < 0)
    {
        log_error(logger, "## Query %d: No se pudo conectar al Storage", query_id);
        return 0;
    }

    if (send(sock_storage, comando, strlen(comando), 0) < 0)
    {
        log_error(logger, "## Query %d: Error enviando comando al Storage: %s", query_id, comando);
        close(sock_storage);
        return 0;
    }

    int bytes = recv(sock_storage, respuesta, tam_respuesta - 1, 0);
    if (bytes > 0)
    {
        respuesta[bytes] = '\0';
        log_info(logger, "## Query %d: Respuesta del Storage: %s", query_id, respuesta);
    }

    close(sock_storage);
    return 1;
}

int enviar_comando_master(int sock_master, t_log *logger, int query_id, const char *comando, const char *datos)
{
    char mensaje[512];
    if (strlen(datos) > 0)
    {
        sprintf(mensaje, "%s|%s", comando, datos);
    }
    else
    {
        strcpy(mensaje, comando);
    }

    if (send(sock_master, mensaje, strlen(mensaje), 0) < 0)
    {
        log_error(logger, "## Query %d: Error enviando comando al Master: %s", query_id, comando);
        return 0;
    }

    log_info(logger, "## Query %d: Comando enviado al Master: %s", query_id, comando);
    return 1;
}

void escribir_memoria(t_memoria_interna *memoria, int direccion, const char *contenido, t_log *logger, int query_id)
{
    int numero_pagina = direccion / memoria->tamanio_pagina;
    int offset = direccion % memoria->tamanio_pagina;

    for (int i = 0; i < memoria->cant_marcos; i++)
    {
        if (memoria->marcos[i].ocupada && memoria->marcos[i].numero_pagina == numero_pagina)
        {
            char *marco_data = (char *)memoria->marcos[i].marco;
            strcpy(marco_data + offset, contenido);
            memoria->marcos[i].modificada = true;

            actualizar_tabla_pagina(memoria, memoria->marcos[i].file, memoria->marcos[i].tag,
                                    memoria->marcos[i].numero_pagina, i, true, true);
            memoria->marcos[i].uso = 1;

            log_info(logger, "Query %d: Acción: ESCRIBIR - Dirección Física: %d - Valor: %s", query_id, direccion, contenido);

            return;
        }
    }

    log_error(logger, "## Query %d: No se encontró la página %d en memoria para escribir", query_id, numero_pagina);
}

void leer_memoria(t_memoria_interna *memoria, int direccion, int tamanio, t_log *logger, int query_id, int sock_master)
{
    int numero_pagina = direccion / memoria->tamanio_pagina;
    int offset = direccion % memoria->tamanio_pagina;

    for (int i = 0; i < memoria->cant_marcos; i++)
    {
        if (memoria->marcos[i].ocupada && memoria->marcos[i].numero_pagina == numero_pagina)
        {
            char *marco_data = (char *)memoria->marcos[i].marco;
            char datos_leidos[256];

            strncpy(datos_leidos, marco_data + offset, tamanio);
            datos_leidos[tamanio] = '\0';

            memoria->marcos[i].uso = 1;

            log_info(logger, "Query %d: Acción: LEER - Dirección Física: %d - Valor: %s", query_id, direccion, datos_leidos);

            char comando_read[512];
            sprintf(comando_read, "READ|%d|%s", query_id, datos_leidos);
            enviar_comando_master(sock_master, logger, query_id, comando_read, "");
            return;
        }
    }

    log_error(logger, "## Query %d: No se encontró la página %d en memoria para leer", query_id, numero_pagina);
}

void flush_file_to_storage(t_worker_config *cfg, t_log *logger, int query_id, t_memoria_interna *memoria, const char *file, const char *tag)
{
    for (int i = 0; i < memoria->cant_marcos; i++)
    {
        if (memoria->marcos[i].ocupada &&
            memoria->marcos[i].modificada &&
            strcmp(memoria->marcos[i].file, file) == 0 &&
            strcmp(memoria->marcos[i].tag, tag) == 0)
        {

            char *marco_data = (char *)memoria->marcos[i].marco;
            char comando_flush[512];
            sprintf(comando_flush, "FLUSH|%d|%s|%s|%d|%.*s",
                    query_id, file, tag, memoria->marcos[i].numero_pagina,
                    memoria->tamanio_pagina, marco_data);

            char respuesta_flush[64];
            if (enviar_comando_storage(cfg, logger, query_id, comando_flush, respuesta_flush, sizeof(respuesta_flush)))
            {
                memoria->marcos[i].modificada = false;
                log_info(logger, "## Query %d: FLUSH exitoso - Marco %d del file %s:%s",
                         query_id, i, file, tag);
            }
        }
    }
}

void cargar_pagina_desde_storage(t_worker_config *cfg, t_log *logger, int query_id, t_memoria_interna *memoria, int marco, const char *file, const char *tag, int numero_pagina)
{
    log_info(logger, "Query %d: - Memoria Miss - File: %s - Tag: %s - Pagina: %d", query_id, file, tag, numero_pagina);

    char comando_get[256];
    sprintf(comando_get, "GET_PAGE|%d|%s|%s|%d", query_id, file, tag, numero_pagina);

    char respuesta[4096] = {0};

    if (!enviar_comando_storage(cfg, logger, query_id, comando_get, respuesta, sizeof(respuesta)))
    {
        log_error(logger, "## Query %d: Error al solicitar página %d del file %s:%s al Storage",
                  query_id, numero_pagina, file, tag);
        return;
    }

    t_pagina *p = &memoria->marcos[marco];

    char *marco_data = (char *)p->marco;
    strncpy(marco_data, respuesta, memoria->tamanio_pagina);
    marco_data[memoria->tamanio_pagina - 1] = '\0';

    p->uso = 1;
    p->modificada = false;
    p->numero_pagina = numero_pagina;
    strcpy(p->file, file);
    strcpy(p->tag, tag);

    log_info(logger,
             "Query %d: - Memoria Add - File: %s - Tag: %s - Pagina: %d - Marco: %d",
             query_id, file, tag, numero_pagina, marco);

    actualizar_tabla_pagina(memoria, file, tag, numero_pagina, marco, true, false);
}

bool existe_file_tag(t_list *archivos_modificados, char *file_tag)
{
    bool _coincide(void *elemento)
    {
        return strcmp((char *)elemento, file_tag) == 0;
    }
    return list_any_satisfy(archivos_modificados, _coincide);
}