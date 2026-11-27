#define _POSIX_C_SOURCE 200809L
#include "comu.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int avisos[2];

void avisar(int senyal) {
    unsigned char avis = senyal;
    int error = errno;

    /* Si la canonada es plena, ja hi ha avisos pendents d'aturada. */
    while (write(avisos[1], &avis, sizeof(avis)) < 0 && errno == EINTR) {
        continue;
    }
    errno = error;
}

int main(void) {
    pid_t fills[NODES + 1] = {0};
    struct sigaction accio = {0};
    unsigned char senyal = 0;
    ssize_t llegits = 0;
    int i = 0, error = 0;
    char *args_replica[] = {"./replica", NULL, NULL};
    char *args_web[] = {"./web", NULL};

    if (pipe(avisos) < 0) {
        perror("pipe");
        return 1;
    }
    if (fcntl(avisos[1], F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl");
        return 1;
    }
    accio.sa_handler = avisar;
    accio.sa_flags = SA_NOCLDSTOP;
    sigemptyset(&accio.sa_mask);
    if (sigaction(SIGINT, &accio, NULL) < 0 || sigaction(SIGTERM, &accio, NULL) < 0 || sigaction(SIGCHLD, &accio, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    for (i = 0; i <= NODES; i++) {
        fills[i] = fork();
        if (fills[i] < 0) {
            perror("fork");
            error = 1;
            break;
        }
        if (fills[i] == 0) {
            close(avisos[1]);
            close(avisos[0]);
            if (i == NODES) {
                execv(args_web[0], args_web);
            } else {
                args_replica[1] = noms[NODES - 1 - i];
                execv(args_replica[0], args_replica);
            }
            perror("execv");
            _exit(1);
        }
    }

    if (!error) {
        do {
            llegits = read(avisos[0], &senyal, sizeof(senyal));
        } while (llegits < 0 && errno == EINTR);
        /* Si un servidor acaba, aturem la resta i indiquem l'error. */
        error = llegits != sizeof(senyal) || senyal == SIGCHLD;
    }
    for (i = 0; i <= NODES; i++) {
        if (fills[i] > 0) {
            kill(fills[i], SIGTERM);
        }
    }
    for (i = 0; i <= NODES; i++) {
        if (fills[i] > 0) {
            while (waitpid(fills[i], NULL, 0) < 0 && errno == EINTR) {
                continue;
            }
        }
    }
    close(avisos[1]);
    close(avisos[0]);
    return error;
}
