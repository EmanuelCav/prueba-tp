#include "../include/storage_worker.h"

extern t_storage_config *cfg;

extern t_log *logger;

int cantidad_workers = 0;
pthread_mutex_t mutex_workers = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_meta = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitmap = PTHREAD_MUTEX_INITIALIZER;

comando_t parse_comando(const char *line)
{
    log_debug(logger, "Parseando comando: [%s]", line);

    char buffer[256];
    strncpy(buffer, line, sizeof(buffer) - 1);

    char *cmd = strtok(buffer, "|");
    log_debug(logger, "Comando extraído: [%s]", cmd);

    if (strcmp(cmd, "GET_BLOCK_SIZE") == 0)
        return CMD_GET_BLOCK_SIZE;
    if (strcmp(cmd, "GET_FS_SIZE") == 0)
        return CMD_GET_FS_SIZE;
    if (strcmp(cmd, "CREATE") == 0)
        return CMD_CREATE;
    if (strcmp(cmd, "TRUNCATE") == 0)
        return CMD_TRUNCATE;
    if (strcmp(cmd, "WRITE") == 0)
        return CMD_WRITE;
    if (strcmp(cmd, "READ") == 0)
        return CMD_READ;
    if (strcmp(cmd, "TAG") == 0)
        return CMD_TAG;
    if (strcmp(cmd, "COMMIT") == 0)
        return CMD_COMMIT;
    if (strcmp(cmd, "DELETE") == 0)
        return CMD_DELETE;
    if (strcmp(cmd, "GET_PAGE") == 0)
        return CMD_READ;
    if (strcmp(cmd, "FLUSH") == 0)
        return CMD_WRITE;
    if (strcmp(cmd, "END") == 0)
        return CMD_END;
    return CMD_UNKNOWN;
}

void *manejar_worker(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);

    log_debug(logger, "Nuevo thread iniciado para manejar conexión (socket: %d)", client_sock);

    char buffer[256];
    int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
    {
        log_debug(logger, "Conexión cerrada sin datos recibidos (bytes: %d)", bytes);
        close(client_sock);
        return NULL;
    }
    buffer[bytes] = '\0';

    log_debug(logger, "Comando recibido: [%s] (%d bytes)", buffer, bytes);

    char buffer_original[256];
    strcpy(buffer_original, buffer);

    comando_t cmd = parse_comando(buffer_original);
    char *cmd_str = strtok(buffer, "|");

    pthread_mutex_lock(&mutex_workers);
    cantidad_workers++;
    pthread_mutex_unlock(&mutex_workers);

    int worker_id = cantidad_workers;
    log_info(logger, "##Se conecta el Worker %d - Cantidad de Workers: %d", worker_id, cantidad_workers);

    char file[64], tag[64];
    int tamanio, query_id;

    switch (cmd)
    {
    case CMD_GET_BLOCK_SIZE:
    {
        log_debug(logger, "Procesando GET_BLOCK_SIZE");
        char resp[32];
        sprintf(resp, "%d", cfg->block_size);
        send(client_sock, resp, strlen(resp), 0);
        log_info(logger, "## Worker solicitó tamaño de bloque, enviado %s", resp);
        break;
    }
    case CMD_GET_FS_SIZE:
    {
        log_debug(logger, "Procesando GET_FS_SIZE");
        char resp[32];
        sprintf(resp, "%d", cfg->fs_size);
        send(client_sock, resp, strlen(resp), 0);
        log_info(logger, "## Worker solicitó tamaño de FS, enviado %s", resp);
        break;
    }
    case CMD_CREATE:
    {
        usleep(cfg->retardo_operacion * 1000);

        int query_id, tamanio_inicial;
        sscanf(buffer_original, "%*[^|]|%d|%[^|]|%[^|]|%d",
               &query_id, file, tag, &tamanio_inicial);

        char file_path[300];
        char tag_path[300];
        char logical_path[300];
        char metadata_path[300];

        sprintf(file_path, "%s/files/%s", cfg->punto_montaje, file);
        sprintf(tag_path, "%s/files/%s/%s", cfg->punto_montaje, file, tag);
        sprintf(logical_path, "%s/files/%s/%s/logical_blocks", cfg->punto_montaje, file, tag);
        sprintf(metadata_path, "%s/files/%s/%s/metadata.config", cfg->punto_montaje, file, tag);

        mkdir_recursive(file_path);
        mkdir_recursive(tag_path);
        mkdir_recursive(logical_path);

        FILE *meta = fopen(metadata_path, "w");
        if (!meta)
        {
            log_error(logger, "Error creando metadata: %s", metadata_path);
            send(client_sock, "ERR_CREATE_METADATA", strlen("ERR_CREATE_METADATA"), 0);
            break;
        }

        fprintf(meta, "TAMAÑO=0\n");
        fprintf(meta, "BLOCKS=[]\n");
        fprintf(meta, "ESTADO=WORK_IN_PROGRESS\n");
        fclose(meta);

        log_info(logger, "##%d - File Creado %s:%s", query_id, file, tag);

        char respuesta[256];
        sprintf(respuesta, "OK|CREATE|%s|%s", file, tag);
        send(client_sock, respuesta, strlen(respuesta), 0);

        break;
    }
    case CMD_TRUNCATE:
    {
        usleep(cfg->retardo_operacion * 1000);
        sscanf(buffer_original, "%*[^|]|%d|%[^|]|%[^|]|%d", &query_id, file, tag, &tamanio);

        log_debug(logger, "Query %d: TRUNCATE - File:%s Tag:%s Nuevo tamaño:%d",
                  query_id, file, tag, tamanio);

        char path_filetag[4096];
        sprintf(path_filetag, "%s/files/%s/%s", cfg->punto_montaje, file, tag);

        struct stat st;
        if (!(stat(path_filetag, &st) == 0 && S_ISDIR(st.st_mode)))
        {
            log_error(logger, "STORAGE | TRUNCATE | Error: File:Tag %s:%s inexistente.", file, tag);
            send(client_sock, "ERR_TAG_NOT_FOUND", 17, 0);
            break;
        }

        char path_metadata[4096];
        sprintf(path_metadata, "%s/files/%s/%s/metadata.config",
                cfg->punto_montaje, file, tag);

        int tamanio_actual = 0;
        char estado[32] = "WORK_IN_PROGRESS";
        t_list *bloques = list_create();

        FILE *meta = fopen(path_metadata, "r");
        if (meta)
        {
            char linea[256];

            while (fgets(linea, sizeof(linea), meta))
            {
                if (sscanf(linea, "TAMAÑO=%d", &tamanio_actual) == 1)
                    continue;

                if (strstr(linea, "BLOCKS=["))
                {
                    char *bloques_str = strchr(linea, '[') + 1;
                    char *token = strtok(bloques_str, ",]");

                    while (token)
                    {
                        list_add(bloques, (void *)atoi(token));
                        token = strtok(NULL, ",]");
                    }
                }

                sscanf(linea, "ESTADO=%s", estado);
            }
            fclose(meta);
        }

        if (strcmp(estado, "COMMITED") == 0)
        {
            log_error(logger, "STORAGE | TRUNCATE | Error: intento de truncado en estado COMMITED.");
            send(client_sock, "ERR_TAG_COMMITED", 16, 0);
            list_destroy(bloques);
            break;
        }

        int bloques_actuales = list_size(bloques);
        int bloques_nuevos = tamanio / cfg->block_size;

        log_debug(logger, "Query %d: Bloques actuales:%d - Bloques nuevos:%d",
                  query_id, bloques_actuales, bloques_nuevos);

        if (bloques_nuevos > bloques_actuales)
        {
            int faltan = bloques_nuevos - bloques_actuales;
            log_debug(logger, "Query %d: Agregando %d bloques nuevos", query_id, faltan);

            char path_fisico_0[4096];
            sprintf(path_fisico_0, "%s/physical_blocks/block%04d.dat",
                    cfg->punto_montaje, 0);

            for (int i = 0; i < faltan; i++)
            {
                usleep(cfg->retardo_acceso_bloque * 1000);

                int num_bloque_logico = bloques_actuales + i;

                char nombre_bloque[32];
                sprintf(nombre_bloque, "block%06d", num_bloque_logico);

                char path_logico[4096];
                sprintf(path_logico, "%s/files/%s/%s/logical_blocks/%s.dat",
                        cfg->punto_montaje, file, tag, nombre_bloque);

                char path_dir[4096];
                sprintf(path_dir, "%s/files/%s/%s/logical_blocks",
                        cfg->punto_montaje, file, tag);
                mkdir_recursive(path_dir);

                unlink(path_logico);

                log_debug(logger, "Creando enlace lógico: %s -> %s",
                          path_logico, path_fisico_0);

                if (link(path_fisico_0, path_logico) != 0)
                {
                    log_error(logger,
                              "Error creando enlace: %s -> %s (errno=%d %s)",
                              path_logico, path_fisico_0,
                              errno, strerror(errno));
                    continue;
                }

                int *value = malloc(sizeof(int));
                *value = 0;
                list_add(bloques, value);
                log_info(logger, "STORAGE | TRUNCATE | Bloque lógico %s creado.", nombre_bloque);
            }
        }
        else if (bloques_nuevos < bloques_actuales)
        {
            int sobran = bloques_actuales - bloques_nuevos;
            log_debug(logger, "Query %d: Eliminando %d bloques", query_id, sobran);

            for (int i = 0; i < sobran; i++)
            {
                int idx = list_size(bloques) - 1;
                char nombre_bloque[32];
                sprintf(nombre_bloque, "%06d", idx);

                char path_logico[4096];
                sprintf(path_logico, "%s/files/%s/%s/logical_blocks/%s.dat",
                        cfg->punto_montaje, file, tag, nombre_bloque);

                unlink(path_logico);
                list_remove(bloques, idx);

                log_info(logger, "STORAGE | TRUNCATE | Bloque %s eliminado.", nombre_bloque);
            }
        }

        meta = fopen(path_metadata, "w");
        fprintf(meta, "TAMAÑO=%d\n", tamanio);
        fprintf(meta, "BLOCKS=[");

        for (int i = 0; i < list_size(bloques); i++)
        {
            fprintf(meta, "%d", *(int *)list_get(bloques, i));
            if (i < list_size(bloques) - 1)
                fprintf(meta, ",");
        }

        fprintf(meta, "]\n");
        fprintf(meta, "ESTADO=WORK_IN_PROGRESS\n");
        fclose(meta);

        list_destroy(bloques);

        char respuesta[64];
        sprintf(respuesta, "OK|TRUNCATE|%s|%s", file, tag);
        send(client_sock, respuesta, strlen(respuesta), 0);

        log_info(logger, "##%d - File Truncado %s:%s - Tamaño final: %d",
                 query_id, file, tag, tamanio);
        break;
    }
    case CMD_WRITE:
    {
        usleep(cfg->retardo_operacion * 1000);
        int num_bloque;
        char contenido[1024];
        sscanf(buffer_original, "%*[^|]|%d|%[^|]|%[^|]|%d|%[^\n]", &query_id, file, tag, &num_bloque, contenido);

        log_debug(logger, "Query %d: WRITE - File:%s Tag:%s Bloque:%d", query_id, file, tag, num_bloque);

        char path_metadata[1024];
        sprintf(path_metadata, "%s/files/%s/%s/metadata.config", cfg->punto_montaje, file, tag);

        struct stat info;
        if (stat(path_metadata, &info) != 0)
        {
            log_error(logger, "STORAGE | WRITE | Error: File:Tag %s:%s inexistente.", file, tag);
            send(client_sock, "ERR_TAG_NOT_FOUND", strlen("ERR_TAG_NOT_FOUND"), 0);
            break;
        }

        FILE *meta = fopen(path_metadata, "r");
        if (!meta)
        {
            log_error(logger, "STORAGE | WRITE | Error abriendo metadata %s", path_metadata);
            send(client_sock, "ERR_OPEN_METADATA", strlen("ERR_OPEN_METADATA"), 0);
            break;
        }

        char meta_buf[4096] = {0};
        char linea_aux[512];
        while (fgets(linea_aux, sizeof(linea_aux), meta))
            strncat(meta_buf, linea_aux, sizeof(meta_buf) - strlen(meta_buf) - 1);
        fclose(meta);

        char estado[32] = "WORK_IN_PROGRESS";
        char *p_estado = strstr(meta_buf, "ESTADO=");
        if (p_estado)
            sscanf(p_estado, "ESTADO=%31s", estado);

        if (strcmp(estado, "COMMITED") == 0)
        {
            log_error(logger, "STORAGE | WRITE | Error: intento de escritura sobre File:Tag COMMITED (%s:%s)", file, tag);
            send(client_sock, "ERR_TAG_COMMITED", strlen("ERR_TAG_COMMITED"), 0);
            break;
        }

        int bloques_fisicos[256];
        int cant_bloques = 0;
        char *p_blocks = strstr(meta_buf, "BLOCKS=[");
        if (p_blocks)
        {
            char *inicio = strchr(p_blocks, '[');
            if (inicio)
                inicio++;
            char *fin = inicio ? strchr(inicio, ']') : NULL;
            if (fin)
                *fin = '\0';

            if (inicio)
            {
                char *nl = strchr(inicio, '\n');
                if (nl)
                    *nl = '\0';

                char *token = strtok(inicio, ",");
                while (token)
                {
                    while (*token == ' ' || *token == '\t')
                        token++;
                    char *end = token + strlen(token) - 1;
                    while (end > token && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
                    {
                        *end = '\0';
                        end--;
                    }

                    if (strlen(token) > 0)
                    {
                        bloques_fisicos[cant_bloques++] = atoi(token);
                    }
                    token = strtok(NULL, ",");
                }
            }
        }

        if (num_bloque >= cant_bloques)
        {
            log_error(logger, "STORAGE | WRITE | Error: Bloque lógico fuera de rango. File:%s Tag:%s", file, tag);
            send(client_sock, "ERR_BLOCK_OUT_OF_RANGE", strlen("ERR_BLOCK_OUT_OF_RANGE"), 0);
            break;
        }

        int bloque_fisico = bloques_fisicos[num_bloque];
        char path_fisico[512];
        sprintf(path_fisico, "%s/physical_blocks/block%06d.dat", cfg->punto_montaje, bloque_fisico);

        log_debug(logger, "Query %d: Bloque lógico %d -> físico %d", query_id, num_bloque, bloque_fisico);

        struct stat st;
        stat(path_fisico, &st);

        if (st.st_nlink > 1)
        {
            log_debug(logger, "Query %d: Bloque físico %d tiene %ld enlaces (COW requerido)",
                      query_id, bloque_fisico, (long)st.st_nlink);
            log_info(logger, "COW | Bloque %d compartido → generando copia...", bloque_fisico);

            int nuevo_bloque = copiar_bloque_fisico(bloque_fisico, logger);
            if (nuevo_bloque < 0)
            {
                log_debug(logger, "Query %d: Error en COW, no se pudo copiar bloque %d", query_id, bloque_fisico);
                send(client_sock, "ERR_COW_COPY", strlen("ERR_COW_COPY"), 0);
                break;
            }

            log_debug(logger, "Query %d: COW exitoso, nuevo bloque físico: %d", query_id, nuevo_bloque);

            metadata_reemplazar_bloque(path_metadata, num_bloque, nuevo_bloque);

            bloque_fisico = nuevo_bloque;
            sprintf(path_fisico, "%s/physical_blocks/block%06d.dat", cfg->punto_montaje, nuevo_bloque);
        }

        usleep(cfg->retardo_operacion * 1000);
        usleep(cfg->retardo_acceso_bloque * 1000);

        FILE *fp = fopen(path_fisico, "r+");
        if (!fp)
        {
            log_error(logger, "STORAGE | WRITE | Error abriendo bloque físico %s", path_fisico);
            send(client_sock, "ERR_OPEN_BLOCK", strlen("ERR_OPEN_BLOCK"), 0);
            break;
        }

        size_t len = strlen(contenido);
        if (len > cfg->block_size)
            len = cfg->block_size;

        fwrite(contenido, 1, len, fp);
        if (len < cfg->block_size)
        {
            for (int i = len; i < cfg->block_size; i++)
                fputc('0', fp);
        }
        fflush(fp);
        fclose(fp);
        actualizar_blocks_hash_index(bloque_fisico, logger);

        char resp[256];
        sprintf(resp, "OK|WRITE|%s|%s|%d", file, tag, num_bloque);
        send(client_sock, resp, strlen(resp), 0);

        log_info(logger, "##%d - Bloque Lógico Escrito %s:%s - Número de Bloque: %d", query_id, file, tag, num_bloque);
        break;
    }
    case CMD_READ:
    {
        usleep(cfg->retardo_operacion * 1000);
        int num_bloque;
        sscanf(buffer_original, "%*[^|]|%d|%[^|]|%[^|]|%d", &query_id, file, tag, &num_bloque);

        log_debug(logger, "Query %d: READ - File:%s Tag:%s Bloque:%d", query_id, file, tag, num_bloque);

        char path_metadata[1024];
        sprintf(path_metadata, "%s/files/%s/%s/metadata.config", cfg->punto_montaje, file, tag);

        struct stat info;
        if (stat(path_metadata, &info) != 0)
        {
            log_error(logger, "STORAGE | READ | Error: File:Tag %s:%s inexistente.", file, tag);
            send(client_sock, "ERR_TAG_NOT_FOUND", strlen("ERR_TAG_NOT_FOUND"), 0);
            break;
        }

        FILE *meta = fopen(path_metadata, "r");
        if (!meta)
        {
            log_error(logger, "STORAGE | READ | Error abriendo metadata %s", path_metadata);
            send(client_sock, "ERR_OPEN_METADATA", strlen("ERR_OPEN_METADATA"), 0);
            break;
        }

        char meta_buf[4096] = {0};
        char linea_aux2[512];
        while (fgets(linea_aux2, sizeof(linea_aux2), meta))
            strncat(meta_buf, linea_aux2, sizeof(meta_buf) - strlen(meta_buf) - 1);
        fclose(meta);

        int bloques_fisicos[256];
        int cant_bloques = 0;
        char *p_blocks2 = strstr(meta_buf, "BLOCKS=[");
        if (p_blocks2)
        {
            char *inicio = strchr(p_blocks2, '[');
            if (inicio)
                inicio++;
            char *fin = inicio ? strchr(inicio, ']') : NULL;
            if (fin)
                *fin = '\0';

            if (inicio)
            {
                char *nl = strchr(inicio, '\n');
                if (nl)
                    *nl = '\0';

                char *token = strtok(inicio, ",");
                while (token)
                {
                    while (*token == ' ' || *token == '\t')
                        token++;
                    char *end = token + strlen(token) - 1;
                    while (end > token && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
                    {
                        *end = '\0';
                        end--;
                    }

                    if (strlen(token) > 0)
                    {
                        bloques_fisicos[cant_bloques++] = atoi(token);
                    }
                    token = strtok(NULL, ",");
                }
            }
        }

        if (num_bloque >= cant_bloques)
        {
            log_error(logger, "STORAGE | READ | Error: Bloque lógico fuera de rango. File:%s Tag:%s (pedido: %d, disponibles: %d)",
                      file, tag, num_bloque, cant_bloques);
            send(client_sock, "ERR_BLOCK_OUT_OF_RANGE", strlen("ERR_BLOCK_OUT_OF_RANGE"), 0);
            break;
        }

        int bloque_fisico = bloques_fisicos[num_bloque];
        char path_fisico[512];
        sprintf(path_fisico, "%s/physical_blocks/block%06d.dat", cfg->punto_montaje, bloque_fisico);

        log_debug(logger, "Query %d: Leyendo bloque lógico %d -> físico %d", query_id, num_bloque, bloque_fisico);

        usleep(cfg->retardo_operacion * 1000);
        usleep(cfg->retardo_acceso_bloque * 1000);

        FILE *fp = fopen(path_fisico, "r");
        if (!fp)
        {
            log_error(logger, "STORAGE | READ | Error abriendo bloque físico %s", path_fisico);
            send(client_sock, "ERR_OPEN_BLOCK", strlen("ERR_OPEN_BLOCK"), 0);
            break;
        }

        char *contenido = malloc(cfg->block_size + 1);
        size_t leidos = fread(contenido, 1, cfg->block_size, fp);
        fclose(fp);
        contenido[leidos] = '\0';

        char *respuesta = malloc(cfg->block_size + 128);
        sprintf(respuesta, "OK|READ|%s|%s|%d|%s", file, tag, num_bloque, contenido);
        send(client_sock, respuesta, strlen(respuesta), 0);

        log_info(logger, "##%d - Bloque Lógico Leído %s:%s - Número de Bloque: %d", query_id, file, tag, num_bloque);

        free(contenido);
        free(respuesta);
        break;
    }
    case CMD_TAG:
    {
        usleep(cfg->retardo_operacion * 1000);
        char tag_origen[64], tag_destino[64];
        sscanf(buffer_original, "%*[^|]|%d|%[^|]|%[^|]|%[^|]", &query_id, file, tag_origen, tag_destino);

        log_debug(logger, "Query %d: TAG - File:%s Origen:%s -> Destino:%s",
                  query_id, file, tag_origen, tag_destino);

        char path_origen[512], path_destino[512];
        sprintf(path_origen, "%s/files/%s/%s", cfg->punto_montaje, file, tag_origen);
        sprintf(path_destino, "%s/files/%s/%s", cfg->punto_montaje, file, tag_destino);

        struct stat info;
        if (stat(path_origen, &info) != 0)
        {
            log_error(logger, "STORAGE | TAG | Error: Tag origen inexistente. File:%s Tag:%s", file, tag_origen);
            send(client_sock, "ERR_TAG_ORIGIN_NOT_FOUND", 25, 0);
            break;
        }

        if (stat(path_destino, &info) == 0)
        {
            log_error(logger, "STORAGE | TAG | Error: Tag destino ya existe. File:%s Tag:%s", file, tag_destino);
            send(client_sock, "ERR_TAG_ALREADY_EXISTS", 23, 0);
            break;
        }

        mkdir_recursive(path_destino);
        char path_logical_origen[512], path_logical_destino[512];
        snprintf(path_logical_origen, sizeof(path_logical_origen), "%s/logical_blocks", path_origen);
        snprintf(path_logical_destino, sizeof(path_logical_destino), "%s/logical_blocks", path_destino);

        mkdir_recursive(path_logical_destino);

        char meta_origen[512], meta_destino[512];

        snprintf(meta_origen, sizeof(meta_origen), "%s/metadata.config", path_origen);
        snprintf(meta_destino, sizeof(meta_destino), "%s/metadata.config", path_destino);

        FILE *src = fopen(meta_origen, "r");
        FILE *dst = fopen(meta_destino, "w");
        if (!src || !dst)
        {
            log_error(logger, "STORAGE | TAG | Error al copiar metadata de %s a %s", meta_origen, meta_destino);
            send(client_sock, "ERR_COPY_METADATA", 18, 0);
            if (src)
                fclose(src);
            if (dst)
                fclose(dst);
            break;
        }

        char linea[256];
        while (fgets(linea, sizeof(linea), src))
        {
            if (strstr(linea, "ESTADO="))
                fprintf(dst, "ESTADO=WORK_IN_PROGRESS\n");
            else
                fputs(linea, dst);
        }
        fclose(src);
        fclose(dst);

        DIR *dir = opendir(path_logical_origen);
        if (!dir)
        {
            log_error(logger, "STORAGE | TAG | Error abriendo carpeta de bloques lógicos: %s", path_logical_origen);
            send(client_sock, "ERR_OPEN_LOGICAL_BLOCKS", 24, 0);
            break;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_name[0] == '.')
                continue;

            char origen_bloque[512], destino_bloque[512];

            snprintf(origen_bloque, sizeof(origen_bloque), "%s/%s", path_logical_origen, entry->d_name);
            snprintf(destino_bloque, sizeof(destino_bloque), "%s/%s", path_logical_destino, entry->d_name);

            char path_real[512];
            ssize_t len = readlink(origen_bloque, path_real, sizeof(path_real) - 1);
            if (len != -1)
            {
                path_real[len] = '\0';
                link(path_real, destino_bloque);
                log_info(logger, "STORAGE | TAG | Enlazado %s -> %s", destino_bloque, path_real);
            }
            else
            {
                log_error(logger, "STORAGE | TAG | Error leyendo enlace de bloque: %s", origen_bloque);
            }
        }
        closedir(dir);

        usleep(cfg->retardo_operacion * 1000);

        char respuesta[128];
        snprintf(respuesta, sizeof(respuesta), "OK|TAG|%s|%s|%s", file, tag_origen, tag_destino);

        send(client_sock, respuesta, strlen(respuesta), 0);

        log_info(logger, "##%d - Tag creado %s:%s a partir de %s", query_id, file, tag_destino, tag_origen);
        break;
    }
    case CMD_COMMIT:
    {
        sscanf(buffer_original, "%*[^|]|%d|%[^|]|%[^|]", &query_id, file, tag);

        log_debug(logger, "Query %d: COMMIT - File:%s Tag:%s", query_id, file, tag);

        char path_metadata[1024];
        sprintf(path_metadata, "%s/files/%s/%s/metadata.config", cfg->punto_montaje, file, tag);

        FILE *meta = fopen(path_metadata, "r+");
        if (!meta)
        {
            send(client_sock, "ERR_OPEN_METADATA", 18, 0);
            break;
        }

        int bloques[256];
        int cant = 0;
        char linea[512];
        while (fgets(linea, sizeof(linea), meta))
        {
            if (strstr(linea, "BLOCKS=["))
            {
                char *inicio = strchr(linea, '[') + 1;
                char *fin = strchr(linea, ']');
                *fin = '\0';
                char *token = strtok(inicio, ",");
                while (token)
                {
                    bloques[cant++] = atoi(token);
                    token = strtok(NULL, ",");
                }
                break;
            }
        }

        rewind(meta);
        char temp_path[1024];
        snprintf(temp_path, sizeof(temp_path), "%s/temp_meta.tmp", cfg->punto_montaje);
        FILE *tmp = fopen(temp_path, "w");
        rewind(meta);
        while (fgets(linea, sizeof(linea), meta))
        {
            if (strstr(linea, "ESTADO="))
                fprintf(tmp, "ESTADO=COMMITED\n");
            else
                fputs(linea, tmp);
        }
        fclose(meta);
        fclose(tmp);
        rename(temp_path, path_metadata);

        log_debug(logger, "Query %d: Actualizando hash de %d bloques", query_id, cant);

        for (int i = 0; i < cant; i++)
            actualizar_blocks_hash_index(bloques[i], logger);

        send(client_sock, "OK|COMMIT", 9, 0);
        log_info(logger, "##%d - COMMIT finalizado %s:%s (%d bloques verificados)", query_id, file, tag, cant);
        break;
    }
    case CMD_DELETE:
    {
        usleep(cfg->retardo_operacion * 1000);
        sscanf(buffer_original, "%*[^|]|%d|%[^|]|%[^|]", &query_id, file, tag);

        log_debug(logger, "Query %d: DELETE - File:%s Tag:%s", query_id, file, tag);

        if (strcmp(file, "initial_file") == 0 && strcmp(tag, "BASE") == 0)
        {
            send(client_sock, "ERR_CANNOT_DELETE_INITIAL_FILE", 31, 0);
            log_error(logger, "STORAGE | DELETE | Intento de borrar initial_file:BASE (protegido)");
            break;
        }

        char path_tag[1024];
        sprintf(path_tag, "%s/files/%s/%s", cfg->punto_montaje, file, tag);

        struct stat info;
        if (stat(path_tag, &info) != 0)
        {
            send(client_sock, "ERR_TAG_NOT_FOUND", 18, 0);
            log_error(logger, "STORAGE | DELETE | Tag inexistente %s:%s", file, tag);
            break;
        }

        char path_metadata[1024];
        snprintf(path_metadata, sizeof(path_metadata), "%s/metadata.config", path_tag);

        FILE *meta = fopen(path_metadata, "r");
        if (!meta)
        {
            send(client_sock, "ERR_OPEN_METADATA", 18, 0);
            break;
        }

        int bloques[256], cant = 0;
        char linea[256];
        while (fgets(linea, sizeof(linea), meta))
        {
            if (strstr(linea, "BLOCKS=["))
            {
                char *inicio = strchr(linea, '[') + 1;
                char *fin = strchr(linea, ']');
                *fin = '\0';
                char *token = strtok(inicio, ",");
                while (token)
                {
                    bloques[cant++] = atoi(token);
                    token = strtok(NULL, ",");
                }
                break;
            }
        }
        fclose(meta);

        char path_logical_block[512];
        log_debug(logger, "Query %d: Liberando %d bloques de %s:%s", query_id, cant, file, tag);

        for (int i = 0; i < cant; i++)
        {
            sprintf(path_logical_block, "%s/files/%s/%s/logical_blocks/%06d.dat", cfg->punto_montaje, file, tag, i);

            unlink(path_logical_block);

            char path_fisico[512];
            sprintf(path_fisico, "%s/physical_blocks/block%06d.dat", cfg->punto_montaje, bloques[i]);

            struct stat st;
            stat(path_fisico, &st);

            if (st.st_nlink == 1)
            {
                marcar_bloque_libre(bloques[i], logger);
                log_info(logger, "STORAGE | DELETE | Bloque físico %d liberado", bloques[i]);
            }
        }

        char cmd_rm[1024];
        snprintf(cmd_rm, sizeof(cmd_rm), "rm -rf %s", path_tag);
        system(cmd_rm);

        char path_file[1024];
        sprintf(path_file, "%s/files/%s", cfg->punto_montaje, file);
        DIR *d = opendir(path_file);
        int solo_dots = 1;
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL)
            if (ent->d_name[0] != '.')
                solo_dots = 0;
        closedir(d);

        if (solo_dots)
        {
            snprintf(cmd_rm, sizeof(cmd_rm), "rm -rf %s", path_file);
            system(cmd_rm);
            log_info(logger, "STORAGE | DELETE | File %s eliminado por no tener más tags", file);
        }

        char resp[256];
        snprintf(resp, sizeof(resp), "OK|DELETE|%s|%s", file, tag);
        send(client_sock, resp, strlen(resp), 0);

        log_info(logger, "##%d - Tag Eliminado %s:%s", query_id, file, tag);
        break;
    }
    case CMD_END:
    {
        log_debug(logger, "Comando END recibido");
        send(client_sock, "OK", 2, 0);
        break;
    }
    case CMD_UNKNOWN:
    default:
    {
        log_debug(logger, "Comando desconocido recibido: tipo %d", cmd);
        send(client_sock, "ERR_UNKNOWN_COMMAND", 20, 0);
        break;
    }
    }

    log_debug(logger, "Cerrando conexión (socket: %d)", client_sock);
    close(client_sock);

    pthread_mutex_lock(&mutex_workers);
    cantidad_workers--;
    log_info(logger, "##Se desconecta el Worker %d - Cantidad de Workers: %d", worker_id, cantidad_workers);
    pthread_mutex_unlock(&mutex_workers);

    return NULL;
}

int actualizar_metadata(char *op, char path_metadata[4096])
{
    FILE *fp = fopen(path_metadata, "r");
    if (!fp)
        return -1;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char buffer[4096] = {0};
    int bloque_ret = -1;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (strstr(line, "BLOCKS=["))
        {
            char *inicio = strchr(line, '[');
            char *fin = strchr(line, ']');
            *fin = '\0';
            char *bloques = inicio + 1;

            if (strcmp(op, "AGREGAR") == 0)
            {
                char nueva_linea[512];
                snprintf(nueva_linea, sizeof(nueva_linea), "BLOCKS=[%s,0]\n", bloques);
                strcat(buffer, nueva_linea);
                bloque_ret = 0;
            }
            else if (strcmp(op, "SACAR") == 0)
            {
                char *ultima_coma = strrchr(bloques, ',');
                if (ultima_coma)
                {
                    char *ultimo_bloque = ultima_coma + 1;
                    bloque_ret = atoi(ultimo_bloque);
                    *ultima_coma = '\0';
                }
                else if (*bloques)
                {
                    bloque_ret = atoi(bloques);
                    *bloques = '\0';
                }
                snprintf(line, len, "BLOCKS=[%s]\n", bloques);
                strcat(buffer, line);
            }
            continue;
        }
        strcat(buffer, line);
    }

    free(line);
    fclose(fp);

    pthread_mutex_lock(&mutex_meta);
    fp = fopen(path_metadata, "w");
    if (!fp)
    {
        pthread_mutex_unlock(&mutex_meta);
        return -1;
    }
    fputs(buffer, fp);
    fclose(fp);
    pthread_mutex_unlock(&mutex_meta);

    return bloque_ret;
}

void sacar_bloque_filetag(int client_sock, char file[64], char tag[64], char path_metadata[4096], char path_logical_block[4096], t_log *logger)
{
    char *nombre_bloque = strrchr(path_logical_block, '/');
    if (!nombre_bloque)
    {
        log_error(logger, "STORAGE | TRUNCATE | Error al buscar numero de bloque logico. File:%s Tag:%s", file, tag);
        char msg[512] = "";
        sprintf(msg, "ERR_PATH_LOGICAL");
        send(client_sock, msg, strlen(msg), 0);
        return;
    }
    int num_bloque_fisico = actualizar_metadata("SACAR", path_metadata);
    if (num_bloque_fisico == -1)
    {
        log_error(logger, "STORAGE | TRUNCATE | Error al modificar metadata. File:%s Tag:%s", file, tag);
        char msg[512] = "";
        sprintf(msg, "ERR_MODIFY_METADATA");
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    if (unlink(path_logical_block) == 0)
    {
        log_info(logger, "STORAGE | Bloque lógico eliminado: %s", path_logical_block);
    }
    else
    {
        log_error(logger, "STORAGE | Error eliminando bloque lógico: %s", path_logical_block);
        char msg[512] = "";
        sprintf(msg, "ERR_UNLINK_LOGICAL");
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    char path_fisico[512];
    snprintf(path_fisico, sizeof(path_fisico), "%s/physical_blocks/block%06d.dat", cfg->punto_montaje, num_bloque_fisico);

    struct stat info;
    if (stat(path_fisico, &info) == 0)
    {
        if (info.st_nlink < 1)
        {
            marcar_bloque_libre(num_bloque_fisico, logger);
            log_info(logger, "STORAGE | TRUNCATE | Bloque fisico %d liberado (ultimo enlace eliminado)", num_bloque_fisico);
        }
    }
}

void marcar_bloque_libre(int num_bloque, t_log *logger)
{
    char path_bitmap[512];
    snprintf(path_bitmap, sizeof(path_bitmap), "%s/bitmap.bin", cfg->punto_montaje);
    int bitmap_fd = open(path_bitmap, O_RDWR, 0666);
    if (bitmap_fd < 0)
    {
        log_error(logger, "STORAGE | No se pudo abrir el bitmap.");
        return;
    }

    int tamanio_fd = (cfg->fs_size / cfg->block_size) / 8;
    unsigned char *bitmap_map = mmap(NULL, tamanio_fd, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_fd, 0);

    if (bitmap_map == MAP_FAILED)
    {
        log_error(logger, "STORAGE | Error al mapear bitmap.");
        close(bitmap_fd);
        return;
    }

    int byte_index = num_bloque / 8;
    if (byte_index >= tamanio_fd)
    {
        log_error(logger, "Bloque fuera de rango del bitmap.");
        munmap(bitmap_map, tamanio_fd);
        close(bitmap_fd);
        return;
    }
    int bit_index = num_bloque % 8;

    pthread_mutex_lock(&mutex_bitmap);
    usleep(cfg->retardo_acceso_bloque * 1000);
    bitmap_map[byte_index] &= ~(1 << bit_index);
    msync(bitmap_map, tamanio_fd, MS_SYNC);
    pthread_mutex_unlock(&mutex_bitmap);

    munmap(bitmap_map, tamanio_fd);
    log_info(logger, "STORAGE | Bitmap actualizado: bloque %d libre.", num_bloque);
}

int asignar_bloque_libre(t_log *logger)
{
    char path_bitmap[512];
    snprintf(path_bitmap, sizeof(path_bitmap), "%s/bitmap.bin", cfg->punto_montaje);
    int bitmap_fd = open(path_bitmap, O_RDWR, 0666);
    if (bitmap_fd < 0)
    {
        log_error(logger, "STORAGE | No se pudo abrir el bitmap para asignar bloque libre.");
        return -1;
    }

    int total_bloques = cfg->fs_size / cfg->block_size;
    int tamanio_fd = total_bloques / 8;
    unsigned char *bitmap_map = mmap(NULL, tamanio_fd, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_fd, 0);

    if (bitmap_map == MAP_FAILED)
    {
        log_error(logger, "STORAGE | Error al mapear bitmap.");
        close(bitmap_fd);
        return -1;
    }

    int bloque_encontrado = -1;

    pthread_mutex_lock(&mutex_bitmap);
    for (int byte_index = 0; byte_index < tamanio_fd; byte_index++)
    {
        usleep(cfg->retardo_acceso_bloque * 1000);
        if (bitmap_map[byte_index] != 0xFF)
        {
            for (int bit_index = 0; bit_index < 8; bit_index++)
            {
                if (!(bitmap_map[byte_index] & (1 << bit_index)))
                {
                    bitmap_map[byte_index] |= (1 << bit_index);
                    bloque_encontrado = byte_index * 8 + bit_index;
                    msync(bitmap_map, tamanio_fd, MS_SYNC);
                    log_info(logger, "STORAGE | Bloque %d asignado.", bloque_encontrado);
                    break;
                }
            }
        }
        if (bloque_encontrado != -1)
            break;
    }
    pthread_mutex_unlock(&mutex_bitmap);

    munmap(bitmap_map, tamanio_fd);
    close(bitmap_fd);

    if (bloque_encontrado == -1)
    {
        log_error(logger, "STORAGE | No hay bloques libres disponibles en el bitmap.");
    }

    return bloque_encontrado;
}

void actualizar_blocks_hash_index(int num_bloque, t_log *logger)
{
    char path_block[512];
    sprintf(path_block, "%s/physical_blocks/block%06d.dat",
            cfg->punto_montaje, num_bloque);

    FILE *f = fopen(path_block, "rb");
    if (!f)
    {
        log_error(logger, "HASH | No se pudo abrir bloque físico %s", path_block);
        return;
    }

    unsigned char *data = malloc(cfg->block_size);
    size_t leidos = fread(data, 1, cfg->block_size, f);
    fclose(f);

    if (leidos == 0)
    {
        log_error(logger, "HASH | Bloque vacío o error leyendo %s", path_block);
        free(data);
        return;
    }

    char *hash_string = crypto_md5(data, leidos);
    free(data);

    if (hash_string == NULL)
    {
        log_error(logger, "HASH | crypto_md5 falló en bloque %d", num_bloque);
        return;
    }

    pthread_mutex_lock(&mutex_meta);

    char index_path[1024];
    sprintf(index_path, "%s/blocks_hash_index.config", cfg->punto_montaje);

    FILE *index = fopen(index_path, "r");
    bool exists = (index != NULL);

    char temp_path[1024];
    sprintf(temp_path, "%s/blocks_hash_index.tmp", cfg->punto_montaje);

    FILE *temp = fopen(temp_path, "w");
    if (!temp)
    {
        if (index)
            fclose(index);
        pthread_mutex_unlock(&mutex_meta);
        log_error(logger, "HASH | No se pudo crear archivo temporal de HASH");
        free(hash_string);
        return;
    }

    int updated = 0;

    if (exists)
    {
        char line[256];
        while (fgets(line, sizeof(line), index))
        {
            char existing_hash[64];
            char existing_block_text[64];

            if (sscanf(line, "%63[^=]=%63s", existing_hash, existing_block_text) == 2)
            {
                int blk;
                if (sscanf(existing_block_text, "block%d", &blk) == 1 && blk == num_bloque)
                {
                    fprintf(temp, "%s=block%06d\n", hash_string, num_bloque);
                    updated = 1;
                }
                else
                {
                    fputs(line, temp);
                }
            }
            else
            {
                fputs(line, temp);
            }
        }

        fclose(index);
    }

    if (!updated)
        fprintf(temp, "%s=block%06d\n", hash_string, num_bloque);

    fclose(temp);

    rename(temp_path, index_path);

    pthread_mutex_unlock(&mutex_meta);

    log_info(logger,
             "HASH | Actualizado bloque %d -> %s",
             num_bloque, hash_string);

    free(hash_string);
}

int actualizar_metadata_agregar(const char *path_metadata, int bloque_fisico)
{
    FILE *fp = fopen(path_metadata, "r");
    if (!fp)
        return -1;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char buffer[8192] = {0};

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (strstr(line, "BLOCKS=["))
        {
            char *ini = strchr(line, '[') + 1;
            char *fin = strchr(line, ']');
            *fin = '\0';

            if (strlen(ini) == 0)
            {
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                         "BLOCKS=[%d]\n", bloque_fisico);
            }
            else
            {
                snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer),
                         "BLOCKS=[%s,%d]\n", ini, bloque_fisico);
            }
        }
        else
        {
            strncat(buffer, line, sizeof(buffer) - strlen(buffer) - 1);
        }
    }

    free(line);
    fclose(fp);

    pthread_mutex_lock(&mutex_meta);
    fp = fopen(path_metadata, "w");
    if (!fp)
    {
        pthread_mutex_unlock(&mutex_meta);
        return -1;
    }

    fputs(buffer, fp);
    fclose(fp);
    pthread_mutex_unlock(&mutex_meta);

    return 0;
}

int copiar_bloque_fisico(int bloque_original, t_log *logger)
{
    int bloque_nuevo = asignar_bloque_libre(logger);
    if (bloque_nuevo < 0)
        return -1;

    char src_path[512], dst_path[512];
    sprintf(src_path, "%s/physical_blocks/block%06d.dat", cfg->punto_montaje, bloque_original);
    sprintf(dst_path, "%s/physical_blocks/block%06d.dat", cfg->punto_montaje, bloque_nuevo);

    FILE *src = fopen(src_path, "rb");
    FILE *dst = fopen(dst_path, "wb");
    if (!src || !dst)
    {
        if (src)
            fclose(src);
        if (dst)
            fclose(dst);
        marcar_bloque_libre(bloque_nuevo, logger);
        return -1;
    }

    unsigned char buffer[cfg->block_size];
    fread(buffer, 1, cfg->block_size, src);
    fwrite(buffer, 1, cfg->block_size, dst);

    fclose(src);
    fclose(dst);

    log_info(logger, "COW | Copiado bloque %d → %d", bloque_original, bloque_nuevo);
    return bloque_nuevo;
}

void metadata_reemplazar_bloque(const char *path_metadata, int num_bloque_logico, int nuevo_bloque)
{
    FILE *fp = fopen(path_metadata, "r");
    if (!fp)
        return;

    char buffer[8192] = {0};
    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fp) != -1)
    {
        if (strstr(line, "BLOCKS=["))
        {
            char lista[4096];
            strcpy(lista, strchr(line, '[') + 1);
            *strchr(lista, ']') = '\0';

            int bloques[256], cant = 0;
            char *token = strtok(lista, ",");
            while (token)
            {
                bloques[cant++] = atoi(token);
                token = strtok(NULL, ",");
            }

            bloques[num_bloque_logico] = nuevo_bloque;

            strcat(buffer, "BLOCKS=[");
            for (int i = 0; i < cant; i++)
            {
                char tmp[32];
                sprintf(tmp, (i == cant - 1) ? "%d" : "%d,", bloques[i]);
                strcat(buffer, tmp);
            }
            strcat(buffer, "]\n");
        }
        else
            strcat(buffer, line);
    }

    free(line);
    fclose(fp);

    pthread_mutex_lock(&mutex_meta);
    fp = fopen(path_metadata, "w");
    fputs(buffer, fp);
    fclose(fp);
    pthread_mutex_unlock(&mutex_meta);
}

void marcar_bloque_ocupado(int num_bloque, t_log *logger)
{
    char path_bitmap[512];
    snprintf(path_bitmap, sizeof(path_bitmap), "%s/bitmap.bin", cfg->punto_montaje);
    int bitmap_fd = open(path_bitmap, O_RDWR, 0666);
    if (bitmap_fd < 0)
    {
        log_error(logger, "STORAGE | No se pudo abrir el bitmap.");
        return;
    }

    int tamanio_fd = (cfg->fs_size / cfg->block_size) / 8;
    unsigned char *bitmap_map = mmap(NULL, tamanio_fd, PROT_READ | PROT_WRITE, MAP_SHARED, bitmap_fd, 0);

    if (bitmap_map == MAP_FAILED)
    {
        log_error(logger, "STORAGE | Error al mapear bitmap.");
        close(bitmap_fd);
        return;
    }

    int byte_index = num_bloque / 8;
    int bit_index = num_bloque % 8;

    pthread_mutex_lock(&mutex_bitmap);
    usleep(cfg->retardo_acceso_bloque * 1000);
    bitmap_map[byte_index] |= (1 << bit_index);
    msync(bitmap_map, tamanio_fd, MS_SYNC);
    pthread_mutex_unlock(&mutex_bitmap);

    munmap(bitmap_map, tamanio_fd);
    close(bitmap_fd);

    log_info(logger, "STORAGE | Bitmap actualizado: bloque %d marcado como OCUPADO.", num_bloque);
}

int mkdir_recursive(const char *path)
{
    char temp[512];
    snprintf(temp, sizeof(temp), "%s", path);

    size_t len = strlen(temp);
    if (temp[len - 1] == '/')
        temp[len - 1] = '\0';

    for (char *p = temp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            mkdir(temp, 0777);
            *p = '/';
        }
    }

    return mkdir(temp, 0777);
}