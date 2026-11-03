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

int main() {

	int child;
	char* buffer;
	int pid;

	int fd = open("questions.txt", O_APPEND|O_WRONLY|O_CREAT, 0777);
	child = fork();
	
	if (child > 0) {

		pid = getpid();
		asprintf(&buffer,"I'm the Parent! My pid = %d\n",pid);
		write(1, buffer, strlen(buffer));
		free(buffer);
		write(fd, "\nThe parent writes :)\n", strlen("\nThe parent writes :)\n"));
		
		wait(NULL);

		exit(0);
	} else {
		if (child == 0) {
			pid = getpid();
			asprintf(&buffer,"I'm the child with pid = %d\n",pid);
			write(1,buffer,strlen(buffer));
			free(buffer);

			write(fd, "\nThe child writes :)\n", strlen("\nThe child writes :)\n"));

			exit(0);
		} else {
			asprintf(&buffer,"There was an error in fork! :(\n");
			write(1,buffer,strlen(buffer));	
			free(buffer);
		}
	}
	return 1;
}
