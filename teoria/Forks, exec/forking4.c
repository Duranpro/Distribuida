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
			write(1,"Error in fork! :(\n",strlen("Error in fork! :(\n"));	
			break;

		case 0:
			asprintf(&buffer,"I'm the child with pid = %d\n",getpid());
			write(1,buffer,strlen(buffer));
			free(buffer);

            int fill2 = fork();
        
            switch (fill2) {
                case -1:
                    write(1,"Error in fork! :(\n",strlen("Error in fork! :(\n"));	
                    break;
                 case 0:
                    asprintf(&buffer,"I'm the child of the child, with pid = %d\n",getpid());
                    write(1,buffer,strlen(buffer));
                    free(buffer);
                    break;   
                default:
                    asprintf(&buffer,"I'm the child of %d, but the parent of %d\n",getppid(), fill2);
                    write(1,buffer,strlen(buffer));
                    free(buffer);
                    wait(NULL);
                    break;
                }
	
			break;
	
		default:
			asprintf(&buffer,"I'm the Parent! My pid = %d\n",getpid());
			write(1,buffer,strlen(buffer));
			free(buffer);			
            wait(NULL);
		break;
	}
}
