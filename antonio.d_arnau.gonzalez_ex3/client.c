#include "comu.h"
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int llegir_transaccio(char *linia, int nucli, Peticio *p) {
    char *s = linia;
    char *final = NULL;
    long clau = 0, valor = 0;
    int capa = -1, tipus = 0, escriu = 0;

    s += strspn(s, " \t");
    if (*s++ != 'b') {
        return -1;
    }
    if (*s >= '0' && *s <= '2') {
        capa = *s++ - '0';
    }
    else if (*s == '<') {
        if (s[1] < '0' || s[1] > '2' || s[2] != '>') {
            return -1;
        }
        capa = s[1] - '0';
        s += 3;
    }
    p->tipus = TRANSACCIO;
    if (capa == 1) {
        p->node = 3;
    } else if (capa == 2) {
        p->node = 5;
    } else {
        p->node = nucli;
    }
    while (1) {
        s += strspn(s, " \t");
        if (*s++ != ',') {
            return -1;
        }
        s += strspn(s, " \t");
        if (*s == 'c') {
            s++;
            s += strspn(s, " \t\r\n");
            if (*s != '\0' || p->quantitat == 0 || (capa < 0 && !escriu)) {
                return -1;
            }
            return 0;
        }
        if (p->quantitat == MAX_OPERACIONS || (*s != 'r' && *s != 'w')) {
            return -1;
        }
        if (*s == 'r') {
            tipus = LECTURA;
        } else {
            tipus = ESCRIPTURA;
        }
        s++;
        if (*s++ != '(') {
            return -1;
        }
        clau = strtol(s, &final, 10);
        if (final == s || clau < 0 || clau > INT_MAX) {
            return -1;
        }
        s = final + strspn(final, " \t");
        if (tipus == ESCRIPTURA) {
            if (capa >= 0 || *s++ != ',') {
                return -1;
            }
            valor = strtol(s, &final, 10);
            if (final == s || valor < INT_MIN || valor > INT_MAX) {
                return -1;
            }
            s = final + strspn(final, " \t");
            escriu = 1;
        }
        if (*s++ != ')') {
            return -1;
        }
        p->operacions[p->quantitat][0] = tipus;
        p->operacions[p->quantitat][1] = (int)clau;
        p->operacions[p->quantitat][2] = (int)valor;
        p->quantitat++;
    }
}

int main(int argc, char **argv) {
    FILE *fitxer = NULL;
    char linia[4096];
    Peticio p = {0};
    Estat estat = {0};
    int nucli = 0, numero = 0, i = 0, clau = 0, error = 0;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Ús: %s fitxer [A1|A2|A3]\n", argv[0]);
        return 1;
    }
    if (argc == 3) {
        while (nucli < 3 && strcmp(noms[nucli], argv[2]) != 0) {
            nucli++;
        }
        if (nucli == 3) {
            return 1;
        }
    }
    signal(SIGPIPE, SIG_IGN);
    fitxer = fopen(argv[1], "r");
    if (!fitxer) {
        perror(argv[1]);
        return 1;
    }
    while (fgets(linia, sizeof(linia), fitxer)) {
        numero++;
        if (linia[strspn(linia, " \t\r\n")] == '\0') {
            continue;
        }
        p.quantitat = 0;
        if ((!strchr(linia, '\n') && strlen(linia) == sizeof(linia) - 1) ||
            llegir_transaccio(linia, nucli, &p) < 0 ||
            demanar(p.node, &p, &estat) < 0) {
            fprintf(stderr, "Error a la transacció de la línia %d\n", numero);
            error = 1;
            break;
        }
        printf("Línia %d · %s · versió %d", numero, noms[p.node], estat.versio);
        for (i = 0; i < p.quantitat; i++) {
            clau = p.operacions[i][1];
            if (p.operacions[i][0] == LECTURA) printf(" · r(%d)=%d", clau, estat.lectures[i]);

        }
        putchar('\n');
    }
    if (ferror(fitxer)) {
        error = 1;
    }
    fclose(fitxer);
    return error;
}
