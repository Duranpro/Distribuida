/*
 * Solution S6_sensor
 * Academic year 2026-27
 *
 * @author: Uri
 *
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define printF(x) write(1, x, strlen(x))

#define MSG_USAGE "Wrong number of parameters.\nUsage: S6_sensor <ftok_path> <type>\n"
#define MSG_ERR_QUEUE "Error opening the queue. Is S6_meteo running?\n"
#define MSG_SESSION_START "%s sensor connected to MeteoLS. Session started.\n"
#define MSG_REPORT "%s sensor reports: %.1f %s\n"
#define MSG_SESSION_OVER "Session over\n"

typedef struct {
    long mtype;
    int sensorType;
    int op;
    char text[128];
} Msg;

int sendFrame(int queue, int type, int op, char *text) {
    Msg m;

    memset(&m, 0, sizeof(Msg));
    m.mtype = type + 1;
    m.sensorType = type;
    m.op = op;
    strcpy(m.text, text);
    return msgsnd(queue, &m, sizeof(Msg) - sizeof(long), 0);
}

int main(int argc, char *argv[]) {
    Msg m;
    char *buffer, *name, *unit;
    int queue, type, i;
    int end = 0;
    float value, low, range;

    if (argc != 3) {
        printF(MSG_USAGE);
        return 1;
    }
    type = atoi(argv[2]);

    queue = msgget(ftok(argv[1], 1), 0666);
    if (queue < 0) {
        printF(MSG_ERR_QUEUE);
        return 1;
    }

    // the sensors do not react to any signal
    for (i = 1; i < 32; i++) {
        if (i != SIGKILL && i != SIGSTOP) {
            signal(i, SIG_IGN);
        }
    }
    srand(getpid());

    if (type == 0) {
        name = strdup("wind");
        unit = strdup("km/h");
        low = 0;
        range = 120;
    } else if (type == 1) {
        name = strdup("pressure");
        unit = strdup("hPa");
        low = 960;
        range = 70;
    } else {
        name = strdup("temperature");
        unit = strdup("C");
        low = 10;
        range = 30;
    }

    sendFrame(queue, type, 0, "");
    msgrcv(queue, &m, sizeof(Msg) - sizeof(long), 1000 + type, 0);
    if (m.op == 5) {
        printF(m.text);
        sendFrame(queue, type, 6, "");
        end = 1;
    } else {
        asprintf(&buffer, MSG_SESSION_START, name);
        printF(buffer);
        free(buffer);
    }

    while (end == 0) {
        value = low + range * rand() / RAND_MAX;
        asprintf(&buffer, MSG_REPORT, name, value, unit);
        printF(buffer);
        free(buffer);

        asprintf(&buffer, "%.1f\n", value);
        // if the queue no longer exists the session is over
        if (sendFrame(queue, type, 2, buffer) < 0) {
            printF(MSG_SESSION_OVER);
            end = 1;
        }
        free(buffer);

        while (end == 0 && msgrcv(queue, &m, sizeof(Msg) - sizeof(long), 1000 + type, IPC_NOWAIT) > 0) {
            printF(m.text);
            if (m.op == 5) {
                sendFrame(queue, type, 6, "");
                end = 1;
            }
        }

        if (end == 0) {
            sleep(2);
        }
    }

    free(name);
    free(unit);
    return 0;
}
