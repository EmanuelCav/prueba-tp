#ifndef STORAGE_FS_H
#define STORAGE_FS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "storage_config.h"

/**
 * @brief Inicializa la estructura del File System (si fresh_start es TRUE crea desde cero)
 *
 * @param cfg Configuración cargada de storage
 */
void inicializar_fs(t_storage_config *cfg);

#endif