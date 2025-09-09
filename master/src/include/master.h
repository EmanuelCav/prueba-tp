#ifndef QUERY_CONTROL_H_
#define QUERY_CONTROL_H_

#define MASTER_LOG_PATH "./logs/master.log"
#define MASTER_MODULE_NAME "MASTER"

#define MAX_BUFFER 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>

#include <commons/config.h>
#include <commons/log.h>

#include "master_config.h"
#include "master_workers.h"
#include "utils/conexiones/conexiones.h"

t_log *logger;

#endif