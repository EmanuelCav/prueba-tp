#include "../include/master_conexiones.h"

t_list *query_controls;

pthread_mutex_t mutex_ready = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_exec = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_query_controls = PTHREAD_MUTEX_INITIALIZER;

int query_id_counter = 1;

void desconectar_worker(int socket)
{
    for (int i = 0; i < cantidad_workers; i++)
    {
        if (workers[i].socket == socket)
        {
            t_query *query_executandose = NULL;

            if (workers[i].ocupado)
            {
                pthread_mutex_lock(&mutex_exec);

                for (int j = 0; j < list_size(exec); j++)
                {
                    t_query *query_actual = list_get(exec, j);
                    if (query_actual->worker_id == workers[i].worker_id)
                    {
                        query_executandose = query_actual;
                        break;
                    }
                }

                pthread_mutex_unlock(&mutex_exec);
            }

            if (workers[i].ocupado && query_executandose)
            {
                log_info(logger,
                         "## Se desconecta el Worker %d - Se finaliza la Query %d - Cantidad total de Workers: %d",
                         workers[i].worker_id, query_executandose->query_id, cantidad_workers - 1);
            }
            else
            {
                log_info(logger,
                         "## Se desconecta el Worker %d - Cantidad total de Workers: %d",
                         workers[i].worker_id, cantidad_workers - 1);
            }

            if (query_executandose)
            {
                pthread_mutex_lock(&mutex_query_controls);
                t_query_control_activo *qc = NULL;
                for (int k = 0; k < list_size(query_controls); k++)
                {
                    t_query_control_activo *qc_actual = list_get(query_controls, k);
                    if (qc_actual->query_id == query_executandose->query_id)
                    {
                        qc = qc_actual;
                        break;
                    }
                }

                if (qc && qc->activo)
                {
                    char mensaje_error[256];
                    sprintf(mensaje_error, "END|Error: Worker desconectado");
                    send(qc->socket, mensaje_error, strlen(mensaje_error), 0);
                    log_info(logger, "## Query %d finalizada por desconexión de Worker", query_executandose->query_id);
                }
                pthread_mutex_unlock(&mutex_query_controls);

                pthread_mutex_lock(&mutex_exec);
                list_remove_element(exec, query_executandose);
                query_destroy(query_executandose);
                pthread_mutex_unlock(&mutex_exec);
            }

            close(workers[i].socket);
            for (int j = i; j < cantidad_workers - 1; j++)
                workers[j] = workers[j + 1];
            cantidad_workers--;

            pthread_mutex_lock(&mutex_ready);
            pthread_mutex_lock(&mutex_exec);
            enviar_query_worker(ready, exec, logger);
            pthread_mutex_unlock(&mutex_exec);
            pthread_mutex_unlock(&mutex_ready);

            return;
        }
    }
}

void desconectar_query_control(t_query_control_activo *qc)
{
    log_info(logger,
             "## Se desconecta un Query Control. Se finaliza la Query %d con prioridad %d. Nivel multiprocesamiento %d",
             qc->query_id, qc->prioridad, cantidad_workers);

    close(qc->socket);
    qc->activo = false;

    pthread_mutex_lock(&mutex_ready);

    t_queue *temp_queue = queue_create();

    while (!queue_is_empty(ready))
    {
        t_query *temp_query = queue_pop(ready);
        if (temp_query->query_id == qc->query_id)
        {
            query_destroy(temp_query);
            log_info(logger, "## Query %d cancelada desde estado READY", qc->query_id);
        }
        else
        {
            queue_push(temp_queue, temp_query);
        }
    }
    while (!queue_is_empty(temp_queue))
    {
        queue_push(ready, queue_pop(temp_queue));
    }
    queue_destroy(temp_queue);
    pthread_mutex_unlock(&mutex_ready);

    pthread_mutex_lock(&mutex_exec);
    t_query *query_exec = NULL;
    for (int i = 0; i < list_size(exec); i++)
    {
        t_query *temp_query = list_get(exec, i);
        if (temp_query->query_id == qc->query_id)
        {
            query_exec = temp_query;
            break;
        }
    }

    if (query_exec)
    {
        for (int i = 0; i < cantidad_workers; i++)
        {
            if (workers[i].worker_id == query_exec->worker_id)
            {
                char mensaje_cancel[256];
                sprintf(mensaje_cancel, "CANCEL|%d", qc->query_id);
                send(workers[i].socket, mensaje_cancel, strlen(mensaje_cancel), 0);
                workers[i].ocupado = false;
                log_info(logger, "## Query %d cancelada en Worker %d", qc->query_id, workers[i].worker_id);
                break;
            }
        }
        list_remove_element(exec, query_exec);
        query_destroy(query_exec);
    }
    pthread_mutex_unlock(&mutex_exec);

    pthread_mutex_lock(&mutex_query_controls);
    for (int i = 0; i < list_size(query_controls); i++)
    {
        t_query_control_activo *qc_actual = list_get(query_controls, i);
        if (qc_actual->query_id == qc->query_id)
        {
            list_remove(query_controls, i);
            free(qc_actual);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_query_controls);

    pthread_mutex_lock(&mutex_ready);
    pthread_mutex_lock(&mutex_exec);
    enviar_query_worker(ready, exec, logger);
    pthread_mutex_unlock(&mutex_exec);
    pthread_mutex_unlock(&mutex_ready);
}

void *atender_conexion(void *arg)
{
    int socket_cliente = *(int *)arg;
    free(arg);

    char buffer[MAX_BUFFER];
    int bytes = recv(socket_cliente, buffer, sizeof(buffer) - 1, 0);

    if (bytes <= 0)
    {
        close(socket_cliente);
        pthread_exit(NULL);
    }

    buffer[bytes] = '\0';

    if (strncmp(buffer, "WORKER", 6) == 0)
    {
        int worker_id;
        sscanf(buffer, "WORKER|%d", &worker_id);
        registrar_worker(socket_cliente, logger, worker_id);

        while (1)
        {
            bytes = recv(socket_cliente, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0)
            {
                desconectar_worker(socket_cliente);
                pthread_exit(NULL);
            }

            buffer[bytes] = '\0';

            if (strncmp(buffer, "FIN_QUERY", 9) == 0)
            {
                int query_id;
                sscanf(buffer, "FIN_QUERY|%d", &query_id);
                log_info(logger, "## Se terminó la Query %d en el Worker %d", query_id, worker_id);

                pthread_mutex_lock(&mutex_exec);
                for (int i = 0; i < list_size(exec); i++)
                {
                    t_query *temp_query = list_get(exec, i);
                    if (temp_query->query_id == query_id)
                    {
                        list_remove(exec, i);
                        query_destroy(temp_query);
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex_exec);

                for (int i = 0; i < cantidad_workers; i++)
                {
                    if (workers[i].worker_id == worker_id)
                    {
                        workers[i].ocupado = false;
                        break;
                    }
                }

                pthread_mutex_lock(&mutex_ready);
                pthread_mutex_lock(&mutex_exec);
                enviar_query_worker(ready, exec, logger);
                pthread_mutex_unlock(&mutex_exec);
                pthread_mutex_unlock(&mutex_ready);
            }
            else if (strncmp(buffer, "READ", 4) == 0)
            {
                int query_id;
                char tag[256], contenido[512];
                sscanf(buffer, "READ|%d|%[^|]|%[^|]", &query_id, tag, contenido);

                pthread_mutex_lock(&mutex_query_controls);
                t_query_control_activo *qc = NULL;
                for (int i = 0; i < list_size(query_controls); i++)
                {
                    t_query_control_activo *qc_actual = list_get(query_controls, i);
                    if (qc_actual->query_id == query_id)
                    {
                        qc = qc_actual;
                        break;
                    }
                }

                if (qc && qc->activo)
                {
                    char mensaje_read[1024];
                    sprintf(mensaje_read, "READ|%s|%s", tag, contenido);
                    send(qc->socket, mensaje_read, strlen(mensaje_read), 0);
                    log_info(logger,
                             "## Se envía un mensaje de lectura de la Query %d en el Worker %d al Query Control",
                             query_id, worker_id);
                }
                pthread_mutex_unlock(&mutex_query_controls);
            }
        }
    }
    else
    {
        int query_id_sender;
        char path_query[512];
        int prioridad;

        sscanf(buffer, "%d|%[^|]|%d", &query_id_sender, path_query, &prioridad);

        int query_id = __sync_fetch_and_add(&query_id_counter, 1);

        t_query *query = query_create(query_id, prioridad, path_query);

        t_query_control_activo *qc = malloc(sizeof(t_query_control_activo));
        qc->socket = socket_cliente;
        qc->query_id = query_id;
        qc->prioridad = prioridad;
        strcpy(qc->path, path_query);
        qc->activo = true;

        pthread_mutex_lock(&mutex_query_controls);
        list_add(query_controls, qc);
        pthread_mutex_unlock(&mutex_query_controls);

        log_info(logger,
                 "## Se conecta un Query Control para ejecutar la Query %s con prioridad %d - Id asignado: %d. Nivel multiprocesamiento %d",
                 path_query, prioridad, query_id, cantidad_workers);

        pthread_mutex_lock(&mutex_ready);
        queue_push(ready, query);
        pthread_mutex_unlock(&mutex_ready);

        pthread_mutex_lock(&mutex_ready);
        pthread_mutex_lock(&mutex_exec);
        enviar_query_worker(ready, exec, logger);
        pthread_mutex_unlock(&mutex_exec);
        pthread_mutex_unlock(&mutex_ready);

        while (1)
        {
            bytes = recv(socket_cliente, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0)
            {
                desconectar_query_control(qc);
                pthread_exit(NULL);
            }

            buffer[bytes] = '\0';
        }
    }

    pthread_exit(NULL);
}