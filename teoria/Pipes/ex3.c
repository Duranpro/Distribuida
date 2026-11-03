//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

//Parent->Multiple Children
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define NUM_CHILDREN 3

int main() {
    int fd[2];
    pid_t pid;
    char buffer[100];

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    for (int i = 0; i < NUM_CHILDREN; ++i) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        } else if (pid == 0) { // Child process
            close(fd[1]); // Close the write end
            read(fd[0], buffer, 100);
            printf("Child %d received: %s\n", i + 1, buffer);
            close(fd[0]);
            exit(0); // Exit after reading
        }
    }

    // Parent process
    close(fd[0]); // Close the read end
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        char *message;
        asprintf(&message, "Hello to child %d", i + 1);
        write(fd[1], message, strlen(message) + 1);
        free(message);
    }
    close(fd[1]);

    // Wait for all children to finish
    for (int i = 0; i < NUM_CHILDREN; ++i) {
        wait(NULL);
    }

    return 0;
}
