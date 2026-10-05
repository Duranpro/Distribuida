#define _POSIX_C_SOURCE 200809L
#include "xarxa.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

int rellotge = 0, node = 0;
char *noms_trames[] = {
    "INVALID", "READY", "START", "GRANT", "READ", "VALUE", "UPDATE",
    "ACK", "RELEASE", "STOP", "REQUEST", "DONE"
};
struct timespec inici = {0};

int temps_ms(void) {
    struct timespec instant = {0};

    clock_gettime(CLOCK_MONOTONIC, &instant);
    return (int)((instant.tv_sec - inici.tv_sec) * 1000 + (instant.tv_nsec - inici.tv_nsec) / 1000000);
}

void registrar(char *accio, int altre_node, char *tipus, int valor) {
    printf("[Node %d | %d ms | Lamport %d] ", node, temps_ms(), rellotge);
    if (strcmp(accio, "SEND") == 0) {
        printf("Envia %s al node %d | valor: %d\n", tipus, altre_node, valor);
    } else if (strcmp(accio, "RECV") == 0) {
        if (altre_node < 0) {
            printf("Rep %s d'un node pendent d'identificar | valor: %d\n", tipus, valor);
        } else {
            printf("Rep %s del node %d | valor: %d\n", tipus, altre_node, valor);
        }
    } else if (strcmp(tipus, "ENCUAR") == 0) {
        printf("Posa el node %d a la cua\n", valor);
    } else if (strcmp(tipus, "CONCEDIR") == 0) {
        printf("Concedeix el torn al node %d\n", valor);
    } else if (strcmp(tipus, "ALLIBERAR") == 0) {
        printf("El node %d allibera el torn\n", valor);
    } else if (strcmp(tipus, "READ") == 0) {
        printf("Llegeix el valor local: %d\n", valor);
    } else if (strcmp(tipus, "UPDATE") == 0 || strcmp(tipus, "APLICAR") == 0) {
        printf("Actualitza el valor local a %d\n", valor);
    } else if (strcmp(tipus, "CALCULAR") == 0) {
        printf("Calcula el nou valor: %d\n", valor);
    } else if (strcmp(tipus, "REQUEST") == 0) {
        printf("Demana torn per a la iteracio %d\n", valor);
    } else if (strcmp(tipus, "DONE") == 0) {
        printf("Ha acabat totes les iteracions\n");
    } else {
        printf("Accio local %s | valor: %d\n", tipus, valor);
    }
}

void registrar_local(char *accio, int valor) {
    ++rellotge;
    registrar("LOCAL", node, accio, valor);
}

int iniciar_xarxa(int identificador) {
    node = identificador;
    /* Les trames utilitzen tres enters de 4 bytes. */
    if (sizeof(unsigned int) != 4) {
        return -1;
    }
    signal(SIGPIPE, SIG_IGN);
    clock_gettime(CLOCK_MONOTONIC, &inici);
    return 0;
}

void tancar_socket(int connexio) {
    if (connexio == SOCKET_INVALID) {
        return;
    }
    close(connexio);
}

void pausa_ms(int durada) {
    struct timespec pausa = {0};

    pausa.tv_sec = durada / 1000;
    pausa.tv_nsec = (durada % 1000) * 1000000;
    while (nanosleep(&pausa, &pausa) == -1 && errno == EINTR) {
    }
}

int interromput(void) {
    return errno == EINTR;
}

/* El temporitzador no impedeix rebre les repliques dels altres nodes. */
int esperar_sockets(int *connexions, int nombre, int espera_ms, int *preparats) {
    int i = 0, resultat = 0;
    int maxim = 0;
    fd_set lectura;
    struct timeval espera = {0};

    if (nombre == 0) {
        pausa_ms(espera_ms);
        return 0;
    }
    if (nombre > FD_SETSIZE) {
        fprintf(stderr, "Massa connexions per al select d'aquest sistema.\n");
        return -1;
    }
    FD_ZERO(&lectura);
    for (i = 0; i < nombre; ++i) {
        preparats[i] = 0;
        if (connexions[i] < 0 || connexions[i] >= FD_SETSIZE) {
            return -1;
        }
        FD_SET(connexions[i], &lectura);
        if (connexions[i] > maxim) {
            maxim = connexions[i];
        }
    }
    espera.tv_sec = espera_ms / 1000;
    espera.tv_usec = (espera_ms % 1000) * 1000;
    resultat = select(maxim + 1, &lectura, NULL, NULL, &espera);
    if (resultat < 0) {
        if (interromput()) {
            return 0;
        }
        return -1;
    }
    for (i = 0; i < nombre; ++i) {
        preparats[i] = FD_ISSET(connexions[i], &lectura) != 0;
    }
    return resultat;
}

int crear_socket_servidor(Configuracio *configuracio) {
    int activat = 1;
    int connexio = SOCKET_INVALID;
    struct sockaddr_in adreca = {0};

    connexio = socket(AF_INET, SOCK_STREAM, 0);
    if (connexio == SOCKET_INVALID) {
        return connexio;
    }
    setsockopt(connexio, SOL_SOCKET, SO_REUSEADDR, &activat, sizeof(activat));
    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(configuracio->propi.port);
    if (inet_pton(AF_INET, configuracio->propi.ip, &adreca.sin_addr) != 1) {
        tancar_socket(connexio);
        return SOCKET_INVALID;
    }
    if (bind(connexio, (struct sockaddr *)&adreca, sizeof(adreca)) != 0 || listen(connexio, SOMAXCONN) != 0) {
        tancar_socket(connexio);
        return SOCKET_INVALID;
    }
    return connexio;
}

int acceptar_connexio(int servidor) {
    int connexio = SOCKET_INVALID;

    do {
        connexio = accept(servidor, NULL, NULL);
    } while (connexio == SOCKET_INVALID && interromput());
    return connexio;
}

int connectar_servidor(Servidor *servidor) {
    int intent = 0;
    int connexio = SOCKET_INVALID;
    struct sockaddr_in adreca = {0};

    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(servidor->port);
    if (inet_pton(AF_INET, servidor->ip, &adreca.sin_addr) != 1) {
        return SOCKET_INVALID;
    }
    for (intent = 0; intent < 100; ++intent) {
        connexio = socket(AF_INET, SOCK_STREAM, 0);
        if (connexio == SOCKET_INVALID) {
            return connexio;
        }
        if (connect(connexio, (struct sockaddr *)&adreca, sizeof(adreca)) == 0) {
            return connexio;
        }
        tancar_socket(connexio);
        pausa_ms(100);
    }
    return SOCKET_INVALID;
}

/* TCP pot transferir nomes una part dels bytes demanats. */
int transferir(int connexio, char *dades, int mida, int enviament) {
    int total = 0, quantitat = 0;

    while (total < mida) {
        if (enviament) {
            quantitat = write(connexio, dades + total, mida - total);
        } else {
            quantitat = read(connexio, dades + total, mida - total);
        }
        if (quantitat < 0 && interromput()) {
            continue;
        }
        if (quantitat <= 0) {
            return -1;
        }
        total += quantitat;
    }
    return 0;
}

int enviar_trama(int connexio, int desti, TipusTrama tipus, int valor) {
    unsigned int dades[3] = {0};

    ++rellotge;
    dades[0] = htonl(tipus);
    dades[1] = htonl(valor);
    dades[2] = htonl(rellotge);
    if (transferir(connexio, (char *)dades, sizeof(dades), 1) != 0) {
        return -1;
    }
    registrar("SEND", desti, noms_trames[tipus], valor);
    return 0;
}

int rebre_trama(int connexio, int origen, Trama *trama) {
    unsigned int dades[3] = {0};
    int remot = 0, tipus = 0, valor = 0;

    if (transferir(connexio, (char *)dades, sizeof(dades), 0) != 0) {
        return -1;
    }
    tipus = (int)ntohl(dades[0]);
    valor = (int)ntohl(dades[1]);
    remot = (int)ntohl(dades[2]);
    if (tipus < READY || tipus > DONE || valor < 0 || remot < 0) {
        return -1;
    }
    if (remot > rellotge) {
        rellotge = remot;
    }
    ++rellotge;
    trama->tipus = (TipusTrama)tipus;
    trama->valor = valor;
    registrar("RECV", origen, noms_trames[tipus], valor);
    return 0;
}
