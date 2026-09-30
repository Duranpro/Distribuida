#define _POSIX_C_SOURCE 200809L
#include "xarxa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif

int rellotge = 0, node = 0, mostrar_traces = 0;
char *noms_trames[] = {
    "INVALID", "READY", "START", "GRANT", "READ", "VALUE", "UPDATE",
    "ACK", "RELEASE", "STOP", "REQUEST", "DONE"
};
#ifdef _WIN32
LARGE_INTEGER inici = {0}, frequencia = {0};
#else
struct timespec inici = {0};
#endif

int temps_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER instant = {0};

    QueryPerformanceCounter(&instant);
    return (int)((instant.QuadPart - inici.QuadPart) * 1000 / frequencia.QuadPart);
#else
    struct timespec instant = {0};

    clock_gettime(CLOCK_MONOTONIC, &instant);
    return (int)((instant.tv_sec - inici.tv_sec) * 1000 +
                 (instant.tv_nsec - inici.tv_nsec) / 1000000);
#endif
}

void registrar(char *accio, int altre_node, char *tipus, int valor) {
    if (mostrar_traces) {
        printf("TRACE,%d,%d,%d,%s,%d,%s,%d\n", node, temps_ms(), rellotge, accio, altre_node, tipus, valor);
    }
}

void registrar_local(char *accio, int valor) {
    ++rellotge;
    registrar("LOCAL", node, accio, valor);
}

int iniciar_xarxa(int identificador) {
    char *opcio_traces = NULL;
#ifdef _WIN32
    WSADATA dades = {0};
#endif

    node = identificador;
    opcio_traces = getenv("DISTRIBUIDA_TRACES");
    if (opcio_traces != NULL && strcmp(opcio_traces, "1") == 0) {
        mostrar_traces = 1;
    }
    /* Les trames utilitzen tres enters de 4 bytes. */
    if (sizeof(unsigned int) != 4) {
        return -1;
    }
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &dades) != 0) {
        return -1;
    }
    QueryPerformanceFrequency(&frequencia);
    QueryPerformanceCounter(&inici);
#else
    signal(SIGPIPE, SIG_IGN);
    clock_gettime(CLOCK_MONOTONIC, &inici);
#endif
    return 0;
}

void acabar_xarxa(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

void tancar_socket(Socket connexio) {
    if (connexio == SOCKET_INVALID) {
        return;
    }
#ifdef _WIN32
    closesocket(connexio);
#else
    close(connexio);
#endif
}

void pausa_ms(int durada) {
#ifndef _WIN32
    struct timespec pausa = {0};
#endif

#ifdef _WIN32
    Sleep(durada);
#else
    pausa.tv_sec = durada / 1000;
    pausa.tv_nsec = (durada % 1000) * 1000000;
    while (nanosleep(&pausa, &pausa) == -1 && errno == EINTR) {
    }
#endif
}

int interromput(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAEINTR;
#else
    return errno == EINTR;
#endif
}

/* El temporitzador no impedeix rebre les repliques dels altres nodes. */
int esperar_sockets(Socket *connexions, int nombre, int espera_ms, int *preparats) {
    int i = 0, resultat = 0;
    Socket maxim = 0;
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
#ifndef _WIN32
        if (connexions[i] < 0 || connexions[i] >= FD_SETSIZE) {
            return -1;
        }
#endif
        FD_SET(connexions[i], &lectura);
        if (connexions[i] > maxim) {
            maxim = connexions[i];
        }
    }
    espera.tv_sec = espera_ms / 1000;
    espera.tv_usec = (espera_ms % 1000) * 1000;
    resultat = select((int)maxim + 1, &lectura, NULL, NULL, &espera);
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

Socket crear_socket_servidor(Configuracio *configuracio) {
    int activat = 1;
    Socket connexio = SOCKET_INVALID;
    struct sockaddr_in adreca = {0};

    connexio = socket(AF_INET, SOCK_STREAM, 0);
    if (connexio == SOCKET_INVALID) {
        return connexio;
    }
#ifndef _WIN32
    setsockopt(connexio, SOL_SOCKET, SO_REUSEADDR, (char *)&activat, sizeof(activat));
#else
    setsockopt(connexio, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *)&activat, sizeof(activat));
#endif
    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(configuracio->propi.port);
    adreca.sin_addr.s_addr = inet_addr(configuracio->propi.ip);
    if (bind(connexio, (struct sockaddr *)&adreca, sizeof(adreca)) != 0 || listen(connexio, SOMAXCONN) != 0) {
        tancar_socket(connexio);
        return SOCKET_INVALID;
    }
    return connexio;
}

Socket acceptar_connexio(Socket servidor) {
    Socket connexio = SOCKET_INVALID;

    do {
        connexio = accept(servidor, NULL, NULL);
    } while (connexio == SOCKET_INVALID && interromput());
    return connexio;
}

Socket connectar_servidor(Servidor *servidor) {
    int intent = 0;
    Socket connexio = SOCKET_INVALID;
    struct sockaddr_in adreca = {0};

    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(servidor->port);
    adreca.sin_addr.s_addr = inet_addr(servidor->ip);
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
int transferir(Socket connexio, char *dades, int mida, int enviament) {
    int total = 0, quantitat = 0;

    while (total < mida) {
        if (enviament) {
            quantitat = send(connexio, dades + total, mida - total, 0);
        } else {
            quantitat = recv(connexio, dades + total, mida - total, 0);
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

int enviar_trama(Socket connexio, int desti, TipusTrama tipus, int valor) {
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

int rebre_trama(Socket connexio, int origen, Trama *trama) {
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
