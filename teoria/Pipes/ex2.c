//************
//
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau
//
//************

// A pipe between parent and child
//
// The child writes a message into fd[1] and the parent reads it from fd[0].
// Remember: fd[0] is the read end, fd[1] is the write end.

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(void)
{
    // The 2 file descriptors that will form the pipe
    int fd[2];
    char buffer[256];
    int n, pid;

    // Create a pipe
    if (pipe(fd) == -1)
    {
        write(STDERR_FILENO, "Error creating pipe\n", 20);
        exit(EXIT_FAILURE);
    }
    
    // Fork the process
    pid = fork();
    // Check the pid to know if we are the parent or the child process
    switch (pid)
    {
        case -1: // Error forking
            break;

        case 0: // CHILD process
            // Close the read end of the pipe
            close(fd[0]);
            // Write through the write end file descriptor
            n = write(fd[1], "Pipe message\n", 13);
            break;

        default: // PARENT process
            write(STDOUT_FILENO, "Waiting for the child process to finish!\n", 41);
            sleep(5);
            // Wait for the child to finish
            wait(&pid);
            // Close the write end file descriptor
            close(fd[1]);
            // Read from the read end of the pipe
            n = read(fd[0], buffer, sizeof(buffer));
            // Show the received message (only if the read succeeded)
            if (n > 0)
            {
                write(STDOUT_FILENO, buffer, n);
            }
            break;
    }
    return 0;
}
