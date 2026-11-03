//************
//
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau
//
//************


#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <pthread.h>

#define printF(x) write(1, x, strlen(x))

int listenFD;

void quit(int signum __attribute__((unused))){
  close(listenFD);
	signal(SIGINT, SIG_DFL);
  raise(SIGINT);
}

void* threadClient(void* fd){
  int fdClient = *((int*)fd);
  char message[100];
  char* screen;
	int done = 0;

  while(!done){
		memset(message, 0, 100);
  	int size = read(fdClient, message, 99);
		if(size == 0){
			done = 1;
			write(1, "Disconnected\n", strlen("Disconnected\n"));
		}else{
	    if(strcmp(message, "QUIT")==0){
	      done = 1;
	      write(1, "Disconnected\n", strlen("Disconnected\n"));
	    }else{
        size = asprintf(&screen, "%s\n", message);
        write(1, screen, size);
        free(screen);
      }
		}
  }

  close(fdClient);

  pthread_cancel(pthread_self());
  pthread_detach(pthread_self());

  return NULL;

}

int main(){
  int clientFD;
  struct sockaddr_in server;

  signal(SIGINT, quit);

  if( (listenFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
    printF("Error creating the socket\n");
  }

  bzero(&server, sizeof(server));
  server.sin_port = htons(9000);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(listenFD, (struct sockaddr*) &server, sizeof(server)) < 0){
    printF("Error on bind\n");
  }

  if(listen(listenFD, 10) < 0){
    printF("Error on listen\n");
  }







  int* clients;
  int n_clients = 0;



  while(1){
    printF("Waiting for connections...\n");

    clientFD = accept(listenFD, NULL, NULL);

    if(n_clients == 0){
      clients = malloc(sizeof(int));
    }else{
      clients = realloc(clients, (n_clients+1)*sizeof(int));
    }

    clients[n_clients] = clientFD;

    printF("New client connected!\n");


  }


}
