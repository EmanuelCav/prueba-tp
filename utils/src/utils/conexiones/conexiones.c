#include <conexiones.h>

int iniciar_servidor(char *puerto)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, puerto, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "Fallo al hacer bind\n");
        exit(2);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(3);
    }

    printf("## Master escuchando en puerto %s\n", puerto);
    return sockfd;
}

int conectar_servidor(char *ip, char *puerto)
{
    int conn = -1;
    struct addrinfo hints;
    struct addrinfo *serverInfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int rv = getaddrinfo(ip, puerto, &hints, &serverInfo);
    if (rv != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
        return -1;
    }

    for (p = serverInfo; p != NULL; p = p->ai_next)
    {
        conn = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (conn == -1)
        {
            perror("socket");
            continue;
        }

        if (connect(conn, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("connect");
            close(conn);
            conn = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(serverInfo);

    return conn;
}