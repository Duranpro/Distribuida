//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#include <stdio.h>
#include "semaphore_v2.h"

int main() {
    semaphore sem;

    SEM_constructor(&sem);
    SEM_init(&sem, 1);  // Semaphore starts unlocked

    printf("Trying to enter critical section...\n");
    SEM_wait(&sem);  // lock
    printf("Inside critical section!\n");

    // do something here
    printf("Doing some work...\n");

    SEM_signal(&sem); // unlock
    printf("Exited critical section!\n");

    SEM_destructor(&sem);
    return 0;
}
