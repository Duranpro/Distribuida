#define _POSIX_C_SOURCE 200809L
#include "comu.h"
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int crear_servidor(int port) {
    int connexio = -1, activat = 1;
    struct sockaddr_in adreca = {0};

    connexio = socket(AF_INET, SOCK_STREAM, 0);
    if (connexio < 0) {
        return -1;
    }
    setsockopt(connexio, SOL_SOCKET, SO_REUSEADDR, &activat, sizeof(activat));
    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(port);
    adreca.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(connexio, (struct sockaddr *)&adreca, sizeof(adreca)) < 0 || listen(connexio, 4) < 0) {
        close(connexio);
        return -1;
    }
    return connexio;
}

int connectar(int port) {
    int connexio = -1, intent = 0;
    struct sockaddr_in adreca = {0};
    struct timespec pausa = {0, 100000000};

    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(port);
    adreca.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    /* Dona temps als altres programes per obrir els seus servidors. */
    for (intent = 0; intent < 100; intent++) {
        connexio = socket(AF_INET, SOCK_STREAM, 0);
        if (connexio < 0) {
            return -1;
        }
        if (connect(connexio, (struct sockaddr *)&adreca, sizeof(adreca)) == 0) {
            return connexio;
        }
        close(connexio);
        nanosleep(&pausa, NULL);
    }
    return -1;
}

/* TCP es un flux: cada missatge ocupa exactament tres enters de xarxa. */
int enviar(int connexio, int tipus, int origen, int temps) {
    int dades[3] = {0}, total = 0, quantitat = 0, mida = sizeof(dades);

    dades[0] = htonl(tipus);
    dades[1] = htonl(origen);
    dades[2] = htonl(temps);
    while (total < mida) {
        quantitat = write(connexio, (char *)dades + total, mida - total);
        if (quantitat < 0 && errno == EINTR) {
            continue;
        }
        if (quantitat <= 0) {
            return -1;
        }
        total += quantitat;
    }
    return 0;
}

int rebre(int connexio, Missatge *missatge) {
    int dades[3] = {0}, total = 0, quantitat = 0, mida = sizeof(dades);

    while (total < mida) {
        quantitat = read(connexio, (char *)dades + total, mida - total);
        if (quantitat < 0 && errno == EINTR) {
            continue;
        }
        if (quantitat <= 0) {
            return -1;
        }
        total += quantitat;
    }
    missatge->tipus = ntohl(dades[0]);
    missatge->origen = ntohl(dades[1]);
    missatge->temps = ntohl(dades[2]);
    return 0;
}
