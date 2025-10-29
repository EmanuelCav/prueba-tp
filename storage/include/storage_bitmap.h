#ifndef STORAGE_BITMAP_H
#define STORAGE_BITMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <commons/log.h>

#include "storage_config.h"

int buscar_bloque_libre(t_storage_config *cfg, t_log *logger);
void liberar_bloque(int bloque, t_storage_config *cfg, t_log *logger);

#endif