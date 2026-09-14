#ifndef CONFIG_H
#define CONFIG_H

#define MAX_SERVIDORS 5

typedef enum {
    READ_ONLY,
    READ_WRITE
} TipusServidor;

typedef struct {
    int id;
    char *ip;
    int port;
} Servidor;

typedef struct {
    Servidor propi;
    TipusServidor tipus;
    int valor_local;
    Servidor altres_servidors[MAX_SERVIDORS];
    int nombre_altres_servidors;
} Configuracio;

int llegir_arguments(int argc, char *argv[], Configuracio *configuracio);

void mostrar_configuracio(Configuracio *configuracio);

#endif