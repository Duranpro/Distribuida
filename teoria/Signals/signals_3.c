//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

void despedida(int signum){
    printf("\nI'm in the signal handler %d\n", signum);
    //....
    // raise(SIGINT);

    alarm(5);
}

int main(){

    signal(SIGALRM, despedida);

    alarm(5);

    while(1){
        printf("I'm process %d and I'm waiting\n", getpid());
        pause();
    }


}