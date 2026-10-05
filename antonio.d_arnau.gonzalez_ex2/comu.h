#ifndef COMU_H
#define COMU_H

/* Els noms de protocol coincideixen amb els apunts. */
enum { IDENTITAT = 1, INICI, FI, TESTIMONI, REQUEST, ACK, RELEASE };

typedef struct {
    int tipus, origen, temps;
} Missatge;

typedef struct {
    int valor;
} RellotgeLamport;

typedef struct {
    int valors[3], identificador;
} RellotgeDirecte;

void avancar_lamport(RellotgeLamport *rellotge);
void rebre_lamport(RellotgeLamport *rellotge, int temps);
void avancar_directe(RellotgeDirecte *rellotge);
void rebre_directe(RellotgeDirecte *rellotge, int origen, int temps);
int pot_entrar(RellotgeDirecte *rellotge, int *cua, int *confirmacions);
int crear_servidor(int port);
int connectar(int port);
int enviar(int connexio, int tipus, int origen, int temps);
int rebre(int connexio, Missatge *missatge);

#endif
