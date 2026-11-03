// Two threads running at once
//
// One thread writes 'x' forever and main() writes 'o', so you can watch the
// scheduler interleave them. Ctrl+C to stop.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

void * write_xs(__attribute__((unused)) void *unused){

    while (1){
      write(STDERR_FILENO, "x", 1);
    }

    return NULL;
}

int main() {
    pthread_t thread_id;
    int status;

    status = pthread_create(&thread_id, NULL, write_xs, NULL);

    if (status != 0) {
        exit(EXIT_FAILURE);
    }

    while (1){
        write(STDERR_FILENO, "o", 1);
    }

    return 0;
}

