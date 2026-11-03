//************
//
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau
//
//************

// Which signals can be caught?
//
// Installs one handler for all 64 signals and prints what signal() returned
// for each, so you can see the ones refused (SIGKILL, SIGSTOP).

#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void kctrlc(int signum) {
    char *buffer;

    asprintf(&buffer, "I received a Ctrl+C (%d). I should abort!\n", signum);
    write(STDOUT_FILENO, buffer, strlen(buffer));
    free(buffer);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void ksigalrm(int signum) {
    char *buffer;

    asprintf(&buffer, "I received a SIGALRM (%d)\n", signum);
    write(STDOUT_FILENO, buffer, strlen(buffer));
    free(buffer);
}

void kgeneric(int signum) {
    char *buffer;

    asprintf(&buffer, "I received other signal (%d)\n", signum);
    write(STDOUT_FILENO, buffer, strlen(buffer));
    free(buffer);
}

/* This function catches all the signals and, depending which one arrives,
 * calls one function or another.
 */
void ksighandler(int signum) {
    switch (signum) {
        case SIGINT:
            kctrlc(signum);
            break;
        case SIGALRM:
            ksigalrm(signum);
            break;
        default:
            kgeneric(signum);
            break;
    }
    // Reprogram the executed signal to perform the same operation again
    // signal(signum, ksighandler);
}

int main() {
    char *buffer;
    int i;
    long result;

    /* By executing 'kill -l' we'll see that there are 64 signals... we'll reprogram all of them
     * so when one is executed, ksighandler function is executed
     */

    write(STDOUT_FILENO, "Reconfigure all signals\n", 24);
    for (i = 1; i <= 64; i++) {
        result = (long)signal(i, ksighandler);
        // The result equals to the previous configured function of that signal (0=SIG_DFL, -1=error)
        asprintf(&buffer, "Signal %i result: %ld\n", i, result);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);
    }

    // Program an alarm so SIGALRM (14) is sent to this process in 30 seconds
    alarm(30);

    /* Perform an infinite loop to see our process PID, so we'll be able to
     * send signals with 'kill -num $PID' or 'kill -name $PID'
     */
    while (1) {
        asprintf(&buffer, "I am PID %d and I wait for signals\n", getpid());
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);

        sleep(2);
    }
}
