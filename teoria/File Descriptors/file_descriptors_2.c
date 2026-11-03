//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************
#define _GNU_SOURCE
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(){
    char string[100];
    char *buffer;
    int fd_file;
    int n;

    write(STDOUT_FILENO, "String:", 7);
    n = read(STDIN_FILENO, string, 100);
    string[n-1] = '\0';
    asprintf(&buffer, "Input string: [%s] %ld characters\n", string, strlen(string));
    write(STDOUT_FILENO, buffer, strlen(buffer));
    free(buffer);

    //Create output file
    fd_file = open ("output.txt", O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if(fd_file == -1){
        asprintf(&buffer, "Cannot create the file!\n");
        write(STDERR_FILENO, buffer, strlen(buffer));
        free(buffer);
    }else{
        // Redirect stdout to file (close the stdout file descriptor)
        //dup2(fd_file, STDOUT_FILENO);

        asprintf(&buffer, "File created with file descriptor = %d\n", fd_file);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);

        // Write content to file
        write(fd_file, string, strlen(string));

        // Close the opened file
        close(fd_file);
    }

    return(0);
}
