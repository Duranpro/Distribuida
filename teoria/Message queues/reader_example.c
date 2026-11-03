//************
//
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau
//
//************

// Message queue server
//
// Waits for a request of type 1, adds the two numbers and replies as type 2.
// A request with n1 == 0 is the shutdown signal.

#define _GNU_SOURCE
#include <stdio.h>      // For asprintf
#include <stdlib.h>     // For exit and free
#include <string.h>     // For strlen
#include <unistd.h>     // For write
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct
{
    long id_msg;        // Must be the first field, and strictly positive
    int n1, n2;
} Message1;

typedef struct
{
    long id_msg;
    int result;
} Message2;

int main(void)
{
    key_t key;
    int id_queue;
    Message1 M1;
    Message2 M2;
    char *buffer;
    int n;

    // Same file and same integer as EnterNumbers.c, or the keys will not match
    key = ftok("example.c", 12);
    if (key == (key_t) -1)
    {
        write(STDERR_FILENO, "Error: key\n", 11);
        exit(EXIT_FAILURE);
    }

    id_queue = msgget(key, 0600 | IPC_CREAT);
    if (id_queue == -1)
    {
        write(STDERR_FILENO, "Error creating queue\n", 21);
        exit(EXIT_FAILURE);
    }

    M2.id_msg = 2;      // Replies travel as type 2

    for (;;)
    {
        // Only pick up requests (type 1), never our own replies.
        // Checking the result matters: when EnterNumbers.c destroys the queue
        // this returns -1/EIDRM, and without the check we would loop forever
        // printing whatever happened to be left in M1.
        if (msgrcv(id_queue, &M1, sizeof(int) * 2, 1, 0) == -1)
        {
            write(STDERR_FILENO, "Queue closed, finishing\n", 24);
            break;
        }

        n = asprintf(&buffer, "Received: %d,%d\n", M1.n1, M1.n2);
        write(STDOUT_FILENO, buffer, n);
        free(buffer);

        if (M1.n1 == 0)   // Shutdown request
        {
            break;
        }

        M2.result = M1.n1 + M1.n2;

        if (msgsnd(id_queue, &M2, sizeof(int), 0) == -1)
        {
            write(STDERR_FILENO, "Error sending message\n", 22);
            break;
        }

        n = asprintf(&buffer, "Sent = %d\n", M2.result);
        write(STDOUT_FILENO, buffer, n);
        free(buffer);
    }

    // EnterNumbers.c owns the queue and removes it; we just detach by exiting
    return 0;
}
