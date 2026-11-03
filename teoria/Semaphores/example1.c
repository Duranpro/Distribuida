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

    // Create semaphore (private)
    if (SEM_constructor(&sem) < 0) {
        perror("SEM_constructor");
        return 1;
    }

    // Initialize with value 1
    if (SEM_init(&sem, 1) < 0) {
        perror("SEM_init");
        return 1;
    }

    printf("Semaphore created and initialized!\n");

    // Destroy semaphore
    if (SEM_destructor(&sem) < 0) {
        perror("SEM_destructor");
        return 1;
    }

    printf("Semaphore destroyed!\n");
    return 0;
}
