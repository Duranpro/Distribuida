//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define N 5 // Number of children

void printF(char* message) {
    write(STDOUT_FILENO, message, strlen(message));
}


int main() {
    char* buffer;
    pid_t pid;
    int i;

    for (i = 0; i < N; i++) {
        pid = fork();
        
        if (pid < 0) {
            printF("Fork failed\n");
            return 1;
        } else if (pid == 0) {
            // Child process
            asprintf(&buffer, "Hello from child %d! PID: %d\n", i, getpid());
            printF(buffer);
            free(buffer);
            exit(0); // Ensure the child doesn't create more children
        }
    }

    // Parent process
    for (i = 0; i < N; i++) {
        wait(NULL); // Wait for all children to finish
    }

    asprintf(&buffer, "All %d children have finished. Parent PID: %d\n", N, getpid());
    printF(buffer);
    free(buffer);

    return 0;
}
