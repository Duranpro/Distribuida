#ifndef COMU_H
#define COMU_H

#define PORT_PESANT_A 8195
#define PORT_PESANT_B 8196
#define PORT_A_BASE 8197
#define PORT_B_PRIMER 8199
/* B2 reutilitza PORT_PESANT_B despres de l'arrencada del pesant. */

/* Els noms de protocol coincideixen amb els apunts. */
#define IDENTITAT 1
#define INICI 2
#define FI 3
#define TESTIMONI 4
#define REQUEST 5
#define ACK 6
#define RELEASE 7

typedef struct {
    int tipus;
    int origen;
    int temps;
} Missatge;

typedef struct {
    int valor;
} RellotgeLamport;

typedef struct {
    int valors[3];
    int identificador;
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
