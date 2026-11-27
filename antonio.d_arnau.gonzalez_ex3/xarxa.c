#include "comu.h"
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

char *noms[NODES] = {"A1", "A2", "A3", "B1", "B2", "C1", "C2"};

int crear_servidor(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adreca = {0};

    if (fd < 0) {
        return -1;
    }
    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(port);
    adreca.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr *)&adreca, sizeof(adreca)) < 0 || listen(fd, 16) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int connectar(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adreca = {0};

    if (fd < 0) {
        return -1;
    }
    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(port);
    adreca.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(fd, (struct sockaddr *)&adreca, sizeof(adreca)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int transferir(int fd, void *dades, int mida, int escriure) {
    char *pos = dades;
    int n = 0;

    while (mida > 0) {
        if (escriure) {
            n = write(fd, pos, mida);
        } else {
            n = read(fd, pos, mida);
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n <= 0) {
            return -1;
        }
        pos += n;
        mida -= n;
    }
    return 0;
}

int enviar(int fd, int *dades, int quantitat) {
    int enter = 0;
    int i = 0;

    for (i = 0; i < quantitat; i++) {
        enter = htonl(dades[i]); //revisar
        if (transferir(fd, &enter, sizeof(enter), 1) < 0) {
            return -1;
        }
    }
    return 0;
}

int rebre(int fd, int *dades, int quantitat) {
    int enter = 0;
    int i = 0;

    for (i = 0; i < quantitat; i++) {
        if (transferir(fd, &enter, sizeof(enter), 0) < 0) {
            return -1;
        }
        dades[i] = ntohl(enter); //revisar
    }
    return 0;
}

int enviar_estat(int fd, Estat *estat) {
    if (enviar(fd, &estat->versio, 1) < 0) {
        return -1;
    }
    if (enviar(fd, &estat->quantitat, 1) < 0 || enviar(fd, &estat->escriptures, 1) < 0 || enviar(fd, &estat->ultima, 1) < 0 || enviar(fd, estat->claus, estat->quantitat) < 0 || enviar(fd, estat->valors, estat->quantitat) < 0) {
        return -1;
    }
    return enviar(fd, estat->lectures, MAX_OPERACIONS);
}

int rebre_estat(int fd, Estat *estat) {
    if (rebre(fd, &estat->versio, 1) < 0 || estat->versio < 0) {
        return -1;
    }
    if (rebre(fd, &estat->quantitat, 1) < 0 || estat->quantitat < 0 || estat->quantitat > POSICIONS || rebre(fd, &estat->escriptures, 1) < 0 || rebre(fd, &estat->ultima, 1) < 0 || rebre(fd, estat->claus, estat->quantitat) < 0 || rebre(fd, estat->valors, estat->quantitat) < 0) {
        return -1;
    }
    return rebre(fd, estat->lectures, MAX_OPERACIONS);
}

int demanar(int node, Peticio *p, Estat *estat) {
    int fd = connectar(PORT_BASE + node), resultat = -1, i = 0;
    int cap[3] = {p->tipus, p->node, p->quantitat};

    if (fd < 0) {
        return -1;
    }
    if (enviar(fd, cap, 3) == 0) {
        for (i = 0; i < p->quantitat; i++) {
            if (enviar(fd, p->operacions[i], 3) < 0) {
                break;
            }
        }
        if (i == p->quantitat) {
            resultat = rebre_estat(fd, estat);
        }
    }
    close(fd);
    return resultat;
}
