//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

void* child_thread(void *arg __attribute__((unused))) {
    int* result = malloc(sizeof(int));
    *result = 42;
    printf("The child thread returns 42\n");
    return (void*)result;  // Returns the result as a pointer
}

int main() {
    pthread_t thread;
    int* result;

    pthread_create(&thread, NULL, child_thread, NULL);
    pthread_join(thread, (void**)&result);  // Waits for the child thread and gets its return value

    printf("The child thread has returned %d\n", *result);
    free(result);  // Frees the dynamically allocated memory
    return 0;
}
