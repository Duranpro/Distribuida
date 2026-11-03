//************
//
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau
//
//************

// Parent and child have separate memory
//
// After fork() each process gets its own copy of a, b and c. The parent
// increments them; the child still prints the original values.

#define _GNU_SOURCE
#include <stdio.h>     // For asprintf function
#include <unistd.h>    // For write, getpid and fork functions
#include <string.h>    // For strlen function
#include <stdlib.h>    // For exit and free functions
#include <sys/wait.h>  // For wait function

int main()
{
    int a, b, c;
    int child;
    char *buffer;

    a=25;
    b=11;
    c=7;

    child = fork();
    if (child > 0)
    {
        // Parent process

        //wait(NULL);

        asprintf(&buffer, "I am the parent! With PID=%d (child's PID=%d)\n", getpid(), child);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);

        a++;
        b++;
        c++;

        asprintf(&buffer, "Parent: a=%d, b=%d, c=%d\n", a, b, c);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);

        exit(0);
    }
    else if (child == 0)
    {
        // Child process

        asprintf(&buffer, "I am the child with PID=%d (parent's PID=%d)\n", getpid(), getppid());
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);

        asprintf(&buffer, "Child: a=%d, b=%d, c=%d\n", a, b, c);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);

        _exit(0);
    }
    else
    {
        // Error forking
        asprintf(&buffer, "JARL!! ERROR\n");
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);

        exit(-1);
    }
}
