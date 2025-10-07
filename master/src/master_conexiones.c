#include "../include/master_conexiones.h"

void desconectar_worker(int socket, t_log *logger)
{
    for (int i = 0; i < cantidad_workers; i++)
    {
        if (workers[i].socket == socket)
        {
            log_info(logger, "## Se desconecta el Worker %d - Cantidad total de Workers: %d",
                     workers[i].worker_id, cantidad_workers - 1);

            close(workers[i].socket);

            for (int j = i; j < cantidad_workers - 1; j++)
                workers[j] = workers[j + 1];
            cantidad_workers--;

            return;
        }
    }
}

void desconectar_query_control(t_query_control_activo *qc, t_log *logger)
{
    log_info(logger,
             "## Se desconecta un Query Control. Se finaliza la Query %d con prioridad %d. Nivel multiprocesamiento %d",
             qc->query_id, qc->prioridad, cantidad_workers);

    close(qc->socket);
    qc->activo = false;

    bool _es_qc(void *elemento)
    {
        return ((t_query_control_activo *)elemento)->query_id == qc->query_id;
    }

    pthread_mutex_lock(&mutex_exec);
    list_remove_and_destroy_by_condition(query_controls, _es_qc, free);
    pthread_mutex_unlock(&mutex_exec);
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
        }
    }
    else
    {
        int query_id_sender;
        char path_query[512];
        int prioridad;

        sscanf(buffer, "%d|%[^|]|%d", &query_id_sender, path_query, &prioridad);

        static int query_id_counter = 1;
        int query_id = __sync_fetch_and_add(&query_id_counter, 1);

        t_query *query = query_create(query_id, prioridad, path_query);

        t_query_control_activo *qc = malloc(sizeof(t_query_control_activo));
        qc->socket = socket_cliente;
        qc->query_id = query_id;
        qc->prioridad = prioridad;
        strcpy(qc->path, path_query);
        qc->activo = true;

        list_add(query_controls, qc);

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