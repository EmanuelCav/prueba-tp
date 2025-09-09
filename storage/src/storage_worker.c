#include "../include/storage_worker.h"

void *manejar_worker(void *arg, t_log *logger)
{
    int sock = *((int *)arg);
    free(arg);

    log_info(logger, "## Se conecta un Worker - Socket: %d", sock);

    char buffer[MAX_BUFFER];
    int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
    {
        close(sock);
        return NULL;
    }
    buffer[bytes] = '\0';

    // Solicitud BLOCK_SIZE
    if (strcmp(buffer, "GET_BLOCK_SIZE") == 0)
    {
        char respuesta[32];
        sprintf(respuesta, "%d", 128);
        send(sock, respuesta, strlen(respuesta), 0);
        log_info(logger, "## Worker solicitó tamaño de bloque, enviado: %s", respuesta);
    }

    close(sock);
    return NULL;
}
