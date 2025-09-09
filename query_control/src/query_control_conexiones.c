#include <include/query_control_conexiones.h>

int conectar_master(char *ip, int puerto)
{
    char puerto_str[10];
    snprintf(puerto_str, sizeof(puerto_str), "%d", puerto);

    int conn = conectar_servidor(ip, puerto_str);
    if (conn == -1)
    {
        fprintf(stderr, "No se pudo conectar al Master (%s:%d)\n", ip, puerto);
        exit(EXIT_FAILURE);
    }

    return conn;
}