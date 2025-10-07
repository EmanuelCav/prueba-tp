#ifndef MASTER_H_
#define MASTER_H_

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
#include "master_queues.h"
#include "master_conexiones.h"

#include "../../utils/src/conexiones/conexiones.h"

#define MASTER_LOG_PATH "./logs/master.log"
#define MASTER_MODULE_NAME "MASTER"

extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_exec;

#endif