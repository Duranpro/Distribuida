/*
 * Solution S4 Operating Systems - Sockets (server)
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
#include <pthread.h>
#include <arpa/inet.h>
#include "S4_library.h"

typedef struct {
    int timestamp;
    char* action;
} Operation;

typedef struct {
    int fd;
    int id;
    char* name;
    char direction;
    Operation* ops;
    int numOps;
} Team;

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

int openListenConnection(int port) {
    int server_fd;
    struct sockaddr_in address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printF("Error creating the socket\n");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printF("Error binding the socket\n");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 3) < 0) {
        printF("Error listening on the socket\n");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

void registerTeam(Team *team) {
    Header header;
    char *msg;
    int count = 0;

    char *payload = receiveMessage(team->fd, &header);
    char **parts = splitByChar(payload, &count, "&");
    free(payload);

    team->name = parts[0];
    team->direction = parts[1][0];
    free(parts[1]);
    free(parts);

    asprintf(&msg, "[TURNSTILE] Team %s registered -- ", team->name);
    printF(msg);
    free(msg);

    asprintf(&msg, "direction %s (id %d)\n", team->direction == 'F' ? "FORWARD" : "INVERTED", team->id);
    printF(msg);
    free(msg);
}

void *handleTeam(void *arg) {
    Team *team = (Team *) arg;
    Header header;
    char *payload;
    char **parts;
    int count = 0;

    while (1) {
        payload = receiveMessage(team->fd, &header);
        if (header.type == 'C') {
            free(payload);
            break;
        }

        parts = splitByChar(payload, &count, "&");
        free(payload);

        team->ops = realloc(team->ops, sizeof(Operation) * (team->numOps + 1));
        team->ops[team->numOps].timestamp = atoi(parts[0]);
        team->ops[team->numOps].action = parts[1];
        team->numOps++;

        free(parts[0]);
        free(parts);
    }

    if (team->direction == 'I') {
        for (int i = 0; i < team->numOps / 2; i++) {
            Operation tmp = team->ops[i];
            team->ops[i] = team->ops[team->numOps - 1 - i];
            team->ops[team->numOps - 1 - i] = tmp;
        }
    }

    return NULL;
}

int hasTimestamp(Team *team, int timestamp) {
    for (int i = 0; i < team->numOps; i++) {
        if (team->ops[i].timestamp == timestamp) return 1;
    }
    return 0;
}

char *buildTimeline(Team *forward, Team *inverted) {
    char *timeline = strdup("=== MISSION TIMELINE ===\n");
    char *line;
    char *tmp;
    int i = 0, j = 0;

    while (i < forward->numOps || j < inverted->numOps) {
        int takeForward;
        if (i >= forward->numOps) takeForward = 0;
        else if (j >= inverted->numOps) takeForward = 1;
        else takeForward = forward->ops[i].timestamp <= inverted->ops[j].timestamp;

        Operation *op = takeForward ? &forward->ops[i] : &inverted->ops[j];

        int collision = !takeForward && hasTimestamp(forward, op->timestamp);

        asprintf(&line, "t=%d (%c) %s", op->timestamp, takeForward ? 'F' : 'I', op->action);

        tmp = timeline;
        asprintf(&timeline, "%s%s%s\n", tmp, line, collision ? "   << TEMPORAL COLLISION" : "");
        free(tmp);
        free(line);

        if (takeForward) i++;
        else j++;
    }

    return timeline;
}

void freeTeams(Team *teams) {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < teams[i].numOps; j++) {
            free(teams[i].ops[j].action);
        }
        free(teams[i].ops);
        free(teams[i].name);
        close(teams[i].fd);
    }
}

int main(int argc, char **argv) {
    Team teams[2];
    pthread_t threads[2];
    char *msg;

    if (argc != 2) {
        printFD(STDERR_FILENO, "Usage: ./S4_server <puerto>\n");
        return 1;
    }

    signal(SIGINT, SIG_IGN);

    int fd_server = openListenConnection(atoi(argv[1]));
    if (fd_server == -1) return 1;
    
    printF("[TURNSTILE] Waiting for the two field teams...\n");
    memset(teams, 0, sizeof(teams));

    for (int i = 0; i < 2; i++) {
        teams[i].fd = accept(fd_server, NULL, NULL);
        if (teams[i].fd < 0) {
            printFD(STDERR_FILENO, "Error accepting connection\n");
            close(fd_server);
            return 1;
        }
        teams[i].id = i + 1;
        registerTeam(&teams[i]);
    }

    printF("[TURNSTILE] Both teams in position -- opening the turnstile\n");
    sendMessage(teams[0].fd, 'S', NULL, 0);
    sendMessage(teams[1].fd, 'S', NULL, 0);

    for (int i = 0; i < 2; i++) {
        if (pthread_create(&threads[i], NULL, handleTeam, &teams[i]) != 0) {
            printFD(STDERR_FILENO, "Error creating the thread\n");
            close(fd_server);
            return 1;
        }
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }

    Team *forward = teams[0].direction == 'F' ? &teams[0] : &teams[1];
    Team *inverted = teams[0].direction == 'F' ? &teams[1] : &teams[0];

    asprintf(&msg, "[TURNSTILE] Reconciling %d operations...\n",forward->numOps + inverted->numOps);
    printF(msg);
    free(msg);

    char *timeline = buildTimeline(forward, inverted);
    printF(timeline);

    sendMessage(forward->fd, 'T', timeline, strlen(timeline));
    sendMessage(inverted->fd, 'T', timeline, strlen(timeline));
    free(timeline);

    freeTeams(teams);
    close(fd_server);
    return 0;
}
