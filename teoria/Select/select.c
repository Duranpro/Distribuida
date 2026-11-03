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
#include <pthread.h>

#define printF(x) write(1, x, strlen(x))

#define ERR_SLCT "select() returned an error\n"
#define ERR_SCKT "Error creating socket\n"
#define ERR_BIND "Error binding socket\n"
#define ERR_LISTEN "Error listening on socket\n"
#define CLIENT_DISCONN "Client disconnected\n"
#define NEW_CLIENT "New client connected\n"
#define WAITING_CONN "Waiting for connections...\n"

int listenFD;
fd_set current_sockets, ready_sockets;

int *conn = NULL;
int totalConnected = 0;
char *buffer = NULL;

// SIGINT handler
void cleanup(int signum __attribute__((unused))) {
    free(buffer);
    if (conn != NULL) {
        for (int i = 0; i < totalConnected; i++) {
            if (conn[i] != -1) {
                close(conn[i]);
            }
        }
        free(conn);
    }
    close(listenFD);
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

// Thread for each message
void* threadClient(void* fd) {
    int client_fd = *((int*)fd);
    free(fd);

    char buffer[100];
    for(int i=0; i<10 ;i++){

        int valread = read(client_fd, buffer, 100);
        if (valread <= 0) {
            printF(CLIENT_DISCONN);
            close(client_fd);
            pthread_exit(NULL);
        }

        buffer[valread] = '\0';
        printF("Message received: ");
        write(STDOUT_FILENO, buffer, valread);

    }

    close(client_fd);

    pthread_cancel(pthread_self());
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        printF("Usage: ./server <ip> <port>\n");
        exit(EXIT_FAILURE);
    }

    // -------------------------------------------- SOCKET CONFIGURATION --------------------------------------------
    struct sockaddr_in server;
    int port = atoi(argv[2]);

    // Signal handling for cleanup
    signal(SIGINT, cleanup);

    // Create server socket
    if ((listenFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printF(ERR_SCKT);
        exit(EXIT_FAILURE);
    }

    // Setup server address
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, argv[1], &server.sin_addr) <= 0) {
        printF("Invalid IP address\n");
        close(listenFD);
        exit(EXIT_FAILURE);
    }

    // Bind the socket
    if (bind(listenFD, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printF(ERR_BIND);
        close(listenFD);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listenFD, 10) < 0) {
        printF(ERR_LISTEN);
        close(listenFD);
        exit(EXIT_FAILURE);
    }

    // --------------------------------------------------------------------------------------------------------------

    printF(WAITING_CONN);

    // Create the FD Set
    FD_ZERO(&current_sockets);
    FD_SET(listenFD, &current_sockets);



    while (1) {
        // Make a copy because select is destructive
        ready_sockets = current_sockets;

        // Wait for activity on any socket
        int activity = select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            printF(ERR_SLCT);
            continue;
        }

        // Loop through all FDs
        for (int i = 3; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &ready_sockets)) {

                // Handle new client connection
                if (i == listenFD) {

                    // Accept the client
                    int clientFD = accept(listenFD, NULL, NULL);
            
                    // Add new client to connection list
                    if (conn == NULL) {
                        conn = malloc(sizeof(int));
                    } else {
                        conn = realloc(conn, (totalConnected + 1) * sizeof(int));
                    }

                    conn[totalConnected] = clientFD;
                    totalConnected++;

                    // Add fd to the permanent set
                    FD_SET(clientFD, &current_sockets);
                    printF(NEW_CLIENT);

                }  else {
                    // Handle client message

                    pthread_t client_thread;
                    int *client_fd = malloc(sizeof(int));

                    *client_fd = i;

                    pthread_create(&client_thread, NULL, threadClient, client_fd);

                }
            }
        }
    }

    cleanup(0);
    return 0;
}
