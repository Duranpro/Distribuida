#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "xarxa.h"


int crear_socket_servidor(Configuracio *configuracio) {
    int socket_servidor, resultat;
    struct sockaddr_in adreca;

    socket_servidor = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_servidor == -1) {
        printf("Error creant el socket\n");
        return -1;
    }

    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(configuracio->propi.port);
    adreca.sin_addr.s_addr = inet_addr(configuracio->propi.ip);

    resultat = bind(socket_servidor, (struct sockaddr *)&adreca, sizeof(adreca));

    if (resultat == -1) {
        printf("Error fent bind\n");
        close(socket_servidor);
        return -1;
    }

    resultat = listen(socket_servidor, 5);

    if (resultat == -1) {
        printf("Error fent listen\n");
        close(socket_servidor);
        return -1;
    }

    printf("Servidor escoltant a %s:%d\n", configuracio->propi.ip, configuracio->propi.port);

    return socket_servidor;
}