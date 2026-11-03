/*
 * Sistemas Operativos 2026-2027
 * Sesion 3: Threads
 * Author: Marcos Ruiz-Flores Vicente
 *
 */

#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 1
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define NUM_KEYS          26

#define SPANISH_FREQ {0.1153f, 0.0215f, 0.0402f, 0.0501f, 0.1218f, 0.0069f, 0.0177f, 0.0070f, 0.0625f, 0.0044f, 0.0002f, 0.0524f, 0.0271f, 0.0683f, 0.0868f, 0.0251f, 0.0088f, 0.0687f, 0.0798f, 0.0463f, 0.0393f, 0.0090f, 0.0002f, 0.0022f, 0.0090f, 0.0052f}


#define printFd(fd, x) write((fd), (x), strlen(x))
#define print(x)       printFd(STDOUT_FILENO, x)
 

#define MSG_SIGINT          "\n[SIGINT] Ctrl+C: cancelando los hilos en marcha...\n"
#define MSG_USO             "Uso: ./S2 <n_hilos (1-26)> <fichero_cifrado>\n"
#define MSG_ERR_NHILOS      "Error: el numero de hilos debe estar entre 1 y 26.\n"
#define MSG_ERR_ABRIR       "Error: no se puede abrir el fichero cifrado.\n"
#define MSG_ERR_TAMANO      "Error: no se puede obtener el tamano del fichero.\n"
#define MSG_HILO_PROBANDO   "[Hilo %d] probando claves %d..%d\n"
#define MSG_HILO_CLAVE      "    clave %2d -> puntuacion = %.3f\n"
#define MSG_HILO_MEJOR      "[Hilo %d] mejor local: clave %d (puntuacion = %.3f)\n\n"
#define MSG_MAIN_CANCELADO  "[main] Hilo %d CANCELADO: resultado perdido y memoria filtrada.\n"
#define MSG_SIN_RESULTADO   "\nNo hay resultado: todos los hilos fueron cancelados.\n"
#define MSG_RESULTADO       "==> Clave mas probable: %d  (puntuacion = %.3f)\n\nTexto descifrado:\n%s\n"


pthread_t *g_threads = NULL;
int g_num_threads = 0;


void handle_sigint() {
    write(STDOUT_FILENO, MSG_SIGINT, strlen(MSG_SIGINT));
    for (int i = 0; i < g_num_threads; ++i) pthread_cancel(g_threads[i]);

}


typedef struct {
    int thread_id;
    char *cipher;
    int len;
    int key_start;
    int key_end;
} DecryptArgs;

typedef struct {
    int best_key;
    float best_score;
    char  *best_text;
} DecryptResult;

void stringBuilderStrcat(char **marcos, char *so) {
    if (so == NULL) so = "";
    if (*marcos == NULL) *marcos = strdup("");
    char *joined = NULL;
    asprintf(&joined, "%s%s", *marcos, so);
    free(*marcos);
    *marcos = joined;
}

char *cesar_decrypt(char *cipher, int len, int key) {
    char *out = malloc(len + 1);
    for (int i = 0; i < len; ++i) {
        char c = (unsigned char) cipher[i];
        if (c >= 'a' && c <= 'z') out[i] = (char) ('a' + (c - 'a' - key + NUM_KEYS) % NUM_KEYS);
        else if (c >= 'A' && c <= 'Z') out[i] = (char) ('A' + (c - 'A' - key + NUM_KEYS) % NUM_KEYS);
        else out[i] = (char) c;
    }
    out[len] = '\0';
    return out;
}


float spanish_score(char *text) {

    float freq[NUM_KEYS] = SPANISH_FREQ;

    float score = 0.0f;
    for (char *p = text; *p != '\0'; ++p) {
        char c = (char) *p;
        if (c >= 'A' && c <= 'Z') c = (char) (c - 'A' + 'a');
        if (c >= 'a' && c <= 'z') score += freq[c - 'a'];
    }
    return score;
}

void *decrypt_thread(void *arg) {
    DecryptArgs *a = (DecryptArgs *) arg;
    DecryptResult *res = malloc(sizeof(DecryptResult));
    res->best_key   = -1;
    res->best_score = -1.0f;
    res->best_text  = NULL;

    char *report = NULL;
    char *seg    = NULL;

    asprintf(&seg, MSG_HILO_PROBANDO, a->thread_id, a->key_start, a->key_end - 1);
    stringBuilderStrcat(&report, seg);
    free(seg);

    for (int k = a->key_start; k < a->key_end; ++k) {
        char  *plain = cesar_decrypt(a->cipher, a->len, k);
        float score = spanish_score(plain);

        asprintf(&seg, MSG_HILO_CLAVE, k, score);
        stringBuilderStrcat(&report, seg);
        free(seg);

        if (score > res->best_score) {
            res->best_score = score;
            res->best_key   = k;
            free(res->best_text);
            res->best_text = plain;
        } else {
            free(plain);
        }
    }

    asprintf(&seg, MSG_HILO_MEJOR,a->thread_id, res->best_key, res->best_score);
    stringBuilderStrcat(&report, seg);
    free(seg);

    if (report != NULL) {
        print(report);
        free(report);
    }

    free(a);
    return res;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        print(MSG_USO);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    if (N < 1 || N > NUM_KEYS) {
        print(MSG_ERR_NHILOS);
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);

    int fd = open(argv[2], O_RDONLY);
    if (fd < 0) {
        print(MSG_ERR_ABRIR);
        return EXIT_FAILURE;
    }

    int fsize = lseek(fd, 0, SEEK_END);
    if (fsize < 0 || lseek(fd, 0, SEEK_SET) < 0) {
        print(MSG_ERR_TAMANO);
        close(fd);
        return EXIT_FAILURE;
    }

    int cap = fsize, total = 0, n, len = 0;
    char  *cipher = malloc(cap + 1);
    while (total < cap && (n = read(fd, cipher + total, cap - total)) > 0) total += n;
    close(fd);
    cipher[total] = '\0';
    len = total;

    pthread_t *threads = malloc((size_t) N * sizeof(pthread_t));

    g_threads = threads;
    g_num_threads = 0;

    int base  = NUM_KEYS / N;
    int extra = NUM_KEYS % N;
    int off   = 0;

    for (int i = 0; i < N; ++i) {
        int nkeys = base + (i < extra ? 1 : 0);

        DecryptArgs *a = malloc(sizeof(DecryptArgs));
        a->thread_id = i;
        a->cipher = cipher;
        a->len = len;
        a->key_start = off;
        a->key_end = off + nkeys;
        off += nkeys;

        pthread_create(&threads[i], NULL, decrypt_thread, a);
        g_num_threads++;
    }

    DecryptResult *best = NULL;
    for (int i = 0; i < N; ++i) {
        void *ret = NULL;
        pthread_join(threads[i], &ret);

        if (ret == PTHREAD_CANCELED) {
            char *m = NULL;
            asprintf(&m, MSG_MAIN_CANCELADO, i);
            print(m);
            free(m);
            continue;
        }

        DecryptResult *r = (DecryptResult *) ret;
        if (best == NULL || r->best_score > best->best_score) {
            if (best != NULL) {
                free(best->best_text);
                free(best);
            }
            best = r;
        } else {
            free(r->best_text);
            free(r);
        }
    }

    if (best == NULL) {
        print(MSG_SIN_RESULTADO);
        free(threads);
        free(cipher);
        return EXIT_FAILURE;
    }

    char *out = NULL;
    asprintf(&out, MSG_RESULTADO, best->best_key, best->best_score, best->best_text);
    print(out);
    free(out);
    free(best->best_text);
    free(best);
    free(threads);
    free(cipher);
    return EXIT_SUCCESS;
}
