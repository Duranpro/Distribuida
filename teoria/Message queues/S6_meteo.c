/*
 * Solution S6_meteo
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

#define NUM_SENSORS 3
#define WIND 0
#define PRESSURE 1
#define TEMP 2

#define MSG_USAGE "Wrong number of parameters.\nUsage: S6_meteo <ftok_path>\n"
#define MSG_ERR_QUEUE "Error creating the message queue.\n"
#define MSG_START "[MeteoLS] Central station started. Waiting for sensors (wind, pressure, temperature)...\n"
#define MSG_CONNECTED "[MeteoLS] %s sensor connected.\n"
#define MSG_ALL_CONNECTED "[MeteoLS] All three sensors are connected. Starting session.\n"
#define MSG_STATUS "[MeteoLS] Status -> wind: %.1f km/h | pressure: %.1f hPa | temperature: %.1f C\n"
#define MSG_STATUS_LINE "WIND:%.1f(%s) PRESSURE:%.1f(%s) TEMP:%.1f(%s)\n"
#define MSG_STORM "STORM ALERT declared! (wind=%.1f km/h, pressure=%.1f hPa, temperature=%.1f C)\n"
#define MSG_END_ALERT "[MeteoLS] END: storm alert declared.\n"
#define MSG_INTERRUPT "[MeteoLS] Interrupt received (CTRL+C).\n"
#define MSG_SHUTDOWN "Shutting down...\n"

typedef struct {
    long mtype;
    int sensorType;
    int op;
    char text[128];
} Msg;

int queue;
int sensors = 0;

void sendToAll(int op, char *text) {
    Msg m;
    int i;

    memset(&m, 0, sizeof(Msg));
    m.sensorType = -1;
    m.op = op;
    strcpy(m.text, text);
    for (i = 0; i < NUM_SENSORS; i++) {
        m.mtype = 1000 + i;
        msgsnd(queue, &m, sizeof(Msg) - sizeof(long), 0);
    }
}

void endSession(char *text) {
    Msg m;
    int done = 0;

    sendToAll(5, text);
    // the queue can only be removed once every connected sensor has confirmed (op 6)
    while (done < sensors) {
        msgrcv(queue, &m, sizeof(Msg) - sizeof(long), -NUM_SENSORS, 0);
        if (m.op == 6) {
            done++;
        }
    }
    printF(MSG_SHUTDOWN);
    msgctl(queue, IPC_RMID, NULL);
}

void sigintHandler() {
    printF(MSG_INTERRUPT);
    endSession("END: interrupted\n");
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

char *sensorName(int type) {
    if (type == WIND) {
        return strdup("wind");
    }
    if (type == PRESSURE) {
        return strdup("pressure");
    }
    return strdup("temperature");
}

char *state(int alert) {
    if (alert == 1) {
        return strdup("ALERT");
    }
    return strdup("OK");
}

int isAlert(int type, float value) {
    if (type == WIND) {
        return value >= 60;
    }
    if (type == PRESSURE) {
        return value <= 990;
    }
    return value >= 30;
}

int main(int argc, char *argv[]) {
    Msg m;
    char *buffer, *name, *wind, *pressure, *temp;
    int connected[NUM_SENSORS];
    int alert[NUM_SENSORS];
    float value[NUM_SENSORS];
    int i, type;

    if (argc != 2) {
        printF(MSG_USAGE);
        return 1;
    }

    queue = msgget(ftok(argv[1], 1), 0666 | IPC_CREAT);
    if (queue < 0) {
        printF(MSG_ERR_QUEUE);
        return 1;
    }
    signal(SIGINT, sigintHandler);

    for (i = 0; i < NUM_SENSORS; i++) {
        connected[i] = 0;
        alert[i] = -1;
        value[i] = 0;
    }

    printF(MSG_START);
    while (sensors < NUM_SENSORS) {
        msgrcv(queue, &m, sizeof(Msg) - sizeof(long), -NUM_SENSORS, 0);
        type = m.mtype - 1;
        if (m.op == 0 && connected[type] == 0) {
            connected[type] = 1;
            sensors++;
            name = sensorName(type);
            asprintf(&buffer, MSG_CONNECTED, name);
            printF(buffer);
            free(buffer);
            free(name);
        }
    }
    printF(MSG_ALL_CONNECTED);
    sendToAll(1, "");

    while (alert[WIND] != 1 || alert[PRESSURE] != 1 || alert[TEMP] != 1) {
        msgrcv(queue, &m, sizeof(Msg) - sizeof(long), -NUM_SENSORS, 0);
        type = m.mtype - 1;
        if (m.op == 2) {
            value[type] = atof(m.text);
            if (isAlert(type, value[type]) != alert[type]) {
                alert[type] = isAlert(type, value[type]);

                wind = state(alert[WIND]);
                pressure = state(alert[PRESSURE]);
                temp = state(alert[TEMP]);
                asprintf(&buffer, MSG_STATUS_LINE, value[WIND], wind, value[PRESSURE], pressure, value[TEMP], temp);
                sendToAll(3, buffer);
                free(buffer);
                free(wind);
                free(pressure);
                free(temp);

                asprintf(&buffer, MSG_STATUS, value[WIND], value[PRESSURE], value[TEMP]);
                printF(buffer);
                free(buffer);
            }
        }
    }

    signal(SIGINT, SIG_IGN);
    asprintf(&buffer, MSG_STORM, value[WIND], value[PRESSURE], value[TEMP]);
    sendToAll(4, buffer);
    free(buffer);

    printF(MSG_END_ALERT);
    endSession("END: storm alert declared\n");
    return 0;
}
