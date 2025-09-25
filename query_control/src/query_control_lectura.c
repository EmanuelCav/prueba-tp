#include "../include/query_control_lectura.h"

void enviar_solicitud_query(int sock_master, char *archivo_query, int prioridad, t_log *logger)
{
    char buffer[MAX_BUFFER];
    sprintf(buffer, "0|%s|%d", archivo_query, prioridad);

    if (send(sock_master, buffer, strlen(buffer), 0) < 0)
    {
        log_error(logger, "Error enviando solicitud de query al Master");
        exit(EXIT_FAILURE);
    }

    log_info(logger, "Query enviada al Master: %s (prioridad %d)", archivo_query, prioridad);
}

void escuchar_respuestas_master(int socket_master, t_log *logger)
{
    char buffer[MAX_BUFFER];

    while (1)
    {
        memset(buffer, 0, MAX_BUFFER);
        int bytes_recibidos = recv(socket_master, buffer, MAX_BUFFER, 0);

        if (bytes_recibidos == -1)
        {
            perror("Error recibiendo datos del Master");
            log_error(logger, "Error en recv() desde el Master");
            break;
        }

        if (bytes_recibidos == 0)
        {
            log_info(logger, "## Query Finalizada - Desconexión del Master");
            break;
        }

        if (string_starts_with(buffer, "READ"))
        {
            char **partes = string_split(buffer, "|");
            log_info(logger, "## Lectura realizada: Archivo <%s>, contenido: <%s>", partes[1], partes[2]);

            string_iterate_lines(partes, (void *)free);
            free(partes);
        }
        else if (string_starts_with(buffer, "END"))
        {
            log_info(logger, "## Query Finalizada - %s", buffer + 4);
            break;
        }
    }
}