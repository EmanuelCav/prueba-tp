#include "../include/storage_worker.h"

extern t_log *logger;

int cantidad_workers = 0;
pthread_mutex_t mutex_workers = PTHREAD_MUTEX_INITIALIZER;

comando_t parse_comando(const char *cmd)
{
    if (strncmp(cmd, "GET_BLOCK_SIZE") == 0)
        return CMD_GET_BLOCK_SIZE;
    if (strncmp(cmd, "GET_FS_SIZE") == 0)
        return CMD_GET_FS_SIZE;
    if (strncmp(cmd, "CREATE") == 0)
        return CMD_CREATE;
    if (strncmp(cmd, "TRUNCATE") == 0)
        return CMD_TRUNCATE;
    if (strncmp(cmd, "WRITE") == 0)
        return CMD_WRITE;
    if (strncmp(cmd, "READ") == 0)
        return CMD_READ;
    if (strncmp(cmd, "TAG") == 0)
        return CMD_TAG;
    if (strncmp(cmd, "COMMIT") == 0)
        return CMD_COMMIT;
    if (strncmp(cmd, "DELETE") == 0)
        return CMD_DELETE;
    if (strncmp(cmd, "END") == 0)
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
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - File Creado <NOMBRE_FILE>:<TAG>");
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