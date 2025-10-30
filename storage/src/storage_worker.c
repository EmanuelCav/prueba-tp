#include "../include/storage_worker.h"
#include "../include/storage_config.h"
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
        struct stat info;
        sscanf(buffer, "%*[^|]|%d|%[^|]|%[^|]|%d", &query_id, file, tag, &tamanio);

        char path_tag[4096];
        snprintf(path_tag, sizeof(path_tag), "./files/%s/%s", file, tag);
        if (stat(path_tag, &info) == 0 && S_ISDIR(info.st_mode))
        {
            log_error(logger, "STORAGE | intento de creacion de un file:tag preexistente. File:%s Tag:%s", file, tag);
            char respuesta[512];
            sprintf(respuesta, "ERR_%s_%s_ALREADY_EXIST", file, tag);
            send(client_sock, respuesta, strlen(respuesta), 0);
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
            // agrandar
            cant_bloques = (tamanio - tamanio_actual) / cfg->block_size;

            char path_block0[4096];
            snprintf(path_block0, sizeof(path_block0), "./physical_blocks/block0000.dat");
            int num_bloque = 0;
            char bloque_nombre[7];
            for (int i = 0; i < cant_bloques; i++)
            {
                num_bloque = bloques_actuales + i;
                sprintf(bloque_nombre, "%06d", num_bloque);
                char path_logical_block[4096];
                snprintf(path_logical_block, sizeof(path_logical_block), "./files/%s/%s/logical_blocks/%s.dat", file, tag, bloque_nombre);
                link(path_block0, path_logical_block);
                actualizar_metadata("AGREGAR", path_metadata);
            }
        }
        else
        {
            // achicar
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
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Bloque Lógico Escrito <NOMBRE_FILE>:<TAG> - Número de Bloque: <BLOQUE>");
        break;
    }
    case CMD_READ:
    {
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Bloque Lógico Leído <NOMBRE_FILE>:<TAG> - Número de Bloque: <BLOQUE>");
        break;
    }
    case CMD_TAG:
    {
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Tag creado <NOMBRE_FILE>:<TAG>");
        break;
    }
    case CMD_COMMIT:
    {
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Commit de File:Tag <NOMBRE_FILE>:<TAG>");
        break;
    }
    case CMD_DELETE:
    {
        send(client_sock, "OK", 2, 0);
        log_info(logger, "##<QUERY_ID> - Tag Eliminado <NOMBRE_FILE>:<TAG>");
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
        log_error(logger, "STORAGE | TRUNCATE | Error al buscar numero de bloque logico. File:%s Tag:%s",file,tag);
        char msg[512] = "";
        sprintf(msg, "ERR_PATH_LOGICAL");
        send(client_sock, msg, strlen(msg), 0);
        return;
    }
    int num_bloque_fisico = actualizar_metadata("SACAR", path_metadata);
    if (num_bloque_fisico == -1)
    {
        log_error(logger, "STORAGE | TRUNCATE | Error al modificar metadata. File:%s Tag:%s",file,tag);
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
    bitmap_map[byte_index] &= ~(1 << bit_index);
    msync(bitmap_map, tamanio_fd, MS_SYNC);
    pthread_mutex_unlock(&mutex_bitmap);

    munmap(bitmap_map, tamanio_fd);
    log_info(logger, "STORAGE | Bitmap actualizado: bloque %d libre.", num_bloque);
}
