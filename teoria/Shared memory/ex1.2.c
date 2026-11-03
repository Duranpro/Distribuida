//************
//
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau
//
//************

// Shared memory reader
//
// Attaches to the region write.c created and shows its value on every
// keypress. It never removes the region: the writer owns it.

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>      // For asprintf
#include <stdlib.h>     // For free
#include <string.h>     // For strlen
#include <unistd.h>     // For read and write
#include <sys/ipc.h>
#include <sys/shm.h>

int main(void)
{
    int memid;
    int *num;
    int n;
    char *buffer;
    char c;

    // Open a shared memory that already exists.
    // Same key as write.c: size 0 and flags 0 mean "find it, don't create it".
    memid = shmget(ftok("ex1.1.c", 12), 0, 0);

    // shmget() reports failure with -1. Note that 0 is a VALID id.
    if (memid != -1)
    {
        // Link this memory to a local address (num)
        num = shmat(memid, NULL, 0);

        // shmat() reports failure with (void *) -1, never with NULL
        if (num != (void *) -1)
        {
            // Read the value of the memory until -1 is passed
            while (*num != -1)
            {
                n = asprintf(&buffer, "Number: %d\n", *num);
                write(STDOUT_FILENO, buffer, n);
                free(buffer);

                // Wait for a keypress before looking again
                if (read(STDIN_FILENO, &c, 1) <= 0)
                {
                    break;
                }
            }

            // Unlink the memory
            shmdt(num);
        }
        else
        {
            write(STDERR_FILENO, "Error: Can't link memory\n", 25);
        }
    }
    else
    {
        // The writer has not created it yet, or has already removed it
        write(STDERR_FILENO, "Error: Region does not exist\n", 29);
    }

    return 0;
}
