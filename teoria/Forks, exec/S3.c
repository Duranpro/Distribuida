/*
 * Solution S3 Operating Systems - Forks
 * Course 2026-27
 *
 * @author: Felipe Trejos（◕‿◕）
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "S3_library.h"

Sector* read_sectors(const char* path, int* num_sectors) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printFD(STDERR_FILENO, "Error opening binary file\n");
        return NULL;
    }

    Sector* sectors = NULL;
    int n = 0;
    Sector tmp;

    while (read(fd, &tmp, sizeof(Sector)) == sizeof(Sector)) {
        sectors = realloc(sectors, sizeof(Sector) * (n + 1));
        sectors[n++] = tmp;
    }
    close(fd);

    *num_sectors = n;
    return sectors;
}

static void run_worker(int director_id, int worker_index) {
    srand(getpid() ^ time(NULL));
    int spice = rand() % 101;

    char* filename;
    asprintf(&filename, "worker_%d_%d.txt", director_id, worker_index);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    free(filename);
    if (fd < 0) exit(1);

    char* spice_str;
    asprintf(&spice_str, "%d\n", spice);
    write(fd, spice_str, strlen(spice_str));
    free(spice_str);
    close(fd);

    exit(0);
}

void run_director(Sector s) {
    // Fork all workers
    for (int i = 0; i < s.num_workers; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            printFD(STDERR_FILENO, "Error creating the fork\n");
            exit(1);
        }
        if (pid == 0) {
            run_worker(s.director_id, i);
        }
    }

    for (int i = 0; i < s.num_workers; i++) {
        wait(NULL);
    }

    int total = readWorkerSpice(s.director_id, s.num_workers);

    int worm_spawns = (total > s.spice_target) ? 1 : 0;

    pid_t worm_pid = fork();
    if (worm_pid < 0) {
        printFD(STDERR_FILENO, "Error creating the fork\n");
        exit(1);
    }
    if (worm_pid == 0) {
        char* flag;
        asprintf(&flag, "%d", worm_spawns);
        execlp("./shai_hulud", "./shai_hulud", s.zone_name, flag, NULL);
        exit(1);
    }
    wait(NULL);

    if (worm_spawns) {
        total = total / 2;
    }

    char* filename;
    asprintf(&filename, "director_%d.txt", s.director_id);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    free(filename);
    if (fd < 0) exit(1);

    char* total_str;
    asprintf(&total_str, "%d\n", total);
    write(fd, total_str, strlen(total_str));
    free(total_str);
    close(fd);

    exit(0);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printFD(STDERR_FILENO, "usage: ./S3 <sectors.bin>\n");
        return 1;
    }

    signal(SIGINT, SIG_IGN);

    int n = 0;
    Sector* sectors = read_sectors(argv[1], &n);
    if (sectors == NULL) return 1;

    if (n == 0) {
        printFD(STDERR_FILENO, "No sectors found in binary file\n");
        free(sectors);
        return 1;
    }

    printFD(STDOUT_FILENO, "[paul]\tBeginning spice harvesting operations on Arrakis.\n\n");

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            printFD(STDERR_FILENO, "Error creating the fork\n");
            free(sectors);
            return 1;
        }
        if (pid == 0) {
            Sector sector = sectors[i];
            free(sectors);
            run_director(sector);
        }
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    printFD(STDOUT_FILENO, "[paul]\tHarvesting complete. Tallying spice production...\n\n");

    int grand_total = 0;
    char* msg;

    for (int i = 0; i < n; i++) {
        int yield = readDirectorSpice(sectors[i].director_id);
        grand_total += yield;

        int original = readWorkerSpice(sectors[i].director_id, sectors[i].num_workers);
        int worm = (original > sectors[i].spice_target) ? 1 : 0;

        if (worm) {
            asprintf(&msg, "[paul]\tZone %s - Spice harvested: %d (WORM ALERT: yield penalized)\n",
                sectors[i].zone_name, yield);
        } else {
            asprintf(&msg, "[paul]\tZone %s - Spice harvested: %d\n",
                sectors[i].zone_name, yield);
        }
        printFD(STDOUT_FILENO, msg);
        free(msg);
    }

    asprintf(&msg, "\n[paul]\tTotal spice production: %d\n", grand_total);
    printFD(STDOUT_FILENO, msg);
    free(msg);

    free(sectors);
    return 0;
}
