#include <stdio.h>
#include "config.h"
#include "xarxa.h"

int main(int argc, char *argv[]) {
    Configuracio configuracio;
    int socket_servidor;

    if (llegir_arguments(argc, argv, &configuracio) != 0) {
        return 1;
    }

    mostrar_configuracio(&configuracio);

    socket_servidor = crear_socket_servidor(&configuracio);

    if (socket_servidor == -1) {
        return 1;
    }

    printf("Prem ENTER per tancar el servidor...\n");
    getchar();

    return 0;
}