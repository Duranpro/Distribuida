#ifndef COMU_H
#define COMU_H

#define PORT_BASE 8200
#define PORT_WEB 8080
#define NODES 7
#define POSICIONS 100
#define MAX_OPERACIONS 100
#define PERIODE 10

enum { TRANSACCIO = 1, APLICAR, COPIAR, ESTAT, MONITOR, DISTRIBUIR, ENTRAR, TESTIMONI };
enum { LECTURA = 1, ESCRIPTURA };

typedef struct {
    int tipus, node, quantitat;
    int operacions[MAX_OPERACIONS][3];
} Peticio;

typedef struct {
    int versio, quantitat, escriptures, ultima;
    int claus[POSICIONS];
    int valors[POSICIONS];
    int lectures[MAX_OPERACIONS];
} Estat;

extern char *noms[NODES];

int crear_servidor(int port);
int connectar(int port);
int enviar(int fd, int *dades, int quantitat);
int rebre(int fd, int *dades, int quantitat);
int demanar(int node, Peticio *p, Estat *estat);
int enviar_estat(int fd, Estat *estat);
int rebre_estat(int fd, Estat *estat);
#endif
