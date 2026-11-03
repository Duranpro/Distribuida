// Passing data in and out of a thread
//
// pthread_create passes a string into the thread; pthread_join collects its
// length back through the void * return value.

/**********************************************************************\
*                Copyright (C) Michael Kerrisk, 2010.                  *
*                                                                      *
* This program is free software. You may use, modify, and redistribute *
* it under the terms of the GNU Affero General Public License as       *
* published by the Free Software Foundation, either version 3 or (at   *
* your option) any later version. This program is distributed without  *
* any warranty. See the file COPYING for details.                      *
\**********************************************************************/
/* threads2.c
   A simple POSIX threads example: create a thread, and then join with it.
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *threadFunc (void *arg)
{
    char *msg = (char *) arg;

    write(STDOUT_FILENO, msg, strlen(msg));

    return (void *) strlen (msg);
}

int main ()
{

    pthread_t t1;
    void *res;
    int status;
    int n;
    char buffer[128];

    status = pthread_create(&t1, NULL, threadFunc, "Hello world\n");
    if (status != 0)
    {
        write(STDERR_FILENO, "pthread_create\n", 15);
        exit(EXIT_FAILURE);
    }

    write(STDOUT_FILENO, "Message from main()\n", 20);

    status = pthread_join(t1, &res);
    if (status != 0)
    {
        write(STDERR_FILENO, "pthread_join\n", 13);
        exit(EXIT_FAILURE);
    }
    n = snprintf(buffer, sizeof(buffer), "Thread returned %ld\n", (long) res);
    write(STDOUT_FILENO, buffer, n);

    exit(EXIT_SUCCESS);
}
