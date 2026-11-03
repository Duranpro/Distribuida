/*
 * Solution S5_trader
 * Academic year 2026-27
 * *
 */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define NAME_SIZE 17
#define FRAME_SIZE 128

char *nextField(char **position, char delimiter) {
    char *start = *position;
    while (*start == delimiter) {
        start++;
    }
    if (*start == '\0') {
        *position = start;
        return NULL;
    }
    char *end = start;
    while (*end != '\0' && *end != delimiter) {
        end++;
    }
    if (*end == delimiter) {
        *end = '\0';
        end++;
    }
    *position = end;
    return start;
}

char *read_until(int fd, char delimiter) {
    char *buffer = NULL;
    int i = 0;
    char c;
    int len = read(fd, &c, 1);

    if (len <= 0) {
        return NULL;
    }

    while (len > 0 && c != delimiter && c != '\r') {
        char *new_buffer = realloc(buffer, sizeof(char) * (i + 2));
        if (new_buffer == NULL) {
            free(buffer);
            return NULL;
        }
        buffer = new_buffer;
        buffer[i] = c;
        i++;
        len = read(fd, &c, 1);
    }

    if (len < 0) {
        free(buffer);
        return NULL;
    }
    if (buffer == NULL) {
        buffer = malloc(1);
        if (buffer == NULL) {
            return NULL;
        }
        buffer[0] = '\0';
        return buffer;
    }

    buffer[i] = '\0';
    return buffer;
}

void send_frame(int fd, char *text) {
    char frame[FRAME_SIZE];

    memset(frame, 0, sizeof(frame));
    strcpy(frame, text);
    write(fd, frame, sizeof(frame));
}

int read_frame(int fd, char *frame) {
    int received;

    memset(frame, 0, FRAME_SIZE);
    received = read(fd, frame, FRAME_SIZE);

    return received;
}

int valid_symbol(char *symbol) {
    return strcmp(symbol, "LSL") == 0 || strcmp(symbol, "CPU") == 0 || strcmp(symbol, "RAM") == 0;
}

void show_help() {
    write(STDOUT_FILENO, "\nCommands:\n", strlen("\nCommands:\n"));
    write(STDOUT_FILENO, "SUB <symbol>\n", strlen("SUB <symbol>\n"));
    write(STDOUT_FILENO, "UNSUB <symbol>\n", strlen("UNSUB <symbol>\n"));
    write(STDOUT_FILENO, "QUIT\n\n", strlen("QUIT\n\n"));
}

void show_prompt() {
    write(STDOUT_FILENO, "> ", strlen("> "));
}

int handle_keyboard(int fd) {
    char *line;
    char *command;
    char *symbol;
    char *message;
    int running = 1;

    line = read_until(STDIN_FILENO, '\n');
    if (line == NULL) {
        send_frame(fd, "QUIT");
        return 0;
    }

    char *position = line;
    command = nextField(&position, ' ');

    if (command == NULL) {
        show_prompt();
        free(line);
        return 1;
    }

    if (strcmp(command, "QUIT") == 0) {
        send_frame(fd, "QUIT");
        running = 0;
    } else if (strcmp(command, "SUB") == 0 || strcmp(command, "UNSUB") == 0) {
        symbol = nextField(&position, ' ');

        if (symbol == NULL || nextField(&position, ' ') != NULL || !valid_symbol(symbol)) {
            write(STDOUT_FILENO, "Invalid command.\n", strlen("Invalid command.\n"));
            show_prompt();
        } else {
            if (asprintf(&message, "%s:%s", command, symbol) >= 0) {
                send_frame(fd, message);
                free(message);
            }
        }
    } else {
        write(STDOUT_FILENO, "Invalid command.\n", strlen("Invalid command.\n"));
        show_prompt();
    }

    free(line);
    return running;
}

int handle_server(int fd) {
    char frame[FRAME_SIZE];

    if (read_frame(fd, frame) <= 0) {
        write(STDOUT_FILENO, "\nServer disconnected.\n", strlen("\nServer disconnected.\n"));
        return 0;
    }

    if (strcmp(frame, "MARKET:CLOSED") == 0) {
        write(STDOUT_FILENO, "\nMarket closed.\n", strlen("\nMarket closed.\n"));
        return 0;
    }

    write(STDOUT_FILENO, "\n", 1);
    write(STDOUT_FILENO, frame, strlen(frame));
    write(STDOUT_FILENO, "\n", 1);
    show_prompt();

    return 1;
}

int connect_server(char *ip, int port) {
    struct sockaddr_in address;
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, ip, &address.sin_addr) != 1) {
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

int main(int argc, char **argv) {
    char frame[FRAME_SIZE];
    char *hello;
    fd_set descriptors;
    fd_set ready;
    int fd;
    int port;
    int running;

    if (argc != 4) {
        write(STDERR_FILENO, "Usage: ./S5_trader <ip> <port> <name>\n", strlen("Usage: ./S5_trader <ip> <port> <name>\n"));
        return 1;
    }

    if (strlen(argv[3]) == 0 || strlen(argv[3]) >= NAME_SIZE || strchr(argv[3], ':') != NULL) {
        write(STDERR_FILENO, "Invalid trader name.\n", strlen("Invalid trader name.\n"));
        return 1;
    }

    port = atoi(argv[2]);

    if (port <= 0 || port > 65535) {
        write(STDERR_FILENO, "Error connecting to server.\n", strlen("Error connecting to server.\n"));
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    fd = connect_server(argv[1], port);

    if (fd < 0) {
        write(STDERR_FILENO, "Error connecting to server.\n", strlen("Error connecting to server.\n"));
        return 1;
    }

    if (asprintf(&hello, "HELLO:%s", argv[3]) < 0) {
        close(fd);
        return 1;
    }
    send_frame(fd, hello);
    free(hello);

    if (read_frame(fd, frame) <= 0) {
        write(STDERR_FILENO, "Server disconnected.\n", strlen("Server disconnected.\n"));
        close(fd);
        return 1;
    }

    if (strncmp(frame, "WELCOME:", 8) != 0) {
        write(STDOUT_FILENO, frame, strlen(frame));
        write(STDOUT_FILENO, "\n", 1);
        close(fd);
        return 1;
    }

    write(STDOUT_FILENO, frame, strlen(frame));
    write(STDOUT_FILENO, "\n", 1);

    show_help();
    show_prompt();

    FD_ZERO(&descriptors);
    FD_SET(STDIN_FILENO, &descriptors);
    FD_SET(fd, &descriptors);

    running = 1;

    while (running) {
        ready = descriptors;

        if (select(fd + 1, &ready, NULL, NULL, NULL) < 0) {
            break;
        }

        if (FD_ISSET(fd, &ready)) {
            running = handle_server(fd);
        }

        if (running && FD_ISSET(STDIN_FILENO, &ready)) {
            running = handle_keyboard(fd);
        }
    }

    close(fd);

    return 0;
}