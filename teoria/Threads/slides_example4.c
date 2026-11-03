// Threads share global variables
//
// Both the thread and main() update the global y with no synchronisation,
// while x is passed by pointer and modified in place.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int y;

static void *threadFunc (void *arg)
{
    int *number = (int *)arg;
    char buffer[128];
    int n;

    *number = *number + 5;
    y = y + 1;
    n = snprintf(buffer, sizeof(buffer), "From thread y=%d\n", y);
    write(STDOUT_FILENO, buffer, n);

    return (void *) arg;
}

int main ()
{
    pthread_t t1;
    void *res;
    int status, x;
    int n;
    char buffer[128];

    x=7;y=5;
    n = snprintf(buffer, sizeof(buffer), "[main] Initial value x=%d, y=%d\n", x, y);
    write(STDOUT_FILENO, buffer, n);

    status = pthread_create (&t1, NULL, threadFunc, &x);
    if (status != 0){
        write(STDERR_FILENO, "pthread_create\n", 15);
        exit (EXIT_FAILURE);
    }

    //int i = 0; while(i<20000) i++;
    //status = pthread_join (t1, &res);

    y = y + 2;
    n = snprintf(buffer, sizeof(buffer), "[main] Middle value y=%d\n", y);
    write(STDOUT_FILENO, buffer, n);

    status = pthread_join (t1, &res);
    if (status != 0){
        write(STDERR_FILENO, "pthread_join\n", 13);
        exit (EXIT_FAILURE);
    }
    n = snprintf(buffer, sizeof(buffer), "Thread returned %lX\n", (long) res);
    write(STDOUT_FILENO, buffer, n);

    n = snprintf(buffer, sizeof(buffer), "[main] Final value x=%d, y=%d\n", x, y);
    write(STDOUT_FILENO, buffer, n);
    exit (EXIT_SUCCESS);
}
