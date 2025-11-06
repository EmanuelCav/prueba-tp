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

    log_info(logger, "## Solicitud de ejecución de Query: %s, prioridad: %d", archivo_query, prioridad);
}

void escuchar_respuestas_master(int socket_master, t_log *logger)
{
    char c;
    char buffer[MAX_BUFFER];
    int pos = 0;

    while (recv(socket_master, &c, 1, 0) > 0)
    {
        if (c == '\n')
        {
            buffer[pos] = '\0';
            pos = 0;

            if (string_starts_with(buffer, "READ"))
            {
                char **partes = string_split(buffer, "|");
                log_info(logger, "## Lectura realizada: File <%s>, contenido: <%s>", partes[1], partes[2]);
                string_iterate_lines(partes, (void *)free);
                free(partes);
            }
            else if (string_starts_with(buffer, "END"))
            {
                char **partes = string_split(buffer, "|");
                char *motivo = partes[1] ? partes[1] : "Finalización normal";
                log_info(logger, "## Query Finalizada - %s", motivo);
                string_iterate_lines(partes, (void *)free);
                free(partes);
                break;
            }
        }
        else
        {
            buffer[pos++] = c;
        }
    }
}