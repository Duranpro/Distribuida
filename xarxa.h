#ifndef XARXA_H
#define XARXA_H
#include "config.h"
#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET Socket;
#define SOCKET_INVALID INVALID_SOCKET
#else
typedef int Socket;
#define SOCKET_INVALID (-1)
#endif

/* 12 bytes: tipus, valor i Lamport, tots en ordre de xarxa. */
typedef enum {
    READY = 1, START, GRANT, READ, VALUE, UPDATE, ACK, RELEASE, STOP,
    REQUEST, DONE
} TipusTrama;
typedef struct { TipusTrama tipus; int valor; } Trama;

int iniciar_xarxa(int id);
void acabar_xarxa(void);
void tancar_socket(Socket connexio);
void pausa_ms(int durada);
int temps_ms(void);
int esperar_sockets(Socket *connexions, int nombre, int espera_ms, int *preparats);
Socket crear_socket_servidor(Configuracio *configuracio);
Socket acceptar_connexio(Socket servidor);
Socket connectar_servidor(Servidor *servidor);
int enviar_trama(Socket connexio, int desti, TipusTrama tipus, int valor);
int rebre_trama(Socket connexio, int origen, Trama *trama);
void registrar_local(char *accio, int valor);
#endif
