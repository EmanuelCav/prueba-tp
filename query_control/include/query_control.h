#ifndef QUERY_CONTROL_H
#define QUERY_CONTROL_H

#define QUERY_CONTROL_LOG_PATH "./logs/query_control.log"
#define QUERY_CONTROL_MODULE_NAME "QUERY_CONTROL"

#define MAX_BUFFER 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>

#include "query_control_config.h"
#include "query_control_conexiones.h"
#include "query_control_lectura.h"

#endif
