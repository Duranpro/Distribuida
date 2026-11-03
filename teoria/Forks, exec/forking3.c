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

int main(){
    char* buffer;
	int child;

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

            char* arr[] = {"/bin/ls", "-l", NULL};
            execv("/bin/ls", arr);
            
			write(1,"Something went wrong...\n",strlen("Something went wrong...\n"));
	
			break;
	
		default:
			asprintf(&buffer,"I'm the Parent! My pid = %d\n",getpid());
			write(1,buffer,strlen(buffer));
			free(buffer);
			wait(NULL);
			
		break;
	}
}
