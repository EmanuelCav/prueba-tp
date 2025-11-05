#include "../include/storage_worker.h"

extern t_storage_config *cfg;

extern t_log *logger;

int cantidad_workers = 0;
pthread_mutex_t mutex_workers = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_meta = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_bitmap = PTHREAD_MUTEX_INITIALIZER;

comando_t parse_comando(const char *cmd)
{
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
    if (strcmp(cmd, "END") == 0)
        return CMD_END;
    return CMD_UNKNOWN;
}

void *manejar_worker(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[256];
    int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0)
    {
        close(client_sock);
        return NULL;
    }
    buffer[bytes] = '\0';

    int worker_id = atoi(buffer);

    pthread_mutex_lock(&mutex_workers);
    cantidad_workers++;
    log_info(logger, "##Se conecta el Worker %d - Cantidad de Workers: %d", worker_id, cantidad_workers);
    pthread_mutex_unlock(&mutex_workers);

    comando_t cmd = parse_comando(buffer);

    char file[64], tag[64];
    int tamanio, query_id;

    switch (cmd)
    {
    case CMD_GET_BLOCK_SIZE:
    {

        char resp[32];
        sprintf(resp, "%d", cfg->block_size);
        send(client_sock, resp, strlen(resp), 0);
        log_info(logger, "## Worker solicitó tamaño de bloque, enviado %s", resp);
        break;
    }
    case CMD_GET_FS_SIZE:
    {
        char resp[32];
        sprintf(resp, "%d", cfg->fs_size);
        send(client_sock, resp, strlen(resp), 0);
        log_info(logger, "## Worker solicitó tamaño de FS, enviado %s", resp);
        break;
    }
    case CMD_CREATE:
    {
        usleep(cfg->retardo_operacion * 1000);
        struct stat info;
        sscanf(buffer, "%*[^|]|%d|%[^|]|%[^|]|%d", &query_id, file, tag, &tamanio);

        char path_tag[4096];
        snprintf(path_tag, sizeof(path_tag), "./files/%s/%s", file, tag);
        if (stat(path_tag, &info) == 0 && S_ISDIR(info.st_mode))
        {
            log_error(logger, "STORAGE | CREATE | Error: File:Tag %s:%s ya existe.", file, tag);
            char msg[64];
            sprintf(msg, "ERR_FILE_TAG_ALREADY_EXIST");
            send(client_sock, msg, strlen(msg), 0);
            break;
        }

        char path_file[4096];
        snprintf(path_file, sizeof(path_file), "./files/%s", file);
        mkdir(path_file, 0777);
        mkdir(path_tag, 0777);

        char path_logical[4096];
        snprintf(path_logical, sizeof(path_logical), "./files/%s/%s/logical_blocks", file, tag);
        mkdir(path_logical, 0777);

        char path_metadata[4096];
        snprintf(path_metadata, sizeof(path_metadata), "./files/%s/%s/metadata.config", file, tag);

        FILE *meta = fopen(path_metadata, "w");
        if (meta)
        {
            fprintf(meta, "TAMAÑO=%d\n", tamanio);
            fprintf(meta, "BLOCKS=[0]\n");
            fprintf(meta, "ESTADO=WORK_IN_PROGRESS\n");
            fclose(meta);
            log_info(logger, "Archivo metadata creado: %s", path_metadata);
        }
        else
        {
            log_error(logger, "Error creando metadata: %s", path_metadata);
            char respuesta[512];
            sprintf(respuesta, "ERR_CREATION_METADATA");
            send(client_sock, respuesta, strlen(respuesta), 0);
        }
        char respuesta[512];
        sprintf(respuesta, "OK|CREATE|%s|%s", file, tag);
        send(client_sock, respuesta, strlen(respuesta), 0);
        log_info(logger, "##%d- File Creado %s:%s", query_id, file, tag);
        break;
    }
    case CMD_TRUNCATE:
    {
        usleep(cfg->retardo_operacion * 1000);
        sscanf(buffer, "%*[^|]|%d|%[^|]|%[^|]|%d", &query_id, file, tag, &tamanio);

        struct stat info;
        char path_filetag[4096];
        sprintf(path_filetag, "./files/%s/%s", file, tag);
        if (!(stat(path_filetag, &info) == 0 && S_ISDIR(info.st_mode)))
        {
            log_error(logger, "STORAGE | TRUNCATE | Error: File:Tag %s:%s inexistente.", file, tag);
            char msg[512] = "";
            sprintf(msg, "ERR_TAG_NOT_FOUND");
            send(client_sock, msg, strlen(msg), 0);
            break;
        }

        int tamanio_actual = 0;
        char estado[32] = "";
        char path_metadata[4096];
        sprintf(path_metadata, "./files/%s/%s/metadata.config", file, tag);

        FILE *meta = fopen(path_metadata, "r");
        if (meta)
        {
            char linea[128];
            while (fgets(linea, sizeof(linea), meta))
            {
                if (sscanf(linea, "TAMAÑO=%d", &tamanio_actual) == 1)
                    continue;
                if (sscanf(linea, "ESTADO=%s", estado) == 1)
                    break;
            }
            fclose(meta);
        }
        if (strcmp(estado, "COMMITED") == 0)
        {
            log_error(logger, "STORAGE | TRUNCATE | Error, intento de truncado de filetag en estado COMMITED. File:%s Tag:%s", file, tag);
            char msg[512] = "";
            sprintf(msg, "ERR_TAG_COMMITED");
            send(client_sock, msg, strlen(msg), 0);
            break;
        }

        int cant_bloques = 0;
        int bloques_actuales = tamanio_actual / cfg->block_size;
        if (tamanio > tamanio_actual)
        {
            cant_bloques = (tamanio - tamanio_actual) / cfg->block_size;

            int num_bloque = 0;
            char bloque_nombre[16];

            for (int i = 0; i < cant_bloques; i++)
            {
                usleep(cfg->retardo_acceso_bloque * 1000);
                int bloque_fisico = asignar_bloque_libre(logger);
                if (bloque_fisico == -1)
                {
                    log_error(logger, "STORAGE | TRUNCATE | No hay bloques libres disponibles.");
                    send(client_sock, "ERR_NO_FREE_BLOCKS", 19, 0);
                    break;
                }

                char path_fisico[512];
                sprintf(path_fisico, "./physical_blocks/block%06d.dat", bloque_fisico);

                if (access(path_fisico, F_OK) != 0)
                {
                    FILE *nuevo_bloque = fopen(path_fisico, "w");
                    if (!nuevo_bloque)
                    {
                        log_error(logger, "STORAGE | TRUNCATE | Error creando bloque físico %s", path_fisico);
                        send(client_sock, "ERR_CREATE_BLOCK", 17, 0);
                        marcar_bloque_libre(bloque_fisico, logger);
                        break;
                    }
                    for (int j = 0; j < cfg->block_size; j++)
                        fputc('0', nuevo_bloque);
                    fclose(nuevo_bloque);
                }

                num_bloque = bloques_actuales + i;
                sprintf(bloque_nombre, "%06d", num_bloque);

                char path_logical_block[4096];
                snprintf(path_logical_block, sizeof(path_logical_block),
                         "./files/%s/%s/logical_blocks/%s.dat", file, tag, bloque_nombre);

                if (link(path_fisico, path_logical_block) != 0)
                {
                    log_error(logger, "STORAGE | TRUNCATE | Error creando enlace lógico: %s -> %s",
                              path_logical_block, path_fisico);
                    marcar_bloque_libre(bloque_fisico, logger);
                    continue;
                }

                actualizar_metadata_agregar(path_metadata, bloque_fisico);

                log_info(logger, "STORAGE | TRUNCATE | Bloque lógico %s vinculado a físico %d", bloque_nombre, bloque_fisico);
            }
        }
        else
        {
            cant_bloques = (tamanio_actual - tamanio) / cfg->block_size;

            int num_bloque = 0;
            char bloque_nombre[7];
            for (int i = 0; i < cant_bloques; i++)
            {
                num_bloque = bloques_actuales - i;
                sprintf(bloque_nombre, "%06d", num_bloque);
                char path_logical_block[4096];
                snprintf(path_logical_block, sizeof(path_logical_block), "./files/%s/%s/logical_blocks/%s.dat", file, tag, bloque_nombre);
                sacar_bloque_filetag(client_sock, file, tag, path_metadata, path_logical_block, logger);
            }
        }

        char respuesta[512];
        sprintf(respuesta, "OK|TRUNCATE|%s|%s", file, tag);
        send(client_sock, respuesta, strlen(respuesta), 0);
        log_info(logger, "##%d - File Truncado %s:%s - Tamaño: %d", query_id, file, tag, tamanio);
        break;
    }
    case CMD_WRITE:
    {
        usleep(cfg->retardo_operacion * 1000);
        int num_bloque;
        char contenido[1024];
        sscanf(buffer, "%*[^|]|%d|%[^|]|%[^|]|%d|%[^\n]", &query_id, file, tag, &num_bloque, contenido);

        char path_metadata[1024];
        sprintf(path_metadata, "./files/%s/%s/metadata.config", file, tag);

        struct stat info;
        if (stat(path_metadata, &info) != 0)
        {
            log_error(logger, "STORAGE | WRITE | Error: File:Tag %s:%s inexistente.", file, tag);
            char msg[128] = "ERR_TAG_NOT_FOUND";
            send(client_sock, msg, strlen(msg), 0);
            break;
        }

        FILE *meta = fopen(path_metadata, "r");
        if (!meta)
        {
            log_error(logger, "STORAGE | WRITE | Error abriendo metadata %s", path_metadata);
            send(client_sock, "ERR_OPEN_METADATA", 18, 0);
            break;
        }

        char estado[32] = "";
        char bloques_linea[512] = "";
        while (fgets(bloques_linea, sizeof(bloques_linea), meta))
        {
            if (sscanf(bloques_linea, "ESTADO=%s", estado) == 1)
                break;
        }
        rewind(meta);

        if (strcmp(estado, "COMMITED") == 0)
        {
            log_error(logger, "STORAGE | WRITE | Error: intento de escritura sobre File:Tag COMMITED (%s:%s)", file, tag);
            char msg[128] = "ERR_TAG_COMMITED";
            send(client_sock, msg, strlen(msg), 0);
            fclose(meta);
            break;
        }

        int bloques_fisicos[256];
        int cant_bloques = 0;
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
                    bloques_fisicos[cant_bloques++] = atoi(token);
                    token = strtok(NULL, ",");
                }
                break;
            }
        }
        fclose(meta);

        if (num_bloque >= cant_bloques)
        {
            log_error(logger, "STORAGE | WRITE | Error: Bloque lógico fuera de rango. File:%s Tag:%s", file, tag);
            send(client_sock, "ERR_BLOCK_OUT_OF_RANGE", 24, 0);
            break;
        }

        int bloque_fisico = bloques_fisicos[num_bloque];
        char path_fisico[512];
        sprintf(path_fisico, "./physical_blocks/block%06d.dat", bloque_fisico);

        struct stat st;
        stat(path_fisico, &st);

        if (st.st_nlink > 1)
        {
            log_info(logger, "COW | Bloque %d compartido → generando copia...", bloque_fisico);

            int nuevo_bloque = copiar_bloque_fisico(bloque_fisico, logger);
            if (nuevo_bloque < 0)
            {
                send(client_sock, "ERR_COW_COPY", 12, 0);
                break;
            }

            metadata_reemplazar_bloque(path_metadata, num_bloque, nuevo_bloque);

            bloque_fisico = nuevo_bloque;
            sprintf(path_fisico, "./physical_blocks/block%06d.dat", nuevo_bloque);
        }

        usleep(cfg->retardo_operacion * 1000);
        usleep(cfg->retardo_acceso_bloque * 1000);

        FILE *fp = fopen(path_fisico, "r+");
        if (!fp)
        {
            log_error(logger, "STORAGE | WRITE | Error abriendo bloque físico %s", path_fisico);
            send(client_sock, "ERR_OPEN_BLOCK", 15, 0);
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
        sscanf(buffer, "%*[^|]|%d|%[^|]|%[^|]|%d", &query_id, file, tag, &num_bloque);

        char path_metadata[1024];
        sprintf(path_metadata, "./files/%s/%s/metadata.config", file, tag);

        struct stat info;
        if (stat(path_metadata, &info) != 0)
        {
            log_error(logger, "STORAGE | READ | Error: File:Tag %s:%s inexistente.", file, tag);
            send(client_sock, "ERR_TAG_NOT_FOUND", 18, 0);
            break;
        }

        FILE *meta = fopen(path_metadata, "r");
        if (!meta)
        {
            log_error(logger, "STORAGE | READ | Error abriendo metadata %s", path_metadata);
            send(client_sock, "ERR_OPEN_METADATA", 18, 0);
            break;
        }

        int bloques_fisicos[256];
        int cant_bloques = 0;
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
                    bloques_fisicos[cant_bloques++] = atoi(token);
                    token = strtok(NULL, ",");
                }
                break;
            }
        }
        fclose(meta);

        if (num_bloque >= cant_bloques)
        {
            log_error(logger, "STORAGE | READ | Error: Bloque lógico fuera de rango. File:%s Tag:%s", file, tag);
            send(client_sock, "ERR_BLOCK_OUT_OF_RANGE", 24, 0);
            break;
        }

        int bloque_fisico = bloques_fisicos[num_bloque];
        char path_fisico[512];
        sprintf(path_fisico, "./physical_blocks/block%06d.dat", bloque_fisico);

        usleep(cfg->retardo_operacion * 1000);
        usleep(cfg->retardo_acceso_bloque * 1000);

        FILE *fp = fopen(path_fisico, "r");
        if (!fp)
        {
            log_error(logger, "STORAGE | READ | Error abriendo bloque físico %s", path_fisico);
            send(client_sock, "ERR_OPEN_BLOCK", 15, 0);
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
        sscanf(buffer, "%*[^|]|%d|%[^|]|%[^|]|%[^|]", &query_id, file, tag_origen, tag_destino);

        char path_origen[512], path_destino[512];
        sprintf(path_origen, "./files/%s/%s", file, tag_origen);
        sprintf(path_destino, "./files/%s/%s", file, tag_destino);

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

        mkdir(path_destino, 0777);
        char path_logical_origen[512], path_logical_destino[512];
        sprintf(path_logical_origen, "%s/logical_blocks", path_origen);
        sprintf(path_logical_destino, "%s/logical_blocks", path_destino);
        mkdir(path_logical_destino, 0777);

        char meta_origen[512], meta_destino[512];
        sprintf(meta_origen, "%s/metadata.config", path_origen);
        sprintf(meta_destino, "%s/metadata.config", path_destino);

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
            sprintf(origen_bloque, "%s/%s", path_logical_origen, entry->d_name);
            sprintf(destino_bloque, "%s/%s", path_logical_destino, entry->d_name);

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
        sprintf(respuesta, "OK|TAG|%s|%s|%s", file, tag_origen, tag_destino);
        send(client_sock, respuesta, strlen(respuesta), 0);

        log_info(logger, "##%d - Tag creado %s:%s a partir de %s", query_id, file, tag_destino, tag_origen);
        break;
    }
    case CMD_COMMIT:
    {
        sscanf(buffer, "%*[^|]|%d|%[^|]|%[^|]", &query_id, file, tag);

        char path_metadata[1024];
        sprintf(path_metadata, "./files/%s/%s/metadata.config", file, tag);

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
        FILE *tmp = fopen("./temp_meta.tmp", "w");
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
        rename("./temp_meta.tmp", path_metadata);

        for (int i = 0; i < cant; i++)
            actualizar_blocks_hash_index(bloques[i], logger);

        send(client_sock, "OK|COMMIT", 9, 0);
        log_info(logger, "##%d - COMMIT finalizado %s:%s (%d bloques verificados)", query_id, file, tag, cant);
        break;
    }
    case CMD_DELETE:
    {
        usleep(cfg->retardo_operacion * 1000);
        sscanf(buffer, "%*[^|]|%d|%[^|]|%[^|]", &query_id, file, tag);

        char path_tag[1024];
        sprintf(path_tag, "./files/%s/%s", file, tag);

        struct stat info;
        if (stat(path_tag, &info) != 0)
        {
            send(client_sock, "ERR_TAG_NOT_FOUND", 18, 0);
            log_error(logger, "STORAGE | DELETE | Tag inexistente %s:%s", file, tag);
            break;
        }

        char path_metadata[1024];
        sprintf(path_metadata, "%s/metadata.config", path_tag);

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
        for (int i = 0; i < cant; i++)
        {
            sprintf(path_logical_block, "./files/%s/%s/logical_blocks/%06d.dat", file, tag, i);

            unlink(path_logical_block);

            char path_fisico[512];
            sprintf(path_fisico, "./physical_blocks/block%06d.dat", bloques[i]);

            struct stat st;
            stat(path_fisico, &st);

            if (st.st_nlink == 1)
            {
                marcar_bloque_libre(bloques[i], logger);
                log_info(logger, "STORAGE | DELETE | Bloque físico %d liberado", bloques[i]);
            }
        }

        char cmd_rm[1024];
        sprintf(cmd_rm, "rm -rf %s", path_tag);
        system(cmd_rm);

        char path_file[1024];
        sprintf(path_file, "./files/%s", file);
        DIR *d = opendir(path_file);
        int solo_dots = 1;
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL)
            if (ent->d_name[0] != '.')
                solo_dots = 0;
        closedir(d);

        if (solo_dots)
        {
            sprintf(cmd_rm, "rm -rf %s", path_file);
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
        send(client_sock, "OK", 2, 0);
        break;
    }
    case CMD_UNKNOWN:
    default:
    {
        send(client_sock, "ERR_UNKNOWN_COMMAND", 20, 0);
        break;
    }
    }

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
    snprintf(path_fisico, sizeof(path_fisico), "./physical_blocks/block%06d.dat", num_bloque_fisico);

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
    char path_bitmap[128] = "./bitmap.bin";
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
    char path_bitmap[128] = "./bitmap.bin";
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
    sprintf(path_block, "./physical_blocks/block%06d.dat", num_bloque);

    FILE *f = fopen(path_block, "rb");
    if (!f)
    {
        log_error(logger, "HASH | No se pudo abrir bloque físico %s", path_block);
        return;
    }

    unsigned char data[cfg->block_size];
    size_t leidos = fread(data, 1, cfg->block_size, f);
    fclose(f);

    if (leidos == 0)
    {
        log_error(logger, "HASH | Bloque vacío o error leyendo %s", path_block);
        return;
    }

    unsigned char hash[MD5_DIGEST_LENGTH];
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
    EVP_DigestUpdate(ctx, data, leidos);
    EVP_DigestFinal_ex(ctx, hash, NULL);
    EVP_MD_CTX_free(ctx);

    char hash_string[MD5_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(&hash_string[i * 2], "%02x", hash[i]);

    pthread_mutex_lock(&mutex_meta);

    FILE *index = fopen("./blocks_hash_index.config", "r+");
    if (!index)
        index = fopen("./blocks_hash_index.config", "w");

    if (!index)
    {
        pthread_mutex_unlock(&mutex_meta);
        log_error(logger, "HASH | No se pudo abrir blocks_hash_index.config");
        return;
    }

    char temp_path[] = "./blocks_hash_index.tmp";
    FILE *temp = fopen(temp_path, "w");
    if (!temp)
    {
        fclose(index);
        pthread_mutex_unlock(&mutex_meta);
        return;
    }

    char line[256];
    int updated = 0;
    while (fgets(line, sizeof(line), index))
    {
        int blk;
        if (sscanf(line, "BLOCK%d=", &blk) == 1 && blk == num_bloque)
        {
            fprintf(temp, "BLOCK%d=%s\n", num_bloque, hash_string);
            updated = 1;
        }
        else
        {
            fputs(line, temp);
        }
    }

    if (!updated)
        fprintf(temp, "BLOCK%d=%s\n", num_bloque, hash_string);

    fclose(index);
    fclose(temp);
    rename(temp_path, "./blocks_hash_index.config");

    pthread_mutex_unlock(&mutex_meta);

    log_info(logger, "HASH | Actualizado bloque %d -> %s", num_bloque, hash_string);
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

    char src_path[256], dst_path[256];
    sprintf(src_path, "./physical_blocks/block%06d.dat", bloque_original);
    sprintf(dst_path, "./physical_blocks/block%06d.dat", bloque_nuevo);

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
    char path_bitmap[128] = "./bitmap.bin";
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