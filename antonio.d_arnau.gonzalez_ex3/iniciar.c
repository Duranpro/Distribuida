#define _POSIX_C_SOURCE 200809L
#include "comu.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

volatile sig_atomic_t aturar = 0;

void demanar_aturada(int senyal) {
    (void)senyal;
    aturar = 1;
}

int main(void) {
    pid_t fills[NODES + 1] = {0};
    struct sigaction accio = {0};
    int i = 0, error = 0;
    char *args_replica[] = {"./replica", NULL, NULL};
    char *args_web[] = {"./web", NULL};

    accio.sa_handler = demanar_aturada;
    sigemptyset(&accio.sa_mask);
    if (sigaction(SIGINT, &accio, NULL) < 0 || sigaction(SIGTERM, &accio, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    for (i = 0; i <= NODES && !aturar; i++) {
        fills[i] = fork();
        if (fills[i] < 0) {
            perror("fork");
            error = 1;
            break;
        }
        if (fills[i] == 0) {
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

    if (!error && !aturar) {
        /* Si un servidor acaba, aturem la resta. */
        if (wait(NULL) > 0 && !aturar) {
            error = 1;
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
    return error;
}
