#define _POSIX_C_SOURCE 200809L
#include "comu.h"
#include <stdio.h>
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

int executar_lleuger_a(RecursosLleuger *recursos) {
    int i = 0, maxim = 0, resultat = 0, mida = 0;
    char text[64] = {0};
    int cua[3] = {-1, -1, -1}, confirmacions[3] = {0};
    int demanant = 0, temps = 0;
    fd_set lectura = {0};
    Missatge missatge = {0};
    RellotgeDirecte rellotge = {{0, 0, 0}, IDENTIFICADOR};
    RellotgeLamport control = {0};

    signal(SIGPIPE, SIG_IGN);
    /* A3 nomes inicia connexions: no necessita servidor. */
    if (IDENTIFICADOR < 2) {
        recursos->servidor = crear_servidor(PORT_A_BASE + IDENTIFICADOR);
        if (recursos->servidor < 0) {
            perror("servidor lightweight A");
            return 1;
        }
    }
    recursos->pesant = connectar(PORT_PESANT_A);
    if (recursos->pesant < 0 || enviar(recursos->pesant, IDENTITAT, IDENTIFICADOR, 0) < 0) {
        return 1;
    }
    /* Un sol canal TCP bidireccional per parella, sempre iniciat pel major. */
    for (i = 0; i < IDENTIFICADOR; i++) {
        recursos->companys[i] = connectar(PORT_A_BASE + i);
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
    while (1) {
        FD_ZERO(&lectura);
        FD_SET(recursos->pesant, &lectura);
        maxim = recursos->pesant;
        for (i = 0; i < 3; i++) {
            if (i != IDENTIFICADOR) {
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
            rebre_lamport(&control, missatge.temps);
            demanant = 1;
            avancar_directe(&rellotge);
            temps = rellotge.valors[IDENTIFICADOR];
            cua[IDENTIFICADOR] = temps;
            for (i = 0; i < 3; i++) {
                confirmacions[i] = 0;
                if (i != IDENTIFICADOR && enviar(recursos->companys[i], REQUEST, IDENTIFICADOR, temps) < 0) {
                    return 1;
                }
            }
        }
        for (i = 0; i < 3; i++) {
            if (i == IDENTIFICADOR || !FD_ISSET(recursos->companys[i], &lectura)) {
                continue;
            }
            if (rebre(recursos->companys[i], &missatge) < 0 || missatge.origen != i) {
                return 1;
            }
            rebre_directe(&rellotge, i, missatge.temps);
            if (missatge.tipus == REQUEST) {
                cua[i] = missatge.temps;
                avancar_directe(&rellotge);
                if (enviar(recursos->companys[i], ACK, IDENTIFICADOR, rellotge.valors[IDENTIFICADOR]) < 0) {
                    return 1;
                }
            } else if (missatge.tipus == ACK) {
                if (demanant) {
                    confirmacions[i] = 1;
                }
            } else if (missatge.tipus == RELEASE) {
                cua[i] = -1;
            } else {
                return 1;
            }
        }
        if (demanant && pot_entrar(&rellotge, cua, confirmacions)) {
            /* Durant aquest bloc, els missatges queden pendents als sockets. */
            mida = snprintf(text, sizeof(text), "I'm lightweight process A%d\n", IDENTIFICADOR + 1);
            for (i = 0; i < 10; i++) {
                write(STDOUT_FILENO, text, mida);
                sleep(1);
            }
            cua[IDENTIFICADOR] = -1;
            avancar_directe(&rellotge);
            for (i = 0; i < 3; i++) {
                if (i != IDENTIFICADOR && enviar(recursos->companys[i], RELEASE, IDENTIFICADOR, rellotge.valors[IDENTIFICADOR]) < 0) {
                    return 1;
                }
            }
            demanant = 0;
            avancar_lamport(&control);
            if (enviar(recursos->pesant, FI, IDENTIFICADOR, control.valor) < 0) {
                return 1;
            }
        }
    }
}

int main(int nombre_arguments, char **arguments) {
    int i = 0, resultat = 0, mida = 0;
    char text[128] = {0};
    RecursosLleuger recursos = {-1, -1, -1, {-1, -1, -1}};

    (void)arguments;
    if (nombre_arguments != 1) {
        mida = snprintf(text, sizeof(text), "Us: ProcessLWA1, ProcessLWA2 o ProcessLWA3, sense arguments.\n");
        write(STDERR_FILENO, text, mida);
        return 1;
    }
    resultat = executar_lleuger_a(&recursos);
    mida = snprintf(text, sizeof(text), "ProcessLWA%d: connexio interrompuda o error d'inici.\n", IDENTIFICADOR + 1);
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
