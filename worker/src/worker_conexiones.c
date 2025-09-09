#include "../include/worker_conexiones.h"

int conectar_al_master(t_worker_config *cfg, t_log *logger)
{
    int sock_master = conectar_servidor(cfg->ip_master, cfg->puerto_master);
    send(sock_master, "WORKER", strlen("WORKER"), 0);
    log_info(logger, "## Worker conectado al Master (%s:%d)", cfg->ip_master, cfg->puerto_master);
    return sock_master;
}

void recibir_query(int sock_master, int *query_id, char *path_query, int *prioridad, t_log *logger)
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
    sscanf(buffer, "%d|%d[^|]|%d", query_id, path_query, prioridad);
    log_info(logger, "## Query %d: Se recibe la Query. El path de operaciones es: %s", *query_id, path_query);
}

void consultar_storage(t_worker_config *cfg, t_log *logger, int query_id)
{
    int sock_storage = conectar_servidor(cfg->ip_storage, cfg->puerto_storage);
    send(sock_storage, "GET_BLOCK_SIZE", strlen("GET_BLOCK_SIZE"), 0);

    char respuesta[64];
    int bytes = recv(sock_storage, respuesta, sizeof(respuesta) - 1, 0);
    if (bytes > 0)
    {
        respuesta[bytes] = '\0';
        log_info(logger, "## Query %d: Worker solicitó tamaño de bloque a Storage, recibido: %s", query_id, respuesta);
    }

    close(sock_storage);
}