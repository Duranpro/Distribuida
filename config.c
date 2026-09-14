#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"


int llegir_arguments(int argc, char *argv[], Configuracio *configuracio) {
    int index, posicio_servidor;

    if (argc < 5) {
        printf("Us: %s ID IP PORT TIPUS [ID IP PORT] ...\n", argv[0]);
        return -1;
    }

    if ((argc - 5) % 3 != 0) {
        printf("Error en la llista de servidors\n");
        return -1;
    }

    configuracio->propi.id = atoi(argv[1]);
    configuracio->propi.ip = argv[2];
    configuracio->propi.port = atoi(argv[3]);

    if (strcmp(argv[4], "READ_ONLY") == 0) {
        configuracio->tipus = READ_ONLY;
    }
    else if (strcmp(argv[4], "READ_WRITE") == 0) {
        configuracio->tipus = READ_WRITE;
    }
    else {
        printf("El tipus ha de ser READ_ONLY o READ_WRITE\n");
        return -1;
    }

    configuracio->valor_local = 0;
    configuracio->nombre_altres_servidors = 0;

    index = 5;
    posicio_servidor = 0;

    while (index < argc) {
        configuracio->altres_servidors[posicio_servidor].id = atoi(argv[index]);

        configuracio->altres_servidors[posicio_servidor].ip = argv[index + 1];

        configuracio->altres_servidors[posicio_servidor].port = atoi(argv[index + 2]);

        posicio_servidor++;
        index += 3;
    }

    configuracio->nombre_altres_servidors = posicio_servidor;

    return 0;
}


void mostrar_configuracio(Configuracio *configuracio)
{
    int i;

    printf("Servidor %d\n", configuracio->propi.id);
    printf("IP: %s\n", configuracio->propi.ip);
    printf("Port: %d\n", configuracio->propi.port);

    if (configuracio->tipus == READ_ONLY) {
        printf("Tipus: READ_ONLY\n");
    }
    else {
        printf("Tipus: READ_WRITE\n");
    }

    printf("Valor local: %d\n", configuracio->valor_local);

    printf("Altres servidors:\n");

    for (i = 0; i < configuracio->nombre_altres_servidors; i++) {
        printf("Servidor %d - %s:%d\n", configuracio->altres_servidors[i].id, configuracio->altres_servidors[i].ip, configuracio->altres_servidors[i].port);
    }
}