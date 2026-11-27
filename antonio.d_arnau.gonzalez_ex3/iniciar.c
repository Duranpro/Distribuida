#define _GNU_SOURCE
#include "comu.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int avisos[2];

void avisar(int senyal) {
    unsigned char avis = senyal;
    int error = errno;

    /* Cada senyal nomes ha d'avisar una vegada. */
    signal(senyal, SIG_IGN);
    while (write(avisos[1], &avis, sizeof(avis)) < 0 && errno == EINTR) {
        continue;
    }
    errno = error;
}

int main(void) {
    pid_t fills[NODES + 1] = {0};
    pid_t vigilant = 0, acabat = 0;
    unsigned char senyal = 0;
    ssize_t llegits = 0;
    int i = 0, error = 0, estat = 0;
    char *args_replica[] = {"./replica", NULL, NULL};
    char *args_web[] = {"./web", NULL};

    if (pipe(avisos) < 0) {
        perror("pipe");
        return 1;
    }
    if (signal(SIGINT, avisar) == SIG_ERR || signal(SIGTERM, avisar) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    /* Converteix l'avís d'aturada en la sortida d'un fill que waitpid pot detectar. */
    vigilant = fork();
    if (vigilant < 0) {
        perror("fork");
        return 1;
    }
    if (vigilant == 0) {
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        close(avisos[1]);
        do {
            llegits = read(avisos[0], &senyal, sizeof(senyal));
        } while (llegits < 0 && errno == EINTR);
        if (llegits == sizeof(senyal)) {
            _exit(0);
        }
        _exit(1);
    }

    for (i = 0; i <= NODES; i++) {
        fills[i] = fork();
        if (fills[i] < 0) {
            perror("fork");
            error = 1;
            break;
        }
        if (fills[i] == 0) {
            signal(SIGINT, SIG_IGN);
            signal(SIGTERM, SIG_DFL);
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
            acabat = waitpid(-1, &estat, 0);
        } while (acabat < 0 && errno == EINTR);
        error = acabat != vigilant || estat != 0;
        if (acabat == vigilant) {
            vigilant = 0;
        }
        for (i = 0; i <= NODES; i++) {
            if (fills[i] == acabat) {
                fills[i] = 0;
                break;
            }
        }
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
    if (vigilant > 0) {
        while (waitpid(vigilant, NULL, 0) < 0 && errno == EINTR) {
            continue;
        }
    }
    return error;
}
