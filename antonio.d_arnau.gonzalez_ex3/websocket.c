#include "websocket.h"
#include <stdint.h>
#include <string.h>

/* Rotacio de paraules de 32 bits requerida pel calcul SHA-1. */
uint32_t rotar(uint32_t valor, int bits) {
    return (valor << bits) | (valor >> (32 - bits));
}

/* RFC 6455, apartat 4.2.2.
   La clau HTTP validada te 24 caracters i el sufix fix en te 36.
   SHA-1 processa aquests 60 bytes en dos blocs de 64 bytes.
   Aquest calcul nomes prepara Sec-WebSocket-Accept. */
void acceptar_websocket(const char *clau, char resposta[29]) {
    const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char dades[128] = {0}, resum[20] = {0};
    uint32_t h[5] = {0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U, 0xc3d2e1f0U};
    uint32_t w[80] = {0};
    uint32_t a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, k = 0, nou = 0, grup = 0;
    int bloc = 0, i = 0, pos = 0, sortida = 0;

    memcpy(dades, clau, 24);
    memcpy(dades + 24, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
    dades[60] = 0x80;
    /* Longitud original en bits, 60 * 8 = 480 = 0x01e0. */
    dades[126] = 1;
    dades[127] = 0xe0;

    for (bloc = 0; bloc < 2; bloc++) {
        for (i = 0; i < 16; i++) {
            pos = bloc * 64 + i * 4;
            w[i] = ((uint32_t)dades[pos] << 24) |
                   ((uint32_t)dades[pos + 1] << 16) |
                   ((uint32_t)dades[pos + 2] << 8) |
                   (uint32_t)dades[pos + 3];
        }
        for (i = 16; i < 80; i++) {
            w[i] = rotar(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }
        a = h[0];
        b = h[1];
        c = h[2];
        d = h[3];
        e = h[4];
        for (i = 0; i < 80; i++) {
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5a827999U;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ed9eba1U;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8f1bbcdcU;
            } else {
                f = b ^ c ^ d;
                k = 0xca62c1d6U;
            }
            nou = rotar(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotar(b, 30);
            b = a;
            a = nou;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
    }
    for (i = 0; i < 20; i++) {
        resum[i] = (unsigned char)(h[i / 4] >> (24 - 8 * (i % 4)));
    }
    /* Codificacio Base64 dels 20 bytes del resum. */
    for (i = 0; i < 18; i += 3) {
        grup = ((uint32_t)resum[i] << 16) |
               ((uint32_t)resum[i + 1] << 8) | resum[i + 2];
        resposta[sortida++] = alphabet[(grup >> 18) & 63];
        resposta[sortida++] = alphabet[(grup >> 12) & 63];
        resposta[sortida++] = alphabet[(grup >> 6) & 63];
        resposta[sortida++] = alphabet[grup & 63];
    }
    grup = ((uint32_t)resum[18] << 16) | ((uint32_t)resum[19] << 8);
    resposta[sortida++] = alphabet[(grup >> 18) & 63];
    resposta[sortida++] = alphabet[(grup >> 12) & 63];
    resposta[sortida++] = alphabet[(grup >> 6) & 63];
    resposta[sortida++] = '=';
    resposta[sortida] = '\0';
}
