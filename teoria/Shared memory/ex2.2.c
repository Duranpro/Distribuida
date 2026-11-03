//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

typedef struct {
    int num;
    int flag; // 0 indicates Program 1 can write, 1 indicates Program 2 can write
} Communication;


int main() {
    int memid;
    Communication *com;

    key_t key = ftok("ex2.1.c", 'B');

    memid = shmget(key, sizeof(Communication), IPC_EXCL | 0600);

    if (memid > 0) {
        com = (Communication *)shmat(memid, 0, 0);

        while (com->num != -1) {
            if (com->flag == 1) { // Program 2 can read and write
                printf("Program 2 has read: %d\n", com->num);
                printf("Program 2, enter a new number (or -1 to exit): ");
                scanf("%d", &com->num);
                com->flag = 0; // Change flag so Program 1 can read and write
            }
        }

        shmdt(com);
    } else {
        printf("Shared memory region already exists\n");
    }

    return 0;
}
