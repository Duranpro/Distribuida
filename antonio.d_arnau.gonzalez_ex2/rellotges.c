#include "comu.h"

void avancar_lamport(RellotgeLamport *rellotge) {
    rellotge->valor++;
}

void rebre_lamport(RellotgeLamport *rellotge, int temps) {
    if (temps > rellotge->valor) {
        rellotge->valor = temps;
    }
    avancar_lamport(rellotge);
}

void avancar_directe(RellotgeDirecte *rellotge) {
    rellotge->valors[rellotge->identificador]++;
}

void rebre_directe(RellotgeDirecte *rellotge, int origen, int temps) {
    if (temps > rellotge->valors[origen]) {
        rellotge->valors[origen] = temps;
    }
    if (temps > rellotge->valors[rellotge->identificador]) {
        rellotge->valors[rellotge->identificador] = temps;
    }
    avancar_directe(rellotge);
}

int pot_entrar(RellotgeDirecte *rellotge, int *cua, int *confirmacions) {
    int i = 0, propi = rellotge->identificador, temps = cua[propi];

    if (temps < 0) {
        return 0;
    }
    for (i = 0; i < 3; i++) {
        if (i == propi) {
            continue;
        }
        if (!confirmacions[i]) {
            return 0;
        }
        /* Desempat per identificador, igual que a l'apendix LamportMutex. */
        if (cua[i] >= 0 && (cua[i] < temps || (cua[i] == temps && i < propi))) {
            return 0;
        }
        if (rellotge->valors[i] < temps || (rellotge->valors[i] == temps && i < propi)) {
            return 0;
        }
    }
    return 1;
}
