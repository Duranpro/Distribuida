//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "semaphore_v2.h"

int main() {
    semaphore sem;
    SEM_constructor_with_name(&sem, 1234);
    SEM_init(&sem, 1);

    if (fork() == 0) {
        // Child process
        SEM_wait(&sem);
        printf("Child: I got the lock!\n");
        sleep(2);
        printf("Child: Releasing lock.\n");
        SEM_signal(&sem);
        return 0;
    } else {
        // Parent process
        SEM_wait(&sem);
        printf("Parent: I got the lock!\n");
        sleep(2);
        printf("Parent: Releasing lock.\n");
        SEM_signal(&sem);
        wait(NULL);
    }

    SEM_destructor(&sem);
    return 0;
}
