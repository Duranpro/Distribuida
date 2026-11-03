//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

//Execvp
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    pid_t pid;
    char *buffer;

    pipe(fd);
    pid = fork();

    if (pid == 0) {
        // Child process: execute 'ls'
        close(fd[0]); // Close the read end
        dup2(fd[1], 1); // Redirect stdout to the pipe
        close(fd[1]);


        
        char *args[] = {"ls", NULL};
        execvp(args[0], args);
        exit(1); // Si execvp falla



    } else {
        // Parent process: read up to 500 bytes of 'ls' output from the pipe
        close(fd[1]); // Close the write end
        buffer = (char *)malloc(500 * sizeof(char));
        int len = read(fd[0], buffer, 500);
        buffer[len] = '\0'; // Null-terminate the string for printing
        
        close(fd[0]);
        wait(NULL); // Wait for the child to finish
        
        // Write the output to stdout using write
        write(1, "Output of 'ls' (first 500 bytes):\n", strlen("Output of 'ls' (first 500 bytes):\n"));
        write(1, buffer, strlen(buffer));

        for(int i=0; i<(int)strlen(buffer) ;i++){
            write(1, &buffer[i], 1);
            write(1, "\n", 1);
        }

        free(buffer);
    }

    return 0;
}
