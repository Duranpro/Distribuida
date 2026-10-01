#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "xarxa.h"

/* Un unic fil per proces. select permet servir repliques durant la pausa. */
#define LLIURE 0
#define EN_CUA 1
#define ACTIU 2
#define ACABAT 3
typedef struct {
    Configuracio *configuracio;
    int *connexions;
    int escolta;
    int *estat;
    int *iteracions;
    int *cua;
    int inici;
    int nombre;
    int acabats;

} Central;

int esperar(int connexio, int id, TipusTrama tipus, Trama *trama) {
    if (rebre_trama(connexio, id, trama) != 0 || trama->tipus != tipus) {
        fprintf(stderr, "Node %d: connexio tancada o trama inesperada (%d esperada).\n", id, tipus);
        return -1;
    }
    return 0;
}

int id_node(Central *central, int posicio) {
    if (posicio == central->configuracio->nombre_altres_servidors) {
        return 0;
    }
    return central->configuracio->altres_servidors[posicio].id;
}

int encuar(Central *central, int posicio) {
    if (central->estat[posicio] != LLIURE || central->iteracions[posicio] >= ITERACIONS) {
        return -1;
    }
    central->cua[(central->inici + central->nombre) % (central->configuracio->nombre_altres_servidors + 1)] = posicio;
    ++central->nombre;
    central->estat[posicio] = EN_CUA;
    registrar_local("ENCUAR", id_node(central, posicio));
    return 0;
}

int gestionar_peticio(Central *central, int posicio, Trama *trama) {
    if (trama->tipus == REQUEST && trama->valor == central->iteracions[posicio] + 1) {
        return encuar(central, posicio);
    }
    if (trama->tipus == DONE && trama->valor == 0 && central->estat[posicio] == LLIURE && central->iteracions[posicio] == ITERACIONS) {
        central->estat[posicio] = ACABAT;
        ++central->acabats;
        return 0;
    }
    return -1;
}

// Una peticio pot haver creuat l'UPDATE de replicacio. Cal encuar-la abans de continuar esperant l'ACK, en lloc de confondre-la amb la confirmacio.
int esperar_confirmacio(Central *central, int posicio) {
    Trama trama = {0};

    while (rebre_trama(central->connexions[posicio], id_node(central, posicio), &trama) == 0) {
        if (trama.tipus == ACK) {
            return 0;
        }
        if (gestionar_peticio(central, posicio, &trama) != 0) {
            return -1;
        }
    }
    return -1;
}

int replicar(Central *central, int escriptor) {
    int i = 0;

    for (i = 0; i < central->configuracio->nombre_altres_servidors; ++i) {
        if (i == escriptor) {
            continue;
        }
        if (enviar_trama(central->connexions[i], id_node(central, i), UPDATE, central->configuracio->valor_local) != 0 || esperar_confirmacio(central, i) != 0) {
            return -1;
        }
    }
    return 0;
}

void mostrar_iteracio(Configuracio *configuracio, int iteracio, int valor_llegit) {
    printf("\n---------------- RESULTAT ----------------\n");
    if (configuracio->tipus == READ_WRITE) {
        printf("Iteracio %2d/%d | Llegit: %3d | Escrit: %3d (confirmat)\n", iteracio, ITERACIONS, valor_llegit, configuracio->valor_local);
    } else {
        printf("Iteracio %2d/%d | Llegit: %3d | Nomes lectura\n", iteracio, ITERACIONS, valor_llegit);
    }
    printf("------------------------------------------\n\n");
}

int torn_local(Central *central) {
    Configuracio *configuracio = central->configuracio;
    int posicio = configuracio->nombre_altres_servidors, valor_llegit = configuracio->valor_local;

    registrar_local("READ", configuracio->valor_local);
    if (configuracio->tipus == READ_WRITE) {
        ++configuracio->valor_local;
        registrar_local("UPDATE", configuracio->valor_local);
        if (replicar(central, -1) != 0) {
            return -1;
        }
    }
    mostrar_iteracio(configuracio, central->iteracions[posicio] + 1, valor_llegit);
    return 0;
}

int torn_remot(Central *central, int posicio) {
    Configuracio *configuracio = central->configuracio;
    int connexio = central->connexions[posicio];
    int id = id_node(central, posicio);
    Trama trama = {0};

    if (enviar_trama(connexio, id, GRANT, central->iteracions[posicio] + 1) != 0 || esperar(connexio, id, READ, &trama) != 0 || enviar_trama(connexio, id, VALUE, configuracio->valor_local) != 0 || rebre_trama(connexio, id, &trama) != 0) {
        return -1;
    }
    if (trama.tipus == UPDATE) {
        configuracio->valor_local = trama.valor;
        registrar_local("UPDATE", configuracio->valor_local);
        if (replicar(central, posicio) != 0 || enviar_trama(connexio, id, ACK, 0) != 0 || rebre_trama(connexio, id, &trama) != 0) {
            return -1;
        }
    }
    if (trama.tipus != RELEASE) {
        return -1;
    }
    return 0;
}

int temps_espera(int venciment) {
    int ara = temps_ms();

    if (ara >= venciment) {
        return 0;
    }
    if (venciment - ara > 1000) {
        return 1000;
    }
    return venciment - ara;
}

int executar_coordinador(Central *central, int *preparats) {
    int i = 0, j = 0, posicio = 0, espera = 0, resultat_torn = 0;
    int participants = central->configuracio->nombre_altres_servidors, seguent = 0;
    int connexio = SOCKET_INVALID;
    Trama trama = {0};

    printf("Esperant %d nodes...\n", participants);
    for (i = 0; i < participants; ++i) {
        connexio = acceptar_connexio(central->escolta);
        if (connexio == SOCKET_INVALID) {
            return -1;
        }
        if (esperar(connexio, -1, READY, &trama) != 0) {
            tancar_socket(connexio);
            return -1;
        }
        for (j = 0; j < participants; ++j) {
            if (id_node(central, j) == trama.valor) {
                break;
            }
        }
        if (j == participants || central->connexions[j] != SOCKET_INVALID) {
            fprintf(stderr, "READY d'un ID desconegut o repetit.\n");
            tancar_socket(connexio);
            return -1;
        }
        central->connexions[j] = connexio;
        printf("Node %d connectat (%d/%d).\n", trama.valor, i + 1, participants);
    }
    tancar_socket(central->escolta);
    central->escolta = SOCKET_INVALID;
    for (i = 0; i < participants; ++i) {
        if (enviar_trama(central->connexions[i], id_node(central, i), START, 0) != 0) {
            return -1;
        }
    }
    printf("Tots els nodes estan preparats. Comencem!\n\n");
    seguent = temps_ms();
    while (central->acabats < participants + 1) {
        espera = 1000;
        if (central->nombre > 0) {
            espera = 0;
        } else if (central->estat[participants] == LLIURE) {
            espera = temps_espera(seguent);
        }
        if (esperar_sockets(central->connexions, participants, espera, preparats) < 0) {
            return -1;
        }

        // FIFO segons l'ordre en que el coordinador rep les peticions. Si diversos sockets estan preparats, es desempata per la llista.
        for (i = 0; i < participants; ++i) {
            if (!preparats[i]) {
                continue;
            }
            if (rebre_trama(central->connexions[i], id_node(central, i), &trama) != 0 || gestionar_peticio(central, i, &trama) != 0) {
                return -1;
            }
        }
        
        // El 0 te el seu propi temporitzador i entra a la mateixa cua.
        if (central->estat[participants] == LLIURE && temps_ms() >= seguent) {
            if (central->iteracions[participants] == ITERACIONS) {
                central->estat[participants] = ACABAT;
                ++central->acabats;
                registrar_local("DONE", 0);
            } else {
                registrar_local("REQUEST", central->iteracions[participants] + 1);
                if (encuar(central, participants) != 0) {
                    return -1;
                }
            }
        }
        if (!central->nombre) {
            continue;
        }
        posicio = central->cua[central->inici];
        central->inici = (central->inici + 1) % (participants + 1);
        --central->nombre;
        central->estat[posicio] = ACTIU;
        printf("\n==========================================\n"
               "TORN DEL NODE %d | ITERACIO %d/%d\n"
               "==========================================\n", id_node(central, posicio), central->iteracions[posicio] + 1, ITERACIONS);
        registrar_local("CONCEDIR", id_node(central, posicio));
        if (posicio == participants) {
            resultat_torn = torn_local(central);
        } else {
            resultat_torn = torn_remot(central, posicio);
        }
        if (resultat_torn != 0) {
            return -1;
        }
        ++central->iteracions[posicio];
        central->estat[posicio] = LLIURE;
        registrar_local("ALLIBERAR", id_node(central, posicio));
        printf("-------------- FINAL DEL TORN ------------\n\n");
        if (posicio == participants) {
            seguent = temps_ms() + 1000;
        }
    }
    for (i = 0; i < participants; ++i) {
        if (enviar_trama(central->connexions[i], id_node(central, i), STOP, 0) != 0 || esperar_confirmacio(central, i) != 0) {
            return -1;
        }
    }
    return 0;
}

int coordinador(Configuracio *configuracio) {
    Central central = {0};
    int i = 0, resultat = -1, participants = configuracio->nombre_altres_servidors;
    int *preparats = NULL;

    central.configuracio = configuracio;
    central.escolta = SOCKET_INVALID;
    central.connexions = malloc((participants + 1) * sizeof(int));
    central.estat = calloc(participants + 1, sizeof(int));
    central.iteracions = calloc(participants + 1, sizeof(int));
    central.cua = malloc((participants + 1) * sizeof(int));
    preparats = malloc((participants + 1) * sizeof(int));
    if (central.connexions != NULL) {
        for (i = 0; i < participants; ++i) {
            central.connexions[i] = SOCKET_INVALID;
        }
    }
    if (central.connexions != NULL && central.estat != NULL && central.iteracions != NULL && central.cua != NULL && preparats != NULL) {
        central.escolta = crear_socket_servidor(configuracio);
        if (central.escolta != SOCKET_INVALID) {
            resultat = executar_coordinador(&central, preparats);
        }
    } else {
        fprintf(stderr, "No s'ha pogut reservar memoria.\n");
    }
    tancar_socket(central.escolta);
    if (central.connexions != NULL) {
        for (i = 0; i < participants; ++i) {
            tancar_socket(central.connexions[i]);
        }
    }
    free(central.connexions);
    free(central.estat);
    free(central.iteracions);
    free(central.cua);
    free(preparats);
    return resultat;
}

int executar_participant(Configuracio *configuracio, int connexio) {
    int iteracio = 0, demanat = 0, acabat = 0;
    int preparat = 0, seguent = 0, espera = 0, valor_llegit = 0;
    Trama trama = {0};

    printf("Connectat al coordinador. Esperant que tots estiguin preparats...\n");
    if (enviar_trama(connexio, 0, READY, configuracio->propi.id) != 0 || esperar(connexio, 0, START, &trama) != 0) {
        return -1;
    }
    printf("Tots els nodes estan preparats. Comencem!\n\n");
    seguent = temps_ms();
    for (;;) {
        if (!demanat && !acabat && temps_ms() >= seguent) {
            if (iteracio == ITERACIONS) {
                if (enviar_trama(connexio, 0, DONE, 0) != 0) {
                    return -1;
                }
                acabat = 1;
                printf("\nIteracions acabades. Esperant que acabin els altres nodes...\n");
            } else {
                printf("\n==========================================\n"
                       "NODE %d | INICI ITERACIO %d/%d\n"
                       "==========================================\n", configuracio->propi.id, iteracio + 1, ITERACIONS);
                if (enviar_trama(connexio, 0, REQUEST, iteracio + 1) != 0) {
                    return -1;
                }
                demanat = 1;
            }
        }
        espera = 1000;
        if (demanat == 0 && acabat == 0) {
            espera = temps_espera(seguent);
        }
        if (esperar_sockets(&connexio, 1, espera, &preparat) < 0) {
            return -1;
        }
        if (!preparat) {
            continue;
        }
        if (rebre_trama(connexio, 0, &trama) != 0) {
            return -1;
        }
        if (trama.tipus == UPDATE) {
            configuracio->valor_local = trama.valor;
            registrar_local("APLICAR", configuracio->valor_local);
            if (enviar_trama(connexio, 0, ACK, 0) != 0) {
                return -1;
            }
        } else if (trama.tipus == READ) {
            if (enviar_trama(connexio, 0, VALUE, configuracio->valor_local) != 0) {
                return -1;
            }
        } else if (trama.tipus == GRANT) {
            if (!demanat || acabat || trama.valor != iteracio + 1) {
                return -1;
            }
            ++iteracio;
            if (enviar_trama(connexio, 0, READ, 0) != 0 || esperar(connexio, 0, VALUE, &trama) != 0) {
                return -1;
            }
            configuracio->valor_local = trama.valor;
            registrar_local("READ", configuracio->valor_local);
            valor_llegit = configuracio->valor_local;
            if (configuracio->tipus == READ_WRITE) {
                registrar_local("CALCULAR", configuracio->valor_local + 1);
                if (enviar_trama(connexio, 0, UPDATE, configuracio->valor_local + 1) != 0 || esperar(connexio, 0, ACK, &trama) != 0) {
                    return -1;
                }
                ++configuracio->valor_local;
            }
            if (enviar_trama(connexio, 0, RELEASE, 0) != 0) {
                return -1;
            }
            mostrar_iteracio(configuracio, iteracio, valor_llegit);
            demanat = 0;
            seguent = temps_ms() + 1000;
        } else if (trama.tipus == STOP && acabat) {
            if (enviar_trama(connexio, 0, ACK, 0) != 0) {
                return -1;
            }
            return 0;
        } else {
            return -1;
        }
    }
}

int participant(Configuracio *configuracio) {
    int i = 0, resultat = -1;
    int connexio = SOCKET_INVALID;
    Servidor *central = NULL;

    for (i = 0; i < configuracio->nombre_altres_servidors; ++i) {
        if (configuracio->altres_servidors[i].id == 0) {
            central = &configuracio->altres_servidors[i];
        }
    }
    if (central == NULL) {
        return -1;
    }
    printf("Connectant al coordinador %s:%d...\n", central->ip, central->port);
    connexio = connectar_servidor(central);
    if (connexio != SOCKET_INVALID) {
        resultat = executar_participant(configuracio, connexio);
        tancar_socket(connexio);
    }
    return resultat;
}

int main(int quantitat_arguments, char *arguments[]) {
    Configuracio configuracio = {0};
    int resultat = 0;

    setvbuf(stdout, NULL, _IOLBF, 0);
    if (llegir_arguments(quantitat_arguments, arguments, &configuracio) != 0) {
        return 1;
    }
    mostrar_configuracio(&configuracio);
    if (iniciar_xarxa(configuracio.propi.id) != 0) {
        free(configuracio.altres_servidors);
        return 1;
    }
    if (configuracio.propi.id == 0) {
        resultat = coordinador(&configuracio);
    } else {
        resultat = participant(&configuracio);
    }
    if (resultat == 0) {
        printf("\n==========================================\n"
               "RESULTAT FINAL\n"
               "==========================================\n"
               "Node %d: execucio completada.\nIteracions: %d/%d\nValor final compartit: %d\n"
               "==========================================\n", configuracio.propi.id, ITERACIONS, ITERACIONS, configuracio.valor_local);
    } else {
        fprintf(stderr, "Execucio interrompuda: error de xarxa o de protocol.\n");
    }
    free(configuracio.altres_servidors);
    if (resultat != 0) {
        return 1;
    }
    return 0;
}
