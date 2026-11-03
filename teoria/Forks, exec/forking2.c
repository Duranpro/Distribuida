//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

char* buffer;

void rsiAlarm(int signum __attribute__((unused))) {
	asprintf(&buffer,"I'm RSI() My pid = %d\n",getpid());
	write(1,buffer,strlen(buffer));
	free(buffer);

	alarm(5);
}

int main() {
	int child;

	signal(SIGALRM,rsiAlarm);

	child = fork();

	switch (child) {
		case -1:
			asprintf(&buffer,"Error in fork! :(\n");
			write(1,buffer,strlen(buffer));	
			free(buffer);
			break;

		case 0:
			asprintf(&buffer,"I'm the child with pid = %d\n",getpid());
			write(1,buffer,strlen(buffer));
			free(buffer);
			alarm(5);
			while (1) {
				pause();
			}	
			break;
	
		default:
			asprintf(&buffer,"I'm the Parent! My pid = %d\n",getpid());
			write(1,buffer,strlen(buffer));
			free(buffer);
			alarm(8);
			while (1) {
				pause();
			}
		break;
	}
}
