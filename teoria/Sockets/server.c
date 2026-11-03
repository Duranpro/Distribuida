//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define ERR_CODE_SOCKET -1
#define ERR_ARGS "ERROR: The number of arguments is not 2."
#define ERR_SOCKET "ERROR: Could not create socket\n"
#define ERR_CONV_IP "ERROR: Could not convert IP address\n"
#define ERR_BIND "ERROR: Could not bind to port\n"
#define ERR_ACCEPT "ERROR accepting connection\n"
#define MAX_CONN 5

#define printR(x) write(2, x, strlen(x))

char byte[50];



char *readUntil(int fd, char cEnd) {
    int i = 0;
    int chars_read = 0;
    char c = ' ';
    char *buffer = NULL;

    while (c != cEnd) {
        chars_read = (int) read(fd, &c, sizeof(char));
        if (buffer == NULL) {
            buffer = (char *) malloc(sizeof(char));
        }
        if (chars_read > 0) {
            if (c != cEnd) {
                buffer[i] = c;
                buffer = (char *) realloc(buffer, (sizeof(char)) * (i + 2));
            }
            i++;
        }
    }

    buffer[i - 1] = '\0';
    return buffer;
}


int openListenConn(char *ip, int port) {
    struct sockaddr_in s_addr;
    int fd_socket = -1;

    fd_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (fd_socket < 0) {
        printR(ERR_SOCKET);
        return ERR_CODE_SOCKET;
    }

    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);

    // s_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (inet_pton(AF_INET, ip, &s_addr.sin_addr) < 0) {
        printR(ERR_CONV_IP);
        close(fd_socket);
        return ERR_CODE_SOCKET;
    }

    if (bind(fd_socket, (void *) &s_addr, sizeof(s_addr)) < 0) {
        printR(ERR_BIND);
        close(fd_socket);
        return ERR_CODE_SOCKET;
    }

    listen(fd_socket, MAX_CONN);

    return fd_socket;
}

int main(int argc, char **argv) {
    int fd_socket = -1;
    int port = 0;
    char *ip = NULL;
    int fd_client = -1;
    char *buffer = NULL;

    if (argc != 3) {
        printR(ERR_ARGS);
        return 1;
    }
    ip = argv[1];
    port = atoi(argv[2]);

    fd_socket = openListenConn(ip, port);
    if (fd_socket == ERR_CODE_SOCKET) {
        return 1;
    }

    fd_client = accept(fd_socket, NULL, NULL);
    
    buffer = readUntil(fd_client, '\n');
    read(fd_client, buffer, 10000);
    printR(buffer);
    free(buffer);
    
    close(fd_client);
    close(fd_socket);

    return 0;
}