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

typedef struct {
    char nom[200];
    int num;
    int flag; // 0 indicates Program 1 can write, 1 indicates Program 2 can write
} Communication;

int main() {
    int memid;
    Communication *com;

    key_t key = ftok("ex2.1.c", 'B');



    memid = shmget(key, sizeof(Communication), IPC_CREAT | IPC_EXCL | 0600);

    if (memid > 0) {
        com = (Communication *)shmat(memid, 0, 0);
        com->num = 0;
        com->flag = 0;

        while (com->num != -1) {
            if (com->flag == 0) { // Program 1 can write
                printf("Program 1, enter a number (or -1 to exit): ");
                scanf("%d", &com->num);
                com->flag = 1; // Change flag so Program 2 can read and write
            }
        }
        
        shmdt(com);
        shmctl(memid, IPC_RMID, NULL);
    } else {
        printf("Shared memory region already exists\n");
    }

    return 0;
}
