#ifndef XARXA_H
#define XARXA_H
#include "config.h"
#define SOCKET_INVALID (-1)

/* 12 bytes: tipus, valor i Lamport, tots en ordre de xarxa. */
typedef enum {
    READY = 1, START, GRANT, READ, VALUE, UPDATE, ACK, RELEASE, STOP,
    REQUEST, DONE
} TipusTrama;
typedef struct { TipusTrama tipus; int valor; } Trama;

int iniciar_xarxa(int id);
void tancar_socket(int connexio);
void pausa_ms(int durada);
int temps_ms(void);
int esperar_sockets(int *connexions, int nombre, int espera_ms, int *preparats);
int crear_socket_servidor(Configuracio *configuracio);
int acceptar_connexio(int servidor);
int connectar_servidor(Servidor *servidor);
int enviar_trama(int connexio, int desti, TipusTrama tipus, int valor);
int rebre_trama(int connexio, int origen, Trama *trama);
void registrar_local(char *accio, int valor);
#endif
