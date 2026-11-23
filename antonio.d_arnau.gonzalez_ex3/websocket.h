#ifndef WEBSOCKET_H
#define WEBSOCKET_H

/* La clau ha de tenir els 24 caracters de Sec-WebSocket-Key. */
void acceptar_websocket(char *clau, char resposta[29]);

#endif
