#include "../include/query_control_lectura.h"

void enviar_solicitud_query(int socket_master, char *path_query, int prioridad, t_log *logger)
{
    char mensaje[MAX_BUFFER];
    snprintf(mensaje, MAX_BUFFER, "%s|%d", path_query, prioridad);

    send(socket_master, mensaje, strlen(mensaje), 0);
    log_info(logger, "## Solicitud de ejecución de Query: %s, prioridad: %d", path_query, prioridad);
}

void escuchar_respuestas_master(int socket_master, t_log *logger)
{
    char buffer[MAX_BUFFER];

    while (1)
    {
        memset(buffer, 0, MAX_BUFFER);
        int bytes_recibidos = recv(socket_master, buffer, MAX_BUFFER, 0);

        if (bytes_recibidos <= 0)
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