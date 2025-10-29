#include "../include/worker_conexiones.h"

int conectar_al_master(t_worker_config *cfg, t_log *logger, int worker_id)
{
    char puerto_str[6];
    sprintf(puerto_str, "%d", cfg->puerto_master);

    int sock_master = conectar_servidor(cfg->ip_master, puerto_str);
    if (sock_master < 0)
    {
        log_error(logger, "No se pudo conectar al Master");
        exit(EXIT_FAILURE);
    }

    char workerSendId[64];
    int len = snprintf(workerSendId, sizeof(workerSendId), "WORKER|%d\n", worker_id);
    if (len < 0 || len >= (int)sizeof(workerSendId))
    {
        log_error(logger, "Error creando workerSendId");
        close(sock_master);
        exit(EXIT_FAILURE);
    }

    if (send(sock_master, workerSendId, len, 0) < 0)
    {
        log_error(logger, "No se pudo enviar el workerSendId al Master");
        close(sock_master);
        exit(EXIT_FAILURE);
    }

    log_info(logger, "## Worker conectado al Master (%s:%s) con ID %d",
             cfg->ip_master, puerto_str, worker_id);

    return sock_master;
}

void recibir_query(int sock_master, int *query_id, char *path_query, int *prioridad, int *program_counter, t_log *logger)
{
    char buffer[MAX_BUFFER];
    int bytes = recv(sock_master, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
    {
        perror("recv");
        close(sock_master);
        exit(EXIT_FAILURE);
    }
    buffer[bytes] = '\0';

    sscanf(buffer, "%d|%[^|]|%d|%d", query_id, path_query, prioridad, program_counter);

    log_info(logger, "## Query %d: Se recibe la Query. El path de operaciones es: %s", *query_id, path_query);
}

int consultar_storage(t_worker_config *cfg, t_log *logger)
{
    char puerto_str[6];
    sprintf(puerto_str, "%d", cfg->puerto_storage);

    int sock_storage = conectar_servidor(cfg->ip_storage, puerto_str);
    if (sock_storage < 0)
    {
        log_error(logger, "No se pudo conectar al Storage para el handshake inicial");
        exit(EXIT_FAILURE);
    }

    const char *mensaje = "GET_BLOCK_SIZE";
    if (send(sock_storage, mensaje, strlen(mensaje), 0) < 0)
    {
        log_error(logger, "Error enviando GET_BLOCK_SIZE al Storage");
        close(sock_storage);
        exit(EXIT_FAILURE);
    }

    char respuesta[64];
    int bytes = recv(sock_storage, respuesta, sizeof(respuesta) - 1, 0);
    if (bytes <= 0)
    {
        log_error(logger, "Error recibiendo tamaño de bloque del Storage");
        close(sock_storage);
        exit(EXIT_FAILURE);
    }

    respuesta[bytes] = '\0';
    int tamanio_bloque = atoi(respuesta);

    if (tamanio_bloque <= 0)
    {
        log_error(logger, "Tamaño de bloque inválido recibido del Storage: %s", respuesta);
        close(sock_storage);
        exit(EXIT_FAILURE);
    }

    log_info(logger, "## Worker: Tamaño de bloque recibido desde Storage: %d bytes", tamanio_bloque);

    close(sock_storage);

    return tamanio_bloque;
}