//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#include <stdio.h>
#include <pthread.h>

void* print_child_thread(void *arg __attribute__((unused))) {
    for (int i = 0; i < 10; i++) {
        printf("I am a thread\n");
    }
    return NULL;
}

int main() {
    pthread_t child_thread;
    pthread_create(&child_thread, NULL, print_child_thread, NULL);

    // Main thread
    for (int i = 0; i < 10; i++) {
        printf("I am the main thread\n");
    }

    pthread_join(child_thread, NULL);
    return 0;
}
