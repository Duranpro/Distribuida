#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "config.h"

int llegir_enter(char *text, int minim, int maxim, int *valor) {
    char *final = NULL;
    long numero = 0;

    errno = 0;
    
    // strtol permet distingir un zero d'un argument que no es numeric.
    numero = strtol(text, &final, 10);
    if (errno != 0 || *text == '\0' || *final != '\0' || numero < minim || numero > maxim) {
        return -1;
    }
    *valor = (int)numero;
    return 0;
}

int llegir_servidor(char **arguments, Servidor *servidor) {
    int primer = 0, segon = 0, tercer = 0, quart = 0, camps = 0;
    char sobrant = '\0';

    servidor->ip = arguments[1];
    if (llegir_enter(arguments[0], 0, INT_MAX, &servidor->id) != 0 || llegir_enter(arguments[2], 1, 65535, &servidor->port) != 0) {
        return -1;
    }
    camps = sscanf(servidor->ip, "%d.%d.%d.%d%c", &primer, &segon, &tercer, &quart, &sobrant);
    if (camps != 4 || primer < 0 || primer > 255 || segon < 0 || segon > 255 || tercer < 0 || tercer > 255 || quart < 0 || quart > 255 || strcmp(servidor->ip, "255.255.255.255") == 0) {
        return -1;
    }
    return 0;
}

int llegir_arguments(int quantitat_arguments, char *arguments[], Configuracio *configuracio) {
    int i = 0, j = 0, coordinador_trobat = 0, correcte = 1;
    Servidor *servidor = NULL, *anterior = NULL;

    if (quantitat_arguments < 5 || (quantitat_arguments - 5) % 3 != 0) {
        fprintf(stderr, "Us: %s ID IPv4 PORT READ_ONLY|READ_WRITE [ID IPv4 PORT] ...\n", arguments[0]);
        return -1;
    }
    if (llegir_servidor(arguments + 1, &configuracio->propi) != 0) {
        correcte = 0;
    }
    if (strcmp(arguments[4], "READ_ONLY") == 0) {
        configuracio->tipus = READ_ONLY;
    } else if (strcmp(arguments[4], "READ_WRITE") == 0) {
        configuracio->tipus = READ_WRITE;
    } else {
        correcte = 0;
    }
    configuracio->nombre_altres_servidors = (quantitat_arguments - 5) / 3;
    if (correcte && configuracio->nombre_altres_servidors > 0) {
        configuracio->altres_servidors = malloc(configuracio->nombre_altres_servidors * sizeof(Servidor));
        if (configuracio->altres_servidors == NULL) {
            fprintf(stderr, "No s'ha pogut reservar memoria.\n");
            return -1;
        }
    }
    if (configuracio->propi.id == 0) {
        coordinador_trobat = 1;
    }
    for (i = 0; i < configuracio->nombre_altres_servidors && correcte; ++i) {
        servidor = &configuracio->altres_servidors[i];
        if (llegir_servidor(arguments + 5 + 3 * i, servidor) != 0 || servidor->id == configuracio->propi.id) {
            correcte = 0;
            break;
        }
        if (servidor->port == configuracio->propi.port && strcmp(servidor->ip, configuracio->propi.ip) == 0) {
            correcte = 0;
        }
        for (j = 0; j < i && correcte; ++j) {
            anterior = &configuracio->altres_servidors[j];
            if (servidor->id == anterior->id || (servidor->port == anterior->port && strcmp(servidor->ip, anterior->ip) == 0)) {
                correcte = 0;
            }
        }
        if (servidor->id == 0) {
            coordinador_trobat = 1;
        }
    }
    if (correcte && coordinador_trobat) {
        return 0;
    }
    fprintf(stderr, "Configuracio invalida: revisa IDs, IPv4, ports, tipus i node 0.\n");
    free(configuracio->altres_servidors);
    configuracio->altres_servidors = NULL;
    return -1;
}

void mostrar_configuracio(Configuracio *configuracio) {
    printf("\n=== NODE %d", configuracio->propi.id);
    if (configuracio->propi.id == 0) {
        printf(" - COORDINADOR");
    }
    printf(" ===\n");
    if (configuracio->tipus == READ_ONLY) {
        printf("Mode: nomes lectura\n");
    } else {
        printf("Mode: lectura i escriptura\n");
    }
    printf("Adreca configurada: %s:%d\n", configuracio->propi.ip, configuracio->propi.port);
    printf("Valor inicial: %d | Iteracions: %d\n\n", configuracio->valor_local, ITERACIONS);
}
