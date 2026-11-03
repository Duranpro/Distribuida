//************
//
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau
//
//************

// Shared memory writer
//
// Creates the region and stores each number the user types, until -1.
// Run together with read.c, in two shells, from this directory.

#include <stdint.h>
#include <stdlib.h>     // For strtol
#include <string.h>     // For strlen
#include <unistd.h>     // For read and write
#include <sys/ipc.h>
#include <sys/shm.h>

int main(void)
{
    int memid;
    int *num;
    int n;
    char buffer[64];

    // Create a shared memory region.
    // The key must be the SAME one read.c uses, so both processes find the
    // same region. ftok() derives it from a file both of them can reach.
    // IPC_PRIVATE would create a region no other process can look up by name.
    memid = shmget(ftok("ex1.1.c", 12), sizeof(int),
                   IPC_CREAT | IPC_EXCL | 0600);

    // shmget() reports failure with -1. Note that 0 is a VALID id.
    if (memid != -1)
    {
        // Link this memory to a local address (num)
        num = shmat(memid, NULL, 0);

        // shmat() reports failure with (void *) -1, never with NULL
        if (num != (void *) -1)
        {
            // Set the value of the memory until -1 is passed
            *num = 0;
            while (*num != -1)
            {
                write(STDOUT_FILENO, "Number: ", 8);

                n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                // Always check the result: 0 is EOF (Ctrl+D), -1 is an error.
                // Without this, buffer[n] would write out of bounds.
                if (n <= 0)
                {
                    break;
                }
                buffer[n] = '\0';

                *num = (int) strtol(buffer, NULL, 10);
            }

            // Unlink the memory
            shmdt(num);
        }
        else
        {
            write(STDERR_FILENO, "Error: Can't link memory\n", 25);
        }

        // Free the shared memory
        shmctl(memid, IPC_RMID, NULL);
    }
    else
    {
        // Left over from an earlier run? Remove it with:
        //   ipcs -m          (list the regions)
        //   ipcrm -m <id>    (or run ./memory_cleaner_v2.sh)
        write(STDERR_FILENO, "Error: Existing region\n", 23);
    }

    return 0;
}
