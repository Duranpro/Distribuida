#define _POSIX_C_SOURCE 200809L
#include "comu.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int node, servidor, actiu;
    int rellotge, instant_peticio, dins;
    int pendents[NODES_A], diferides[NODES_A];
    int cua[FD_SETSIZE], inici, quantitat;
    Estat estat;
    char fitxer[32];
} Replica;

int guardar(char *fitxer, char *mode, Estat *estat) {
    FILE *log = fopen(fitxer, mode);
    int i = 0, resultat = 0;

    if (!log) {
        return -1;
    }
    if (fprintf(log, "Versio %d · escriptures %d", estat->versio, estat->escriptures) < 0) {
        resultat = -1;
    }
    for (i = 0; i < estat->quantitat && resultat == 0; i++) {
        if (fprintf(log, " %d=%d", estat->claus[i], estat->valors[i]) < 0) {
            resultat = -1;
        }
    }
    if (resultat == 0 && fprintf(log, "\n") < 0) {
        resultat = -1;
    }
    if (fclose(log) == EOF) {
        resultat = -1;
    }
    return resultat;
}

/* Diccionari senzill de parelles clau-valor, amb cerca lineal. */
int buscar(Estat *estat, int clau) {
    int i = 0;

    while (i < estat->quantitat && estat->claus[i] != clau) {
        i++;
    }
    return i;
}

int executar(Estat *estat, Peticio *p) {
    int i = 0, pos = 0, escriptures = 0;

    memset(estat->lectures, 0, sizeof(estat->lectures));
    for (i = 0; i < p->quantitat; i++) {
        pos = buscar(estat, p->operacions[i][1]);
        if (p->operacions[i][0] == LECTURA) {
            if (pos < estat->quantitat) {
                estat->lectures[i] = estat->valors[pos];
            }
        } else {
            if (pos == estat->quantitat) {
                if (estat->quantitat == POSICIONS) {
                    return -1;
                }
                estat->claus[pos] = p->operacions[i][1];
                estat->quantitat++;
            }
            estat->valors[pos] = p->operacions[i][2];
            escriptures++;
        }
    }
    if (escriptures > 0) {
        estat->versio++;
        estat->escriptures += escriptures;
        estat->ultima = (int)time(NULL);
    }
    return escriptures;
}

int copiar(int node, Estat *estat) {
    int fd = connectar(PORT_BASE + node), resultat = -1;
    int cap[3] = {COPIAR, 0, 0};

    if (fd < 0) {
        return -1;
    }
    if (enviar(fd, cap, 3) == 0 && enviar_estat(fd, estat) == 0) {
        if (rebre(fd, &resultat, 1) < 0) {
            resultat = -1;
        }
    }
    close(fd);
    return resultat;
}

int distribuir(int node) {
    Peticio p = {0};
    Estat resposta = {0};

    p.tipus = DISTRIBUIR;
    return demanar(node, &p, &resposta);
}

/* El coordinador avisa el seu node que surt de la seccio critica tancant el socket. */
int entrar(int node) {
    int fd = connectar(PORT_BASE + node), confirmacio = -1;
    int cap[3] = {ENTRAR, node, 0};

    if (fd < 0) {
        return -1;
    }
    if (enviar(fd, cap, 3) < 0 || rebre(fd, &confirmacio, 1) < 0 || confirmacio != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int obtenir(int node, Estat *estat) {
    Peticio p = {0};

    p.tipus = ESTAT;
    return demanar(node, &p, estat);
}

int coordinar(int fd, int node, Peticio *p) {
    Estat inicial = {0}, resposta = {0}, local = {0};
    int permis = -1, i = 0, escriptures = 0, resultat = -1, desti = 0;

    desti = node;
    if (node >= NODES_A) {
        desti = 0;
    }
    permis = entrar(desti);
    if (permis < 0) {
        return -1;
    }
    if (obtenir(node, &inicial) < 0) {
        close(permis);
        return -1;
    }
    local = inicial;
    escriptures = executar(&local, p);
    if (escriptures >= 0 && (escriptures == 0 || node < NODES_A)) {
        p->tipus = APLICAR;
        resultat = 0;
        if (escriptures == 0) {
            if (demanar(node, p, &resposta) < 0) {
                resultat = -1;
            }
        } else {
            /* El receptor coordina i espera l'execucio de tots tres A. */
            for (i = 0; i < NODES_A && resultat == 0; i++) {
                desti = (node + i) % NODES_A;
                if (demanar(desti, p, &resposta) < 0 || resposta.versio != local.versio || resposta.escriptures != local.escriptures || resposta.quantitat != local.quantitat || memcmp(resposta.claus, local.claus, local.quantitat * sizeof(int)) != 0 || memcmp(resposta.valors, local.valors, local.quantitat * sizeof(int)) != 0 || memcmp(resposta.lectures, local.lectures, sizeof(local.lectures)) != 0) {
                    resultat = -1;
                }
            }
            if (resultat == 0 && inicial.escriptures / PERIODE != local.escriptures / PERIODE) {
                if (distribuir(1) < 0 || distribuir(2) < 0) {
                    resultat = -1;
                }
            }
        }
        if (resultat == 0) {
            resultat = enviar_estat(fd, &resposta);
        }
    }
    if (resultat < 0) {
        enviar(fd, &resultat, 1);
    }
    close(permis);
    return resultat;
}

int mostrar(int fd) {
    Estat estat = {0};
    int permis = entrar(0), i = 0, resultat = 0;

    if (permis < 0) {
        return -1;
    }
    for (i = 0; i < NODES; i++) {
        if (obtenir(i, &estat) < 0 || enviar_estat(fd, &estat) < 0) {
            resultat = -1;
            break;
        }
    }
    close(permis);
    return resultat;
}

int propagar(void) {
    Estat nucli = {0}, b = {0};
    int permis = entrar(0), resultat = 0;

    if (permis < 0) {
        return -1;
    }
    if (obtenir(1, &nucli) < 0 || obtenir(4, &b) < 0) {
        resultat = -1;
    } else {
        /* El lot final pendent permet la convergencia quan cessen les escriptures. */
        if (nucli.versio > b.versio && difftime(time(NULL), nucli.ultima) >= PERIODE) {
            if (distribuir(1) < 0 || distribuir(2) < 0) {
                resultat = -1;
            }
        }
        if (resultat == 0 && distribuir(4) < 0) {
            resultat = -1;
        }
    }
    close(permis);
    return resultat;
}

int rebre_peticio(int fd, Peticio *p) {
    int cap[3] = {0}, i = 0;

    if (rebre(fd, cap, 3) < 0 || cap[2] < 0 || cap[2] > MAX_OPERACIONS) {
        return -1;
    }
    p->tipus = cap[0];
    p->node = cap[1];
    p->quantitat = cap[2];
    for (i = 0; i < p->quantitat; i++) {
        if (rebre(fd, p->operacions[i], 3) < 0 || (p->operacions[i][0] != LECTURA && p->operacions[i][0] != ESCRIPTURA) || p->operacions[i][1] < 0) {
            return -1;
        }
    }
    return 0;
}

void tancar_heretats(int conservar) {
    int i = 0;

    for (i = 3; i < FD_SETSIZE; i++) {
        if (i != conservar) {
            close(i);
        }
    }
}

void acabar_peticio(Replica *r) {
    int i = 0, permis = 0;

    close(r->actiu);
    r->actiu = -1;
    r->dins = 0;
    for (i = 0; i < NODES_A; i++) {
        if (r->pendents[i] >= 0) {
            close(r->pendents[i]);
            r->pendents[i] = -1;
        }
        if (r->diferides[i] >= 0) {
            enviar(r->diferides[i], &permis, 1);
            close(r->diferides[i]);
            r->diferides[i] = -1;
        }
    }
}

void iniciar_peticio(Replica *r) {
    int i = 0, fd = -1;
    int cap[4] = {DEMANAR_PERMIS, r->node, 0, ++r->rellotge};

    r->actiu = r->cua[r->inici];
    r->inici = (r->inici + 1) % FD_SETSIZE;
    r->quantitat--;
    r->instant_peticio = cap[3];
    for (i = 0; i < NODES_A; i++) {
        if (i == r->node) {
            continue;
        }
        fd = connectar(PORT_BASE + i);
        if (fd < 0 || fd >= FD_SETSIZE || enviar(fd, cap, 4) < 0) {
            if (fd >= 0) {
                close(fd);
            }
            acabar_peticio(r);
            return;
        }
        /* La resposta pot arribar mes tard. El bucle select continua atenent peticions. */
        r->pendents[i] = fd;
    }
}

int respondre_permis(int fd, Replica *r, int origen) {
    int instant = 0, permis = 0;

    if (origen < 0 || origen >= NODES_A || origen == r->node || rebre(fd, &instant, 1) < 0 || instant <= 0) {
        return -1;
    }
    if (instant > r->rellotge) {
        r->rellotge = instant;
    }
    r->rellotge++;
    /* Ricart-Agrawala: prioritat per rellotge de Lamport i desempat per ID. */
    if (r->dins || (r->actiu >= 0 && (r->instant_peticio < instant || (r->instant_peticio == instant && r->node < origen)))) {
        if (r->diferides[origen] >= 0) {
            close(r->diferides[origen]);
        }
        r->diferides[origen] = fd;
        return 1;
    }
    return enviar(fd, &permis, 1);
}

int atendre(int fd, Replica *r) {
    Peticio p = {0};
    Estat copia = {0};
    int resultat = -1, desti = -1, escriptures = 0;
    pid_t fill = 0;

    if (rebre_peticio(fd, &p) < 0) {
        return -1;
    }
    if (p.tipus == DEMANAR_PERMIS && r->node < NODES_A) {
        return respondre_permis(fd, r, p.node);
    } else if (p.tipus == ENTRAR && r->node < NODES_A) {
        if (r->quantitat == FD_SETSIZE) {
            return -1;
        }
        r->cua[(r->inici + r->quantitat) % FD_SETSIZE] = fd;
        r->quantitat++;
        return 1;
    } else if (p.tipus == TRANSACCIO || (p.tipus == MONITOR && r->node == 0)) {
        fill = fork();
        if (fill < 0) {
            return -1;
        }
        if (fill == 0) {
            tancar_heretats(fd);
            if (p.tipus == MONITOR) {
                resultat = mostrar(fd);
            } else {
                resultat = coordinar(fd, r->node, &p);
            }
            close(fd);
            if (resultat < 0) {
                _exit(1);
            }
            _exit(0);
        }
    } else if (p.tipus == ESTAT) {
        enviar_estat(fd, &r->estat);
    } else if (p.tipus == APLICAR) {
        copia = r->estat;
        escriptures = executar(&copia, &p);
        if (escriptures < 0 || (escriptures > 0 && r->node >= NODES_A)) {
            return -1;
        }
        if (escriptures > 0) {
            if (guardar(r->fitxer, "a", &copia) < 0) {
                return -2;
            }
            r->estat = copia;
        }
        enviar_estat(fd, &copia);
    } else if (p.tipus == COPIAR && r->node >= NODES_A) {
        if (rebre_estat(fd, &copia) < 0 || copia.versio < r->estat.versio) {
            return -1;
        }
        resultat = 0;
        if (copia.versio > r->estat.versio) {
            if (guardar(r->fitxer, "a", &copia) < 0) {
                return -2;
            }
            r->estat = copia;
        }
        enviar(fd, &resultat, 1);
    } else if (p.tipus == DISTRIBUIR) {
        if (r->node == 1) {
            desti = 3;
        } else if (r->node == 2) {
            desti = 4;
        } else if (r->node == 4) {
            desti = 5;
        }
        if (desti < 0 || copiar(desti, &r->estat) < 0) {
            return -2;
        }
        if (r->node == 4 && copiar(6, &r->estat) < 0) {
            return -2;
        }
        enviar_estat(fd, &r->estat);
    } else {
        enviar(fd, &resultat, 1);
    }
    return 0;
}

int main(int argc, char **argv) {
    Replica r = {0};
    fd_set lectura;
    struct timeval espera;
    time_t proper = time(NULL) + PERIODE, ara = 0;
    pid_t fill_periodic = 0;
    int avis[2], periodic = -1;
    int fd = -1, maxim = 0, resultat = 0, confirmacio = 0, i = 0, faltants = 0;
    char missatge[32];

    if (argc != 2) {
        fprintf(stderr, "Ús: %s A1|A2|A3|B1|B2|C1|C2\n", argv[0]);
        return 1;
    }
    while (r.node < NODES && strcmp(argv[1], noms[r.node]) != 0) {
        r.node++;
    }
    if (r.node == NODES) {
        return 1;
    }
    r.actiu = -1;
    for (i = 0; i < NODES_A; i++) {
        r.pendents[i] = -1;
        r.diferides[i] = -1;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    r.servidor = crear_servidor(PORT_BASE + r.node);
    if (r.servidor < 0) {
        perror("Servidor");
        return 1;
    }
    snprintf(r.fitxer, sizeof(r.fitxer), "%s.txt", noms[r.node]);
    if (guardar(r.fitxer, "w", &r.estat) < 0) {
        resultat = -2;
    } else {
        snprintf(missatge, sizeof(missatge), "%s preparat\n", noms[r.node]);
        write(STDOUT_FILENO, missatge, strlen(missatge));
    }
    while (resultat != -2) {
        ara = time(NULL);
        if (r.node == 4 && ara >= proper) {
            /* Un sol treballador periodic per evitar acumular temporitzadors. */
            if (periodic < 0 && pipe(avis) == 0) {
                fill_periodic = avis[0] < FD_SETSIZE && avis[1] < FD_SETSIZE ? fork() : -1;
                if (fill_periodic == 0) {
                    /* En acabar, el pare detectara el tancament de la canonada. */
                    tancar_heretats(avis[1]);
                    resultat = propagar();
                    if (resultat < 0) {
                        _exit(1);
                    }
                    _exit(0);
                }
                close(avis[1]);
                if (fill_periodic > 0) {
                    periodic = avis[0];
                } else {
                    close(avis[0]);
                }
            }
            proper = ara + PERIODE;
        }
        FD_ZERO(&lectura);
        FD_SET(r.servidor, &lectura);
        maxim = r.servidor;
        if (periodic >= 0) {
            FD_SET(periodic, &lectura);
            if (periodic > maxim) {
                maxim = periodic;
            }
        }
        if (r.actiu >= 0) {
            FD_SET(r.actiu, &lectura);
            if (r.actiu > maxim) {
                maxim = r.actiu;
            }
        }
        for (i = 0; i < NODES_A; i++) {
            fd = r.pendents[i];
            if (fd >= 0) {
                FD_SET(fd, &lectura);
                if (fd > maxim) {
                    maxim = fd;
                }
            }
        }
        espera.tv_sec = 0;
        espera.tv_usec = 100000;
        if (select(maxim + 1, &lectura, NULL, NULL, &espera) < 0) {
            if (errno == EINTR) {
                continue;
            }
            resultat = -2;
            break;
        }
        if (r.actiu >= 0 && FD_ISSET(r.actiu, &lectura)) {
            acabar_peticio(&r);
        }
        if (periodic >= 0 && FD_ISSET(periodic, &lectura)) {
            close(periodic);
            periodic = -1;
        }
        faltants = 0;
        for (i = 0; i < NODES_A; i++) {
            fd = r.pendents[i];
            if (fd >= 0 && FD_ISSET(fd, &lectura)) {
                if (rebre(fd, &confirmacio, 1) < 0 || confirmacio != 0) {
                    acabar_peticio(&r);
                    break;
                }
                close(fd);
                r.pendents[i] = -1;
            }
            faltants += r.pendents[i] >= 0;
        }
        if (r.actiu >= 0 && !r.dins && faltants == 0) {
            r.dins = 1;
            if (enviar(r.actiu, &confirmacio, 1) < 0) {
                acabar_peticio(&r);
            }
        }
        if (FD_ISSET(r.servidor, &lectura)) {
            fd = accept(r.servidor, NULL, NULL);
            if (fd >= FD_SETSIZE) {
                close(fd);
            } else if (fd >= 0) {
                resultat = atendre(fd, &r);
                if (resultat != 1) {
                    close(fd);
                }
            }
        }
        if (resultat != -2 && r.actiu < 0 && r.quantitat > 0) {
            iniciar_peticio(&r);
        }
    }
    if (r.actiu >= 0) {
        acabar_peticio(&r);
    }
    if (periodic >= 0) {
        close(periodic);
    }
    for (i = 0; i < r.quantitat; i++) {
        close(r.cua[(r.inici + i) % FD_SETSIZE]);
    }
    close(r.servidor);
    return 1;
}
