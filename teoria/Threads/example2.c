//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#include <stdio.h>
#include <pthread.h>

void* print_thread_id(void* arg) {
    int* id = (int*)arg;
    for (int i = 0; i < 10; i++) {
        printf("Thread %d is running\n", *id);
    }
    return NULL;
}

int main() {
    pthread_t threads[5];
    int thread_ids[5];

    for (int i = 0; i < 5; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, print_thread_id, &thread_ids[i]);
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
