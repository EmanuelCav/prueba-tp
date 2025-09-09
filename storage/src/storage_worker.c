#include "../include/storage_worker.h"

void *manejar_worker(void *arg) {
    int sock = *((int*)arg);
    free(arg);

    char buffer[MAX_BUFFER];
    int bytes = recv(sock, buffer, MAX_BUFFER, 0);
    if (bytes <= 0) return NULL;

    buffer[bytes] = '\0';
    if (strcmp(buffer, "GET_BLOCK_SIZE") == 0) {
        char respuesta[32];
        sprintf(respuesta, "%d", 128);
        send(sock, respuesta, strlen(respuesta), 0);
        log_info(logger, "## Worker solicitó tamaño de bloque, enviado %s", respuesta);
    }

    close(sock);
    return NULL;
}

