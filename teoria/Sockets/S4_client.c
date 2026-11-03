/*
 * Solution S4 Operating Systems - Sockets (client)
 * Course 2026-27
 *
 * @author: Felipe Trejos（◕‿◕）
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include "S4_library.h"

void sendMessage(int fd, char type, char *payload, int length) {
    char buffer[5];
    int len = htonl(length);

    buffer[0] = type;
    memcpy(buffer + 1, &len, sizeof(len));

    write(fd, buffer, 5);
    if (length > 0) write(fd, payload, length);
}

char *receiveMessage(int fd, Header *header) {
    char buffer[5];
    int len;

    read(fd, buffer, 5);

    header->type = buffer[0];
    memcpy(&len, buffer + 1, sizeof(len));
    header->length = ntohl(len);

    char *payload = malloc(header->length + 1);
    if (header->length > 0) read(fd, payload, header->length);
    payload[header->length] = '\0';
    return payload;
}

int connectToServer(char *ip, int port) {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printF("Error creating the socket\n");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1) {
        printF("Error connecting to the server\n");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main(int argc, char **argv) {
    Header header;
    char *msg;
    int count = 0;

    if (argc != 4) {
        printFD(STDERR_FILENO, "Usage: ./S4_client <ip> <puerto> <archivo_equipo>\n");
        return 1;
    }

    signal(SIGINT, SIG_IGN);

    int fd_file = open(argv[3], O_RDONLY);
    if (fd_file < 0) {
        printFD(STDERR_FILENO, "Error opening the team file\n");
        return 1;
    }

    int fd_socket = connectToServer(argv[1], atoi(argv[2]));
    if (fd_socket == -1) {
        close(fd_file);
        return 1;
    }

    char *firstLine = readUntil(fd_file, '\n');
    sendMessage(fd_socket, 'R', firstLine, strlen(firstLine));

    char **parts = splitByChar(firstLine, &count, "&");
    char *name = parts[0];
    free(parts[1]);
    free(parts);
    free(firstLine);

    asprintf(&msg, "[%s] Registered at the turnstile\n", name);
    printF(msg);
    free(msg);

    asprintf(&msg, "[%s] Waiting for the other team...\n", name);
    printF(msg);
    free(msg);

    char *payload = receiveMessage(fd_socket, &header);
    free(payload);

    asprintf(&msg, "[%s] Entering the turnstile\n", name);
    printF(msg);
    free(msg);

    int sent = 0;
    while (1) {
        char *line = readUntil(fd_file, '\n');
        if (line == NULL) break;
        sendMessage(fd_socket, 'O', line, strlen(line));
        sent++;
        free(line);
    }
    close(fd_file);

    asprintf(&msg, "[%s] %d operations sent through the turnstile\n", name, sent);
    printF(msg);
    free(msg);

    sendMessage(fd_socket, 'C', NULL, 0);

    asprintf(&msg, "[%s] Waiting for the reconstructed timeline...\n", name);
    printF(msg);
    free(msg);

    payload = receiveMessage(fd_socket, &header);
    printF(payload);
    free(payload);

    close(fd_socket);
    free(name);
    return 0;
}
