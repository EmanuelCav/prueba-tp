#include "../include/storage_worker.h"

extern t_log *logger;

int cantidad_workers = 0;
pthread_mutex_t mutex_workers = PTHREAD_MUTEX_INITIALIZER;

comando_t parse_comando(const char *cmd)
{
    if (strcmp(cmd, "GET_BLOCK_SIZE") == 0)
        return CMD_GET_BLOCK_SIZE;
    if (strcmp(cmd, "GET_FS_SIZE") == 0)
        return CMD_GET_FS_SIZE;
    if (strcmp(cmd, "CREATE") == 0)
        return CMD_CREATE;
    if (strcmp(cmd, "TRUNCATE") == 0)
        return CMD_TRUNCATE;
    if (strcmp(cmd, "WRITE") == 0)
        return CMD_WRITE;
    if (strcmp(cmd, "READ") == 0)
        return CMD_READ;
    if (strcmp(cmd, "TAG") == 0)
        return CMD_TAG;
    if (strcmp(cmd, "COMMIT") == 0)
        return CMD_COMMIT;
    if (strcmp(cmd, "DELETE") == 0)
        return CMD_DELETE;
    if (strcmp(cmd, "END") == 0)
        return CMD_END;
    return CMD_UNKNOWN;
}

void *manejar_worker(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[256];
    int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
    {
        close(client_sock);
        return NULL;
    }
    buffer[bytes] = '\0';

    int worker_id = atoi(buffer);

    pthread_mutex_lock(&mutex_workers);
    cantidad_workers++;
    log_info(logger, "##Se conecta el Worker %d - Cantidad de Workers: %d", worker_id, cantidad_workers);
    pthread_mutex_unlock(&mutex_workers);

    comando_t cmd = parse_comando(buffer);

    switch (cmd)
    {
    case CMD_GET_BLOCK_SIZE:
    {
        char resp[32];
        sprintf(resp, "%d", 128);
        send(client_sock, resp, strlen(resp), 0);
        log_info(logger, "## Worker solicitó tamaño de bloque, enviado %s", resp);
        break;
    }
    case CMD_GET_FS_SIZE:
    {
        char resp[32];
        sprintf(resp, "%d", 4096);
        send(client_sock, resp, strlen(resp), 0);
        log_info(logger, "## Worker solicitó tamaño de FS, enviado %s", resp);
        break;
    }
    case CMD_CREATE:
        char file[64], tag[64];
        int tamanio, query_id;
        sscanf(buffer, "%*[^|]|%d|%[^|]|%[^|]|%d", &query_id, file, tag, &tamanio);

        char path_file[4096];
        snprintf(path_file, sizeof(path_file), "./files/%s", file);
        mkdir(path_file, 0777);

        char path_tag[4096];
        snprintf(path_tag, sizeof(path_tag), "./files/%s/%s", file, tag);
        mkdir(path_tag, 0777);

        char path_logical[4096];
        snprintf(path_logical, sizeof(path_logical), "./files/%s/%s/logical_blocks", file, tag);
        mkdir(path_logical, 0777);

        char path_metadata[4096];
        snprintf(path_metadata, sizeof(path_metadata), "./files/%s/%s/metadata.config", file, tag);

        FILE *meta = fopen(path_metadata, "w");
        if (meta)
        {
            fprintf(meta, "TAMAÑO=%d\n", tamanio);
            fprintf(meta, "BLOCKS=[0]\n");
            fprintf(meta, "ESTADO=WORK_IN_PROGRESS\n");
            fclose(meta);
            log_info(logger, "Archivo metadata creado: %s", path_metadata);
        }
        else
        {
            log_error(logger, "Error creando metadata: %s", path_metadata);
        }
        char respuesta[512];
        sprintf(respuesta, "OK|CREATE|%s|%s", file, tag);
        send(client_sock, respuesta, sizeof(respuesta), 0);
        log_info(logger, "##%d- File Creado %s:%s", query_id, file, tag);
        break;
    case CMD_TRUNCATE:
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - File Truncado <NOMBRE_FILE>:<TAG> - Tamaño: <TAMAÑO>");
        break;
    case CMD_WRITE:
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Bloque Lógico Escrito <NOMBRE_FILE>:<TAG> - Número de Bloque: <BLOQUE>");
        break;
    case CMD_READ:
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Bloque Lógico Leído <NOMBRE_FILE>:<TAG> - Número de Bloque: <BLOQUE>");
        break;
    case CMD_TAG:
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Tag creado <NOMBRE_FILE>:<TAG>");
        break;
    case CMD_COMMIT:
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Commit de File:Tag <NOMBRE_FILE>:<TAG>");
        break;
    case CMD_DELETE:
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Tag Eliminado <NOMBRE_FILE>:<TAG>");
        break;
    case CMD_END:
        send(client_sock, "OK", 2, 0);
        break;
    case CMD_UNKNOWN:
    default:
        send(client_sock, "ERR_UNKNOWN_COMMAND", 20, 0);
        break;
    }

    close(client_sock);

    pthread_mutex_lock(&mutex_workers);
    cantidad_workers--;
    log_info(logger, "##Se desconecta el Worker %d - Cantidad de Workers: %d", worker_id, cantidad_workers);
    pthread_mutex_unlock(&mutex_workers);

    return NULL;
}