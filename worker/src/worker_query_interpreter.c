#include "../include/worker_query_interpreter.h"

t_instruccion instr_to_enum(char *line)
{
    char buffer[256];
    strcpy(buffer, line);
    char * instruccion = strtok(buffer," ");

    if (strcmp(instruccion, "CREATE")== 0) {return INS_CREATE;}
    else if (strcmp(instruccion, "TRUNCATE")== 0) {return INS_TRUNCATE;}
    else if (strcmp(instruccion, "WRITE")== 0) {return INS_WRITE;}
    else if (strcmp(instruccion, "READ")== 0) {return INS_READ;}
    else if (strcmp(instruccion, "TAG")== 0) {return INS_TAG;}
    else if (strcmp(instruccion, "COMMIT")== 0) {return INS_COMMIT;}
    else if (strcmp(instruccion, "FLUSH")== 0) {return INS_FLUSH;}
    else if (strcmp(instruccion, "DELETE")== 0) {return INS_DELETE;}
    else if (strcmp(instruccion, "END")== 0) {return INS_END;}
    else {return INS_UNKNOWN;}
    
}

void query_interpretar(char *line, int query_id, char *path_query, t_log *logger)
{

    switch (instr_to_enum(line))
    {
    case INS_CREATE:

        break;

    case INS_TRUNCATE:

        break;

    case INS_WRITE:

        break;

    case INS_READ:

        break;

    case INS_TAG:

        break;

    case INS_COMMIT:

        break;

    case INS_FLUSH:

        break;

    case INS_DELETE:

        break;

    case INS_END:

        break;

    case INS_UNKNOWN:
        log_error(logger, "Query: %d, File: %s instruccion desconocida", query_id, path_query);
        break;

    default:
        break;
    }

}