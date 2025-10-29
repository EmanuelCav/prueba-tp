#ifndef WORKER_H
#define WORKER_H

#define WORKER_LOG_PATH "./logs/worker.log"
#define WORKER_MODULE_NAME "WORKER"

#define MAX_BUFFER 1024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>

#include "worker_config.h"
#include "worker_conexiones.h"
#include "worker_query_interpreter.h"
#include "worker_memoria_interna.h"

t_log *logger;

#endif