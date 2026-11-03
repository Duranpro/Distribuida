/*
 * Solution S0 Operating Systems - File Descriptors
 * Course 2026-27
 *
 * @author: Felipe Trejos（◕‿◕）
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define printF(x)    write(STDOUT_FILENO, (x), strlen(x))
#define printFD(f,x) write((f), (x), strlen(x))

typedef struct {
    char name[25];
    int model;
    int construction_date;
} ModelBin;

typedef struct {
    int modelID;
    int sessionID;
    char *question;
    int response_score;
    int empathy_index;
    int classification;
} Session;

typedef struct {
    ModelBin model;
    Session *sessions;
    int totalSessions;
    int finalVerdict;
} ModelData;

char* readUntil(int fd, char del){
    char* buffer = NULL;
    int i = 0;
    char aux;
    int len = read(fd, &aux, 1);

    if(len <= 0){
        return NULL;
    }

    while(len != 0 && aux != del && aux != '\r'){
        buffer = realloc(buffer, sizeof(char)*(i + 2));
        buffer[i] = aux;
        i++;
        len = read(fd, &aux, 1);
    }

    if(buffer == NULL){
        buffer = malloc(1);
        buffer[0] = '\0';
        return buffer;
    }
    buffer[i] = '\0';
    return buffer;
}

int classifySession(int response_score, int empathy_index) {
    float avg = (response_score + empathy_index) / 2.0;

    if(avg > 80.0){
        return 2;
    }
    if(avg >= 60.0){
        return 1;
    }
    return 0;
}

char* verdictToString(int verdict) {
    if(verdict == 2){
        return "CONSCIOUS";
    }
    if(verdict == 1){
        return "UNCERTAIN";
    }
    return "SIMULATED";
}

int findModelByID(ModelData *models, int totalModels, int modelID) {
    for(int i = 0; i < totalModels; i++) {
        if(models[i].model.model == modelID){
            return i;
        }
    }
    return -1;
}

ModelData* loadModels(char *filename, int *totalModels) {
    int fd = open(filename, O_RDONLY);
    ModelData *models = NULL;

    *totalModels = 0;
    if(fd < 0) {
        char *msg;
        asprintf(&msg, "Could not open models file: %s\n", filename);
        printF(msg);
        free(msg);
        return NULL;
    }

    while(1) {
        ModelBin tmp;
        int n = (int)read(fd, &tmp, sizeof(ModelBin));

        if(n == 0){
            break;
        }
        if(n < 0) {
            printF("Error reading models file\n");
            free(models);
            close(fd);
            return NULL;
        }
        if(n < (int)sizeof(ModelBin)) {
            printF("Models file appears truncated\n");
            break;
        }

        models = realloc(models, (size_t)(*totalModels + 1) * sizeof(ModelData));
        if(models == NULL) {
            close(fd);
            return NULL;
        }

        models[*totalModels].model = tmp;
        models[*totalModels].sessions = NULL;
        models[*totalModels].totalSessions = 0;
        models[*totalModels].finalVerdict = 2;
        (*totalModels)++;
    }

    close(fd);
    return models;
}

int loadSessions(char *filename, ModelData *models, int totalModels) {
    int fd = open(filename, O_RDONLY);
    Session session;

    if(fd < 0) {
        char *msg;
        asprintf(&msg, "Could not open sessions file: %s\n", filename);
        if(msg != NULL) {
            printF(msg);
            free(msg);
        }
        return -1;
    }

    while(1) {
        char *buf = readUntil(fd, '&');
        if(buf == NULL){
            break;
        }

        session.modelID = atoi(buf);
        free(buf);

        buf = readUntil(fd, '&');
        if(buf != NULL){
            session.sessionID = atoi(buf);
        }else{
            session.sessionID = 0;
        }
        free(buf);

        session.question = readUntil(fd, '&');

        buf = readUntil(fd, '&');
        if(buf != NULL){
            session.response_score = atoi(buf);
        }else{
            session.response_score = 0;
        }
        free(buf);

        buf = readUntil(fd, '\n');
        if(buf != NULL){
            session.empathy_index = atoi(buf);
        }else{
            session.empathy_index = 0;
        }
        free(buf);

        session.classification = classifySession(session.response_score, session.empathy_index);

        int i = findModelByID(models, totalModels, session.modelID);
        if(i != -1) {
            models[i].sessions = realloc(models[i].sessions, (models[i].totalSessions + 1) * sizeof(Session));
            models[i].sessions[models[i].totalSessions++] = session;

            if(session.classification < models[i].finalVerdict) {
                models[i].finalVerdict = session.classification;
            }
        }
    }
    close(fd);
    return 0;
}

void printReport(ModelData *models, int totalModels) {

    printF("=== TURING EVALUATION REPORT ===\n\n");
    for(int i = 0; i < totalModels; i++) {
        char *line;

        asprintf(&line, "Model: %s (ID=%d) - Build Year: %d\n", models[i].model.name, models[i].model.model, models[i].model.construction_date);
        if(line != NULL) {
            printF(line);
            free(line);
        }

        for(int j = 0; j < models[i].totalSessions; j++) {
            asprintf(&line, "\t[Session %d]\n", models[i].sessions[j].sessionID);
            if(line != NULL) {
                printF(line);
                free(line);
            }

            asprintf(&line, "\t\tQuestion: %s\n", models[i].sessions[j].question);
            if(line != NULL) {
                printF(line);
                free(line);
            }

            asprintf(&line, "\t\tResponse: %d | Empathy: %d --> %s\n", models[i].sessions[j].response_score, models[i].sessions[j].empathy_index, verdictToString(models[i].sessions[j].classification));
            if(line != NULL) {
                printF(line);
                free(line);
            }
        }

        if(models[i].totalSessions > 0) {
            asprintf(&line, "\tFinal Verdict: %s\n\n", verdictToString(models[i].finalVerdict));
            if(line != NULL) {
                printF(line);
                free(line);
            }
        } else {
            printF("\tVerdict: NO_DATA\n\n");
        }
    }
    printF("=== END OF REPORT ===\n");
}

void freeAll(ModelData *models, int totalModels) {
    for(int i = 0; i < totalModels; i++) {
        for(int j = 0; j < models[i].totalSessions; j++) {
            free(models[i].sessions[j].question);
        }
        free(models[i].sessions);
    }
    free(models);
}

int main(int argc, char **argv) {
    ModelData *models;
    int totalModels = 0;

    if(argc != 3) {
        printFD(STDERR_FILENO, "Usage: ./S0 <models.bin> <sessions.txt>\n");
        return 1;
    }

    models = loadModels(argv[1], &totalModels);
    if(models == NULL || totalModels == 0) {
        free(models);
        return 1;
    }

    if(loadSessions(argv[2], models, totalModels) != 0) {
        freeAll(models, totalModels);
        return 1;
    }

    printReport(models, totalModels);
    freeAll(models, totalModels);
    return 0;
}
