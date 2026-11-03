/*
 * Solution S5_sol
 * Academic year 2026-27
 * *
 */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_TRADERS 5
#define SYMBOL_COUNT 3
#define FRAME_SIZE 128

typedef struct {
    char *name;
    int subscribed[SYMBOL_COUNT];
} Client;

typedef struct {
    int listen_fd;
    int max_fd;
    int trader_count;
    int prices[SYMBOL_COUNT];
    int running;

    fd_set fds;
    Client *clients;
    int capacity;
} Server;

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

char *symbol_name(int symbol) {
    if (symbol == 0) {
        return strdup("LSL");
    }
    if (symbol == 1) {
        return strdup("CPU");
    }
    if (symbol == 2) {
        return strdup("RAM");
    }
    return NULL;
}

int symbol_id(char *symbol) {
    if (strcmp(symbol, "LSL") == 0) {
        return 0;
    }
    if (strcmp(symbol, "CPU") == 0) {
        return 1;
    }
    if (strcmp(symbol, "RAM") == 0) {
        return 2;
    }
    return -1;
}

int growClients(Server *server, int fd) {
    if (fd < server->capacity) {
        return 0;
    }
    Client *next = realloc(server->clients, (fd + 1) * sizeof(Client));
    if (next == NULL) {
        return -1;
    }
    server->clients = next;
    memset(next + server->capacity, 0, (fd + 1 - server->capacity) * sizeof(Client));
    server->capacity = fd + 1;
    return 0;
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

void disconnect_client(Server *server, int fd) {
    if (server->clients[fd].name != NULL) {
        server->trader_count--;
    }

    FD_CLR(fd, &server->fds);
    close(fd);
    free(server->clients[fd].name);
    memset(&server->clients[fd], 0, sizeof(Client));
}

int name_exists(Server *server, char *name) {
    int fd;

    for (fd = 0; fd <= server->max_fd; fd++) {
        if (server->clients[fd].name != NULL && strcmp(server->clients[fd].name, name) == 0) {
            return 1;
        }
    }

    return 0;
}

void register_client(Server *server, int fd, char *message) {
    char *name;
    char *response;

    if (strncmp(message, "HELLO:", 6) != 0) {
        disconnect_client(server, fd);
        return;
    }

    name = message + 6;

    if (name_exists(server, name)) {
        send_frame(fd, "ERROR:NAME_EXISTS");
        disconnect_client(server, fd);
        return;
    }

    if (server->trader_count == MAX_TRADERS) {
        send_frame(fd, "ERROR:MARKET_FULL");
        disconnect_client(server, fd);
        return;
    }

    server->clients[fd].name = strdup(name);
    if (server->clients[fd].name == NULL) {
        disconnect_client(server, fd);
        return;
    }
    server->trader_count++;

    if (asprintf(&response, "WELCOME:%s", name) >= 0) {
        send_frame(fd, response);
        free(response);
    }
}

void change_subscription(Server *server, int fd, char *symbol_text, int subscribe) {
    char *response;
    int symbol = symbol_id(symbol_text);

    if (symbol < 0) {
        return;
    }

    server->clients[fd].subscribed[symbol] = subscribe;

    int length;
    if (subscribe) {
        length = asprintf(&response, "SUBSCRIBED:%s", symbol_text);
    } else {
        length = asprintf(&response, "UNSUBSCRIBED:%s", symbol_text);
    }

    if (length >= 0) {
        send_frame(fd, response);
        free(response);
    }
}

void handle_client(Server *server, int fd) {
    char frame[FRAME_SIZE];
    char *command;
    char *symbol;
    int bytes;

    memset(frame, 0, sizeof(frame));
    bytes = read(fd, frame, sizeof(frame));

    if (bytes <= 0) {
        disconnect_client(server, fd);
        return;
    }

    if (server->clients[fd].name == NULL) {
        register_client(server, fd, frame);
        return;
    }

    if (strcmp(frame, "QUIT") == 0) {
        disconnect_client(server, fd);
        return;
    }

    char *position = frame;
    command = nextField(&position, ':');
    symbol = nextField(&position, ':');

    if (command == NULL || symbol == NULL) {
        return;
    }

    if (strcmp(command, "SUB") == 0) {
        change_subscription(server, fd, symbol, 1);
    } else if (strcmp(command, "UNSUB") == 0) {
        change_subscription(server, fd, symbol, 0);
    }
}

void send_tick(Server *server, int symbol) {
    char *message;
    int fd;

    {
        char *symbolText = symbol_name(symbol);
        if (symbolText != NULL && asprintf(&message, "TICK:%s:%d", symbolText, server->prices[symbol]) >= 0) {
            for (fd = 0; fd <= server->max_fd; fd++) {
                if (server->clients[fd].name != NULL && server->clients[fd].subscribed[symbol]) {
                    send_frame(fd, message);
                }
            }

            free(message);
        }
        free(symbolText);
    }
}

void show_status(Server *server) {
    char *message;
    for (int symbol = 0; symbol < SYMBOL_COUNT; symbol++) {
        char *name = symbol_name(symbol);
        if (name == NULL) {
            continue;
        }
        int length;
        if (server->prices[symbol] < 0) {
            length = asprintf(&message, "%s: N/A\n", name);
        } else {
            length = asprintf(&message, "%s: %d\n", name, server->prices[symbol]);
        }
        free(name);
        if (length >= 0) {
            write(STDOUT_FILENO, message, length);
            free(message);
        }
    }
    if (asprintf(&message, "Traders connected: %d\n", server->trader_count) >= 0) {
        write(STDOUT_FILENO, message, strlen(message));
        free(message);
    }
    for (int fd = 0; fd <= server->max_fd; fd++) {
        if (server->clients[fd].name != NULL) {
            if (asprintf(&message, "- %s\n", server->clients[fd].name) >= 0) {
                write(STDOUT_FILENO, message, strlen(message));
                free(message);
            }
        }
    }
}

void close_market(Server *server) {
    int fd;

    for (fd = 0; fd <= server->max_fd; fd++) {
        if (server->clients[fd].name != NULL) {
            send_frame(fd, "MARKET:CLOSED");
        }
    }

    server->running = 0;
}

void handle_admin(Server *server) {
    char *line;
    char *command;
    char *symbol_text;
    char *price_text;
    int symbol;
    int price;

    line = read_until(STDIN_FILENO, '\n');
    if (line == NULL) {
        close_market(server);
        return;
    }

    char *position = line;
    command = nextField(&position, ' ');

    if (command == NULL) {
        free(line);
        return;
    }

    if (strcmp(command, "STATUS") == 0) {
        show_status(server);
    } else if (strcmp(command, "CLOSE") == 0) {
        close_market(server);
    } else if (strcmp(command, "PRICE") == 0) {
        symbol_text = nextField(&position, ' ');
        price_text = nextField(&position, ' ');

        if (symbol_text != NULL && price_text != NULL) {
            symbol = symbol_id(symbol_text);
            price = atoi(price_text);

            if (symbol >= 0 && price > 0) {
                server->prices[symbol] = price;
                send_tick(server, symbol);
            }
        }
    }

    free(line);
}

void accept_client(Server *server) {
    int fd;

    fd = accept(server->listen_fd, NULL, NULL);

    if (fd < 0 || fd >= FD_SETSIZE) {
        if (fd >= 0) {
            close(fd);
        }
        return;
    }

    if (growClients(server, fd) < 0) {
        close(fd);
        return;
    }
    free(server->clients[fd].name);
    memset(&server->clients[fd], 0, sizeof(Client));

    FD_SET(fd, &server->fds);

    if (fd > server->max_fd) {
        server->max_fd = fd;
    }
}

int open_server(char *ip, int port) {
    struct sockaddr_in address;
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        return -1;
    }

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &address.sin_addr) != 1) {
        close(fd);
        return -1;
    }

    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(fd);
        return -1;
    }

    if (listen(fd, MAX_TRADERS) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

void close_server(Server *server) {
    int fd;

    for (fd = 0; fd <= server->max_fd; fd++) {
        if (FD_ISSET(fd, &server->fds) && fd != STDIN_FILENO) {
            close(fd);
        }
    }
    for (fd = 0; fd < server->capacity; fd++) {
        free(server->clients[fd].name);
    }
    free(server->clients);
}

int main(int argc, char **argv) {
    Server server;
    fd_set ready;
    int port;
    int fd;

    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./S5 <ip> <port>\n", strlen("Usage: ./S5 <ip> <port>\n"));
        return 1;
    }

    port = atoi(argv[2]);

    if (port <= 0 || port > 65535) {
        write(STDERR_FILENO, "Error starting server.\n", strlen("Error starting server.\n"));
        return 1;
    }

    signal(SIGINT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    memset(&server, 0, sizeof(server));

    server.listen_fd = open_server(argv[1], port);
    server.running = 1;

    if (server.listen_fd < 0) {
        write(STDERR_FILENO, "Error starting server.\n", strlen("Error starting server.\n"));
        return 1;
    }

    if (server.listen_fd >= FD_SETSIZE || growClients(&server, server.listen_fd) < 0) {
        close(server.listen_fd);
        return 1;
    }

    FD_ZERO(&server.fds);
    FD_SET(STDIN_FILENO, &server.fds);
    FD_SET(server.listen_fd, &server.fds);

    server.max_fd = server.listen_fd;

    while (server.running) {
        ready = server.fds;

        if (select(server.max_fd + 1, &ready, NULL, NULL, NULL) < 0) {
            break;
        }

        if (FD_ISSET(server.listen_fd, &ready)) {
            accept_client(&server);
        }

        if (FD_ISSET(STDIN_FILENO, &ready)) {
            handle_admin(&server);
        }

        for (fd = 0; fd <= server.max_fd && server.running; fd++) {
            if (fd != STDIN_FILENO && fd != server.listen_fd && FD_ISSET(fd, &ready)) {
                handle_client(&server, fd);
            }
        }
    }

    close_server(&server);

    return 0;
}