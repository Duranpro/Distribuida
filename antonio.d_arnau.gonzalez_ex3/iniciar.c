#define _POSIX_C_SOURCE 200809L
#include "comu.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t fills[NODES + 1] = {0};
    sigset_t senyals, anterior;
    int i = 0, error = 0, senyal = 0;
    char *args_replica[] = {"./replica", NULL, NULL};
    char *args_web[] = {"./web", NULL};

    sigemptyset(&senyals);
    sigaddset(&senyals, SIGINT);
    sigaddset(&senyals, SIGTERM);
    sigaddset(&senyals, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &senyals, &anterior) < 0) {
        perror("sigprocmask");
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
            if (sigprocmask(SIG_SETMASK, &anterior, NULL) < 0) {
                perror("sigprocmask");
                _exit(1);
            }
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

    while (!error) {
        if (sigwait(&senyals, &senyal) != 0) {
            error = 1;
        } else if (senyal == SIGINT || senyal == SIGTERM) {
            break;
        } else if (waitpid(-1, NULL, WNOHANG) != 0) {
            /* Si un servidor acaba, aturem la resta. */
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
