//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************
// 
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
    int n;

    write(1, "String:", 7);
    n = read(0, string, 100);
    string[n-1] = '\0';
    asprintf(&buffer, "Input string: [%s] %ld characters\n", string, strlen(string));
    write(1, buffer, strlen(buffer));
    free(buffer);

    return(0);
}