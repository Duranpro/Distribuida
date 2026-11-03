//************
//
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau
//
//************

// Message queue client
//
// Sends two numbers as a message of type 1 and prints the sum that comes back
// as type 2. Entering 0 stops both programs. Run from this directory.

#define _GNU_SOURCE
#include <stdio.h>      // For asprintf
#include <stdlib.h>     // For exit, free and strtol
#include <string.h>     // For strlen
#include <unistd.h>     // For read and write
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

// Prompts and reads one integer. Returns 0 on EOF or read error.
static int read_number(const char *prompt, int *out)
{
    char buffer[64];
    int n;

    write(STDOUT_FILENO, prompt, strlen(prompt));

    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    // Always check the result: 0 is EOF (Ctrl+D), -1 is an error.
    // Without this, buffer[n] would write out of bounds.
    if (n <= 0)
    {
        return 0;
    }
    buffer[n] = '\0';

    *out = (int) strtol(buffer, NULL, 10);
    return 1;
}

int main(void)
{
    key_t key;
    int id_queue;
    Message1 M1;
    Message2 M2;
    char *buffer;
    int n;

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

    M1.id_msg = 1;      // Requests travel as type 1
    M1.n1 = 1;

    while (M1.n1 != 0)
    {
        if (!read_number("Enter num1:", &M1.n1))
        {
            break;
        }
        if (M1.n1 == 0)   // Stop before asking for num2
        {
            break;
        }
        if (!read_number("Enter num2:", &M1.n2))
        {
            break;
        }

        // msgsz counts the payload only, NOT the id_msg field.
        // No IPC_NOWAIT: if the queue is full we want to wait, not lose the
        // message and then block forever waiting for a reply that never comes.
        // Flag 0 blocks if the queue is full. IPC_NOWAIT would instead
        // return an error immediately, losing the request.
        // msgrcv's type argument selects what to pick up:
        //   0  the first message in the queue, whatever its type
        //   X  that specific type only
        //  -X  any type from 1 up to X
        if (msgsnd(id_queue, &M1, sizeof(int) * 2, 0) == -1)
        {
            write(STDERR_FILENO, "Error sending message\n", 22);
            break;
        }
        write(STDOUT_FILENO, "Sending petition...\n", 20);

        // Replies travel as type 2, so we only pick those up
        if (msgrcv(id_queue, &M2, sizeof(int), 2, 0) == -1)
        {
            write(STDERR_FILENO, "Error receiving message\n", 24);
            break;
        }

        n = asprintf(&buffer, "%d+%d = %d\n\n", M1.n1, M1.n2, M2.result);
        write(STDOUT_FILENO, buffer, n);
        free(buffer);
    }

    // Tell Adder.c to stop, then destroy the queue
    M1.id_msg = 1;
    M1.n1 = 0;
    M1.n2 = 0;
    msgsnd(id_queue, &M1, sizeof(int) * 2, 0);

    write(STDOUT_FILENO, "Program finished\n", 17);
    msgctl(id_queue, IPC_RMID, NULL);

    return 0;
}
