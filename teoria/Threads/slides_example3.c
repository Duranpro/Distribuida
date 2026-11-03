// Passing a parameter by pointer
//
// The thread modifies the caller's x directly, because threads share memory.
// With fork() each process would get its own copy instead.

/**********************************************************************\
*                Copyright (C) XaviC, 2013.                  *
*               Thread parameters                            *
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *threadFunc(void *arg)
{
    int *number = (int *)arg;

    *number = *number + 5;

    return (void *)arg;
}

int main ()
{
    pthread_t t1;
    void *res;
    int status, x;
    int n;
    char buffer[128];

    x = 7;
    n = snprintf(buffer, sizeof(buffer), "Value of x=%d\n", x);
    write(STDOUT_FILENO, buffer, n);
    status = pthread_create(&t1, NULL, threadFunc, &x);
    if(status != 0)
    {
        write(STDERR_FILENO, "pthread_create\n", 15);
        exit(EXIT_FAILURE);
    }
    write(STDOUT_FILENO, "Message from main()\n", 20);
    status = pthread_join(t1, &res);
    if(status != 0)
    {
        write(STDERR_FILENO, "pthread_join\n", 13);
        exit(EXIT_FAILURE);
    }
    n = snprintf(buffer, sizeof(buffer), "Thread returns %lX\n", (long)res);
    write(STDOUT_FILENO, buffer, n);
    n = snprintf(buffer, sizeof(buffer), "Value of x=%d\n", x);
    write(STDOUT_FILENO, buffer, n);
    exit(EXIT_SUCCESS);
}
