#ifndef STORAGE_H
#define STORAGE_H

#define STORAGE_LOG_PATH "./logs/storage.log"
#define STORAGE_MODULE_NAME "STORAGE"

#define MAX_BUFFER 1024

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

#include <commons/config.h>
#include <commons/log.h>

#include "storage_fs.h"
#include "storage_worker.h"
#include "storage_config.h"

#include "../../utils/src/conexiones/conexiones.h"

/**
 * @brief Logger global para el módulo Storage.
 */

#endif