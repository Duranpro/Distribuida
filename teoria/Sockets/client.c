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

#define ERR_SOCKET "ERROR: Could not create socket\n"
#define ERR_CONNECT "ERROR: Could not connect to the server\n"

#define printR(x) write(2, x, strlen(x))


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


int openClientConn(char *ip, int port) {
    struct sockaddr_in s_addr;
    int fd_socket = -1;

    fd_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_socket < 0) {
        printR(ERR_SOCKET);
        return -1;
    }

    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &s_addr.sin_addr) < 0) {
        printR("ERROR: Could not convert IP address\n");
        close(fd_socket);
        return -1;
    }

    if (connect(fd_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0) {
        printR(ERR_CONNECT);
        close(fd_socket);
        return -1;
    }

    return fd_socket;
}

int main(int argc, char **argv) {
    int fd_socket = -1;
    int port = 0;
    char *ip = NULL;
    char *message = NULL;
    char *buffer = NULL;

    if (argc != 3) {
        printR("ERROR: The number of arguments must be 2 (IP and port)\n");
        return 1;
    }
    
    ip = argv[1];
    port = atoi(argv[2]);

    fd_socket = openClientConn(ip, port);
    if (fd_socket < 0) {
        return 1;
    }
    printR("Connected to server\n");
    printR("Enter the message to send to the server: ");

    buffer = readUntil(0, '\n');
    asprintf(&message, "%s\n", buffer);
    free(buffer);

    // Send message to server
    write(fd_socket, message, strlen(message));

    close(fd_socket);
    free(message);

    return 0;
}
