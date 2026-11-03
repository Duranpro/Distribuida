//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

//Parent->Child
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    pid_t pid;
    char *message;
    char buffer[100];

    asprintf(&message, "Hello from parent");

    if (pipe(fd) == -1) {
        perror("pipe");
        free(message);
        return 1;
    }

    pid = fork();

    if (pid < 0) {
        
        perror("fork");
        free(message);
        return 1;

    } else if (pid == 0) { // Child process

        close(fd[1]); // Close the write end
        read(fd[0], buffer, 100);
        printf("The child received: %s\n", buffer);
        close(fd[0]);

    } else { // Parent process

        close(fd[0]); // Close the read end
        write(fd[1], message, strlen(message) + 1);
        close(fd[1]);
        wait(NULL); // Wait for the child to finish

    }

    free(message);
    return 0;
}
