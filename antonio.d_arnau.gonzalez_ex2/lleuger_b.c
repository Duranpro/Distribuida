#include "comu.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main(int nombre_arguments, char **arguments) {
    int pesant = -1, actiu = 1;
    Missatge missatge = {0};
    RellotgeLamport rellotge = {0};

    (void)arguments;
    if (nombre_arguments != 1) {
        fprintf(stderr, "Us: ProcessLWB1, ProcessLWB2 o ProcessLWB3, sense arguments.\n");
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);
    pesant = connectar(5001);
    if (pesant < 0 || enviar(pesant, IDENTITAT, IDENTIFICADOR, 0) < 0) {
        actiu = 0;
    }
    while (actiu) {
        if (rebre(pesant, &missatge) < 0 || missatge.tipus != INICI) {
            actiu = 0;
        } else {
            rebre_lamport(&rellotge, missatge.temps);
            /* Fase 1: sense Ricart & Agrawala, no s'accedeix a la pantalla. */
            avancar_lamport(&rellotge);
            if (enviar(pesant, FI, IDENTIFICADOR, rellotge.valor) < 0) {
                actiu = 0;
            }
        }
    }

    fprintf(stderr, "ProcessLWB%d: connexio interrompuda o error d'inici.\n", IDENTIFICADOR + 1);
    if (pesant >= 0) {
        close(pesant);
    }
    return 1;
}
