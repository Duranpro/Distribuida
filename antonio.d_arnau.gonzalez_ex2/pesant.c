#define _POSIX_C_SOURCE 200809L
#include "comu.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>

typedef struct {
    int servidor, altre, connexio, lleugers[3], fills[3];
} RecursosPesant;

int executar_pesant(RecursosPesant *recursos) {
    int i = 0, torn = 0;
    char programa[32] = {0}, grup = 'A';
    Missatge missatge = {0};
    RellotgeLamport rellotge = {0};

    if (GRUP == 1) {
        grup = 'B';
    }
    signal(SIGPIPE, SIG_IGN);
    recursos->servidor = crear_servidor(5000 + GRUP);
    if (recursos->servidor < 0) {
        perror("servidor heavyweight");
        return 1;
    }
    for (i = 0; i < 3; i++) {
        snprintf(programa, sizeof(programa), "./ProcessLW%c%d", grup, i + 1);
        recursos->fills[i] = fork();
        if (recursos->fills[i] < 0) {
            perror("fork");
            return 1;
        }
        if (recursos->fills[i] == 0) {
            close(recursos->servidor);
            execl(programa, programa, (char *)NULL);
            perror("execl");
            _exit(1);
        }
    }
    for (i = 0; i < 3 + (GRUP == 0); i++) {
        recursos->connexio = accept(recursos->servidor, NULL, NULL);
        if (recursos->connexio < 0 || rebre(recursos->connexio, &missatge) < 0) {
            return 1;
        }
        if (missatge.tipus != IDENTITAT) {
            return 1;
        }
        /* B pot connectar abans que algun lightweight d'A. */
        if (GRUP == 0 && missatge.origen == 3 && recursos->altre < 0) {
            recursos->altre = recursos->connexio;
        } else if (missatge.origen >= 0 && missatge.origen < 3 && recursos->lleugers[missatge.origen] < 0) {
            recursos->lleugers[missatge.origen] = recursos->connexio;
        } else {
            return 1;
        }
        recursos->connexio = -1;
    }
    /* Nomes A crea el testimoni inicial; B espera el primer traspas. */
    if (GRUP == 0) {
        torn = 1;
    } else {
        recursos->altre = connectar(5000);
        if (recursos->altre < 0 || enviar(recursos->altre, IDENTITAT, 3, 0) < 0) {
            return 1;
        }
    }
    if (recursos->altre < 0) {
        return 1;
    }
    close(recursos->servidor);
    recursos->servidor = -1;
    while (1) {
        if (!torn) {
            if (rebre(recursos->altre, &missatge) < 0 || missatge.tipus != TESTIMONI) {
                return 1;
            }
            rebre_lamport(&rellotge, missatge.temps);
            torn = 1;
        }
        avancar_lamport(&rellotge);
        for (i = 0; i < 3; i++) {
            if (enviar(recursos->lleugers[i], INICI, GRUP, rellotge.valor) < 0) {
                return 1;
            }
        }
        for (i = 0; i < 3; i++) {
            if (rebre(recursos->lleugers[i], &missatge) < 0 || missatge.tipus != FI || missatge.origen != i) {
                return 1;
            }
            rebre_lamport(&rellotge, missatge.temps);
        }
        torn = 0;
        avancar_lamport(&rellotge);
        if (enviar(recursos->altre, TESTIMONI, GRUP, rellotge.valor) < 0) {
            return 1;
        }
    }
}

int main(int nombre_arguments, char **arguments) {
    int i = 0, resultat = 0;
    char grup = 'A';
    RecursosPesant recursos = {-1, -1, -1, {-1, -1, -1}, {-1, -1, -1}};

    (void)arguments;
    if (nombre_arguments != 1) {
        fprintf(stderr, "Us: ProcessA o ProcessB, sense arguments.\n");
        return 1;
    }
    if (GRUP == 1) {
        grup = 'B';
    }
    resultat = executar_pesant(&recursos);
    fprintf(stderr, "Process%c: connexio interrompuda o error d'inici.\n", grup);
    if (recursos.connexio >= 0) {
        close(recursos.connexio);
    }
    if (recursos.servidor >= 0) {
        close(recursos.servidor);
    }
    if (recursos.altre >= 0) {
        close(recursos.altre);
    }
    for (i = 0; i < 3; i++) {
        if (recursos.lleugers[i] >= 0) {
            close(recursos.lleugers[i]);
        }
        if (recursos.fills[i] > 0) {
            kill(recursos.fills[i], SIGTERM);
            waitpid(recursos.fills[i], NULL, 0);
        }
    }
    return resultat;
}
