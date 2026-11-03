#define _POSIX_C_SOURCE 200809L
#include "comu.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>

typedef struct {
    int servidor;
    int pesant;
    int connexio;
    int companys[3];
} RecursosLleuger;

/* Amb el primer INICI, el pesant ja no fa accept al servidor heretat. */
int preparar_companys_b(RecursosLleuger *recursos) {
    int i = 0, port = 0;
    Missatge missatge = {0};

    /* B1 obre servidor; B2 l'hereta del pesant i B3 nomes connecta. */
    if (IDENTIFICADOR == 0) {
        recursos->servidor = crear_servidor(PORT_B_PRIMER);
        if (recursos->servidor < 0) {
            perror("servidor lightweight B");
            return 1;
        }
    }
    /* Un sol canal TCP bidireccional per parella, sempre iniciat pel major. */
    for (i = 0; i < IDENTIFICADOR; i++) {
        port = PORT_B_PRIMER;
        if (i == 1) {
            port = PORT_PESANT_B;
        }
        recursos->companys[i] = connectar(port);
        if (recursos->companys[i] < 0 || enviar(recursos->companys[i], IDENTITAT, IDENTIFICADOR, 0) < 0) {
            return 1;
        }
    }
    for (i = IDENTIFICADOR + 1; i < 3; i++) {
        recursos->connexio = accept(recursos->servidor, NULL, NULL);
        if (recursos->connexio < 0 || rebre(recursos->connexio, &missatge) < 0) {
            return 1;
        }
        if (missatge.tipus != IDENTITAT || missatge.origen <= IDENTIFICADOR || missatge.origen > 2 || recursos->companys[missatge.origen] >= 0) {
            return 1;
        }
        recursos->companys[missatge.origen] = recursos->connexio;
        recursos->connexio = -1;
    }
    if (recursos->servidor >= 0) {
        close(recursos->servidor);
        recursos->servidor = -1;
    }
    return 0;
}

int executar_lleuger_b(RecursosLleuger *recursos) {
    int i = 0, maxim = 0, resultat = 0, mida = 0;
    char text[64] = {0};
    int confirmacions[3] = {0}, ajornades[3] = {0};
    int demanant = 0, temps = 0, preparat = 0;
    fd_set lectura = {0};
    Missatge missatge = {0};
    RellotgeLamport rellotge = {0};

    signal(SIGPIPE, SIG_IGN);
    recursos->pesant = connectar(PORT_PESANT_B);
    if (recursos->pesant < 0 || enviar(recursos->pesant, IDENTITAT, IDENTIFICADOR, 0) < 0) {
        return 1;
    }
    while (1) {
        FD_ZERO(&lectura);
        FD_SET(recursos->pesant, &lectura);
        maxim = recursos->pesant;
        for (i = 0; i < 3; i++) {
            if (i != IDENTIFICADOR && preparat) {
                FD_SET(recursos->companys[i], &lectura);
                if (recursos->companys[i] > maxim) {
                    maxim = recursos->companys[i];
                }
            }
        }
        resultat = select(maxim + 1, &lectura, NULL, NULL, NULL);
        if (resultat < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 1;
        }
        if (FD_ISSET(recursos->pesant, &lectura)) {
            if (rebre(recursos->pesant, &missatge) < 0 || missatge.tipus != INICI || demanant) {
                return 1;
            }
            if (!preparat) {
                if (preparar_companys_b(recursos) != 0) {
                    return 1;
                }
                preparat = 1;
            }
            rebre_lamport(&rellotge, missatge.temps);
            avancar_lamport(&rellotge);
            temps = rellotge.valor;
            demanant = 1;
            for (i = 0; i < 3; i++) {
                confirmacions[i] = 0;
                if (i != IDENTIFICADOR && enviar(recursos->companys[i], REQUEST, IDENTIFICADOR, temps) < 0) {
                    return 1;
                }
            }
        }
        for (i = 0; i < 3; i++) {
            if (i == IDENTIFICADOR || !preparat || !FD_ISSET(recursos->companys[i], &lectura)) {
                continue;
            }
            if (rebre(recursos->companys[i], &missatge) < 0 || missatge.origen != i) {
                return 1;
            }
            rebre_lamport(&rellotge, missatge.temps);
            /* Ricart-Agrawala: Lecture 3 part 2, apendix RAMutex. */
            if (missatge.tipus == REQUEST) {
                if (!demanant || missatge.temps < temps || (missatge.temps == temps && i < IDENTIFICADOR)) {
                    avancar_lamport(&rellotge);
                    if (enviar(recursos->companys[i], ACK, IDENTIFICADOR, rellotge.valor) < 0) {
                        return 1;
                    }
                } else {
                    ajornades[i] = 1;
                }
            } else if (missatge.tipus == ACK) {
                if (demanant) {
                    confirmacions[i] = 1;
                }
            } else {
                return 1;
            }
        }
        if (demanant) {
            resultat = 1;
            for (i = 0; i < 3; i++) {
                if (i != IDENTIFICADOR && !confirmacions[i]) {
                    resultat = 0;
                }
            }
            if (resultat) {
                /* Durant aquest bloc, els missatges queden pendents als sockets. */
                mida = snprintf(text, sizeof(text), "I'm lightweight process B%d\n", IDENTIFICADOR + 1);
                for (i = 0; i < 10; i++) {
                    write(STDOUT_FILENO, text, mida);
                    sleep(1);
                }
                demanant = 0;
                /* En sortir, responem les peticions ajornades; no hi ha RELEASE. */
                for (i = 0; i < 3; i++) {
                    if (ajornades[i]) {
                        avancar_lamport(&rellotge);
                        if (enviar(recursos->companys[i], ACK, IDENTIFICADOR, rellotge.valor) < 0) {
                            return 1;
                        }
                        ajornades[i] = 0;
                    }
                }
                avancar_lamport(&rellotge);
                if (enviar(recursos->pesant, FI, IDENTIFICADOR, rellotge.valor) < 0) {
                    return 1;
                }
            }
        }
    }
}

int main(int nombre_arguments, char **arguments) {
    int i = 0, resultat = 0, mida = 0;
    char text[128] = {0};
    RecursosLleuger recursos = {-1, -1, -1, {-1, -1, -1}};

    if (nombre_arguments != 1 + (IDENTIFICADOR == 1)) {
        mida = snprintf(text, sizeof(text), "Us: iniciar els lightweight B des de ProcessB.\n");
        write(STDERR_FILENO, text, mida);
        return 1;
    }
    if (IDENTIFICADOR == 1) {
        recursos.servidor = atoi(arguments[1]);
    }
    resultat = executar_lleuger_b(&recursos);
    mida = snprintf(text, sizeof(text), "ProcessLWB%d: connexio interrompuda o error d'inici.\n", IDENTIFICADOR + 1);
    write(STDERR_FILENO, text, mida);
    if (recursos.servidor >= 0) {
        close(recursos.servidor);
    }
    if (recursos.connexio >= 0) {
        close(recursos.connexio);
    }
    if (recursos.pesant >= 0) {
        close(recursos.pesant);
    }
    for (i = 0; i < 3; i++) {
        if (recursos.companys[i] >= 0) {
            close(recursos.companys[i]);
        }
    }
    return resultat;
}
