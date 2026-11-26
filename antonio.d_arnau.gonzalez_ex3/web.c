#define _POSIX_C_SOURCE 200809L
#include "comu.h"
#include "websocket.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

/* Marge per node i per parella d'enters de 32 bits, incloent el signe i els separadors. */
#define CAPACITAT_JSON (3 + NODES * (40 + POSICIONS * 26))

int escriure(int fd, void *dades, int mida) {
    char *pos = dades;
    int n = 0;

    while (mida > 0) {
        n = write(fd, pos, mida);
        if (n <= 0) {
            return -1;
        }
        pos += n;
        mida -= n;
    }
    return 0;
}

int trama(int fd, char *text) {
    int mida = strlen(text);
    /* La capcalera WebSocket es transmet com a bytes binaris. */
    unsigned char cap[4] = {129, 126, (unsigned char)(mida >> 8), (unsigned char)mida};
    int bytes = 4;

    if (mida < 126) {
        cap[1] = (unsigned char)mida;
        bytes = 2;
    }
    if (escriure(fd, cap, bytes) < 0) {
        return -1;
    }
    return escriure(fd, text, mida);
}

int estat_json(char *text, int capacitat) {
    Estat estats[NODES] = {0};
    int node = 0, clau = 0, fd = connectar(PORT_BASE);
    int cap[3] = {MONITOR, 0, 0};
    int usats = 0, mida = 0;

    if (fd >= 0 && enviar(fd, cap, 3) == 0) {
        while (node < NODES && rebre_estat(fd, &estats[node]) == 0) {
            node++;
        }
    }
    if (fd >= 0) close(fd);
    if (node != NODES) {
        mida = snprintf(text, capacitat, "{\"error\":\"Rèpliques no disponibles\"}");
        return mida < 0 || mida >= capacitat ? -1 : 0;
    }
    for (node = 0; node < NODES; node++) {
        mida = snprintf(text + usats, capacitat - usats,
                "%c\"%s\":{\"version\":%d,\"data\":{",
                node == 0 ? '{' : ',', noms[node], estats[node].versio);
        if (mida < 0 || mida >= capacitat - usats) {
            return -1;
        }
        usats += mida;
        for (clau = 0; clau < estats[node].quantitat; clau++) {
            mida = snprintf(text + usats, capacitat - usats, "%s\"%d\":%d",
                    clau == 0 ? "" : ",", estats[node].claus[clau], estats[node].valors[clau]);
            if (mida < 0 || mida >= capacitat - usats) {
                return -1;
            }
            usats += mida;
        }
        if (capacitat - usats < 4) {
            return -1;
        }
        text[usats++] = '}';
        text[usats++] = '}';
    }
    text[usats++] = '}';
    text[usats] = '\0';
    return 0;
}

void monitor(int fd, char *key) {
    char acceptacio[29], resposta[200], text[CAPACITAT_JSON], anterior[CAPACITAT_JSON] = "";
    int mida = 0;
    fd_set lectura;
    struct timeval espera;

    acceptar_websocket(key, acceptacio);
    mida = snprintf(resposta, sizeof(resposta), "HTTP/1.1 101 Switching Protocols\r\n"
                    "Upgrade: websocket\r\nConnection: Upgrade\r\n"
                    "Sec-WebSocket-Accept: %s\r\n\r\n", acceptacio);
    if (escriure(fd, resposta, mida) < 0) {
        return;
    }
    while (1) {
        if (estat_json(text, sizeof(text)) < 0) {
            strcpy(text, "{\"error\":\"Estat massa gran\"}");
        }
        if (strcmp(text, anterior) != 0) {
            if (trama(fd, text) < 0) {
                return;
            }
            strcpy(anterior, text);
        }
        FD_ZERO(&lectura);
        FD_SET(fd, &lectura);
        espera.tv_sec = 1;
        espera.tv_usec = 0;
        if (select(fd + 1, &lectura, NULL, NULL, &espera) != 0) {
            return;
        }
    }
}

void servir_fitxer(int fd, char *nom, char *tipus) {
    FILE *fitxer = fopen(nom, "rb");
    char cap[200], dades[4096];
    int mida_cap = 0;
    int mida = 0;

    if (!fitxer) {
        return;
    }
    mida_cap = snprintf(cap, sizeof(cap),
                        "HTTP/1.1 200 OK\r\nContent-Type: %s; charset=utf-8\r\n"
                        "Connection: close\r\n\r\n", tipus);
    if (escriure(fd, cap, mida_cap) == 0) {
        while ((mida = fread(dades, 1, sizeof(dades), fitxer)) > 0) {
            if (escriure(fd, dades, mida) < 0) {
                break;
            }
        }
    }
    fclose(fitxer);
}

void atendre(int fd) {
    char peticio[4096] = "", *key = NULL, *final = NULL;
    int usats = 0;
    int n = 0;

    while (!strstr(peticio, "\r\n\r\n") && usats < (int)sizeof(peticio) - 1) {
        n = read(fd, peticio + usats, sizeof(peticio) - 1 - usats);
        if (n <= 0) {
            return;
        }
        usats += n;
        peticio[usats] = '\0';
    }
    if (strncmp(peticio, "GET /versions ", 14) == 0) {
        key = strstr(peticio, "\r\nSec-WebSocket-Key:");
        if (!key) {
            return;
        }
        key += strlen("\r\nSec-WebSocket-Key:");
        key += strspn(key, " \t");
        final = strstr(key, "\r\n");
        if (!final || final - key != 24) {
            return;
        }
        *final = '\0';
        monitor(fd, key);
    } else if (strncmp(peticio, "GET / ", 6) == 0) {
        servir_fitxer(fd, "monitor.html", "text/html");
    } else if (strncmp(peticio, "GET /monitor.css ", 17) == 0) {
        servir_fitxer(fd, "monitor.css", "text/css");
    } else if (strncmp(peticio, "GET /monitor.js ", 16) == 0) {
        servir_fitxer(fd, "monitor.js", "text/javascript");
    }
}

int main(void) {
    int servidor = crear_servidor(PORT_WEB), fd = -1;
    pid_t fill = 0;

    if (servidor < 0) {
        perror("Monitor");
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    printf("Monitor: http://127.0.0.1:%d\n", PORT_WEB);
    fflush(stdout);
    while (1) {
        fd = accept(servidor, NULL, NULL);
        if (fd < 0) {
            break;
        }
        fill = fork();
        if (fill == 0) {
            close(servidor);
            atendre(fd);
            close(fd);
            _exit(0);
        }
        close(fd);
    }
    close(servidor);
    return 1;
}
