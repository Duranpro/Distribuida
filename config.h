#ifndef CONFIG_H
#define CONFIG_H

#define ITERACIONS 10

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
    Servidor *altres_servidors;
    int nombre_altres_servidors;
} Configuracio;

int llegir_arguments(int quantitat_arguments, char *arguments[], Configuracio *configuracio);
void mostrar_configuracio(Configuracio *configuracio);

#endif