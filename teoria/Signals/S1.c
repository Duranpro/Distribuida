/*
** S1.c  –  SubastaLS
**
** Autor: Marcos Ruiz-Flores Vicente
**
*/

#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include "auction_support.h"

#define GREEN       "\x1B[32m"
#define YELLOW      "\x1B[33m"
#define RESET       "\x1B[0m"
#define TURN_TIME   10
#define BID_STEP    10

typedef struct {
    char    *name;
    int     money;
}   tPlayer;

typedef struct {
    int     product;
    int     owner;
    int     paid;
}   tSale;

typedef struct {
    tSale   *list;
    int     count;
}   tSales;

/* Globales necesarias para los signals */
int     g_signal;
int     g_time_left;
tPlayer *g_players;
tSales  g_sales;
char    *g_line;

void print(char *s) {
    write(1, s, strlen(s));
}

void print_player(int player, char *msg) {
    if (player == 0) {
        print(GREEN);
    } else {
        print(YELLOW);
    }
    print(msg);
    print(RESET);
}

void show_signal_help() {
    char *msg;

    asprintf(&msg, "  PID: %d | SIGUSR1 pujar | SIGUSR2 pasar | SIGTERM salir\n", getpid());
    print(msg);
    free(msg);
}

void handler(int sig) {
    char *msg;

    signal(sig, handler);
    if (sig == SIGTERM) {
        print("\n  Cerrando programa...\n");
        free(g_players[0].name);
        free(g_players[1].name);
        free(g_line);
        free(g_sales.list);
        free_products();
        print("  Memoria liberada correctamente.\n");
        exit(0);
    } else if (sig == SIGINT) {
        show_signal_help();
    } else if (sig == SIGALRM) {
        alarm(1);
        if (g_time_left > 0) {
            g_time_left--;
            asprintf(&msg, "  Tiempo restante: %d\n", g_time_left);
            print(msg);
            free(msg);
        }
    } else if (sig == SIGUSR1 || sig == SIGUSR2) {
        g_signal = sig;
    } else {
        asprintf(&msg, "  Senal no gestionada ignorada: %d\n", sig);
        print(msg);
        free(msg);
    }
}

char *readUntil() {
    char c = '\0';
    char *line;
    int i = 0;

    g_line = malloc(1);
    g_line[0] = '\0';
    while (read(0, &c, 1) > 0 && c != '\n') {
        g_line = realloc(g_line, i + 2);
        g_line[i++] = c;
        g_line[i] = '\0';
    }
    line = g_line;
    g_line = NULL;
    if (c != '\n') {
        free(line);
        return NULL;
    }
    return line;
}

char *ask(char *prompt) {
    char *line;

    print(prompt);
    line = readUntil();
    if (line == NULL) {
        raise(SIGTERM);
    }
    return line;
}

int is_valid_money(char *s) {
    int i = 0;

    while (s[i] >= '0' && s[i] <= '9') {
        i++;
    }
    return s[i] == '\0' && atoi(s) > 0;
}

void create_players(tPlayer players[2]) {
    char *line;
    int i;

    for (i = 0; i < 2; i++) {
        line = ask("  Nombre del jugador: ");
        while (line[0] == '\0') {
            free(line);
            print("  El nombre no puede estar vacio. Intentalo de nuevo.\n");
            line = ask("  Nombre del jugador: ");
        }
        players[i].name = line;

        line = ask("  Dinero disponible: ");
        while (!is_valid_money(line)) {
            free(line);
            print("  Introduce un numero valido (mayor que 0).\n");
            line = ask("  Dinero disponible: ");
        }
        players[i].money = atoi(line);
        free(line);
    }
    print("  Jugadores creados correctamente.\n");
}

void show_inventory(tPlayer players[2], tSales *sales) {
    char *msg;
    int i, j, count;

    for (i = 0; i < 2; i++) {
        asprintf(&msg, "  Inventario de %s. Dinero restante: %d\n", players[i].name, players[i].money);
        print_player(i, msg);
        free(msg);
        count = 0;
        for (j = 0; j < sales->count; j++) {
            if (sales->list[j].owner == i) {
                asprintf(&msg, "    - %s por %d euros\n", get_product_name(sales->list[j].product), sales->list[j].paid);
                print_player(i, msg);
                free(msg);
                count++;
            }
        }
        if (count == 0) {
            print_player(i, "    Sin productos.\n");
        }
    }
}

int run_turn(tPlayer *player, int turn, int *price) {
    char *msg = NULL;
    int result = -1;

    g_signal = 0;
    g_time_left = TURN_TIME;
    alarm(1);
    while (result == -1) {
        pause();
        if (g_signal == SIGUSR1 && player->money >= *price + BID_STEP) {
            *price += BID_STEP;
            asprintf(&msg, "  %s puja %d euros.\n", player->name, *price);
            result = 1;
        } else if (g_signal == SIGUSR1) {
            asprintf(&msg, "  %s no tiene dinero suficiente y pasa.\n", player->name);
            result = 0;
        } else if (g_signal == SIGUSR2) {
            asprintf(&msg, "  %s pasa.\n", player->name);
            result = 0;
        } else if (g_time_left == 0) {
            asprintf(&msg, "  %s pierde el turno por tiempo.\n", player->name);
            result = 0;
        }
        g_signal = 0;
    }
    alarm(0);
    print_player(turn, msg);
    free(msg);
    return result;
}

void start_auction(tPlayer players[2], tSales *sales) {
    char *msg;
    int i, turn, price, bidder, passes;

    for (i = 0; i < get_product_count(); i++) {
        price = get_product_price(i);
        bidder = -1;
        passes = 0;
        turn = 0;
        asprintf(&msg, "\n  Producto: %s | Precio inicial: %d\n", get_product_name(i), price);
        print(msg);
        free(msg);
        while (passes == 0 || (passes == 1 && bidder == -1)) {
            asprintf(&msg, "  Turno de %s. Precio actual: %d\n", players[turn].name, price);
            print_player(turn, msg);
            free(msg);
            show_signal_help();
            if (run_turn(&players[turn], turn, &price)) {
                bidder = turn;
                passes = 0;
            } else {
                passes++;
            }
            turn = 1 - turn;
        }
        if (bidder == -1) {
            print("  Producto sin comprador.\n");
        } else {
            players[bidder].money -= price;
            sales->list = realloc(sales->list, sizeof(tSale) * (sales->count + 1));
            sales->list[sales->count].product = i;
            sales->list[sales->count].owner = bidder;
            sales->list[sales->count].paid = price;
            sales->count++;
            asprintf(&msg, "  %s gana %s por %d euros.\n", players[bidder].name, get_product_name(i), price);
            print_player(bidder, msg);
            free(msg);
        }
    }
}

void menu(tPlayer players[2], tSales *sales) {
    char *option;
    int choice, created;

    while (1) {
        created = players[1].name != NULL;
        print("\n  ===== SUBASTA =====\n  1. Crear jugadores");
        if (created) {
            print(" [no disponible]");
        }
        option = ask("\n  2. Iniciar subasta\n  3. Ver inventarios\n  Opcion: ");
        choice = -1;
        if (strlen(option) == 1) {
            choice = option[0] - '0';
        }
        free(option);

        if (choice == 1 && created) {
            print("  Los jugadores ya han sido creados.\n");
        } else if (choice == 1) {
            create_players(players);
        } else if ((choice == 2 || choice == 3) && !created) {
            print("  Primero debes crear los dos jugadores.\n");
        } else if (choice == 2) {
            start_auction(players, sales);
        } else if (choice == 3) {
            show_inventory(players, sales);
        } else {
            print("  Opcion no valida.\n");
        }
    }
}

int main(int argc, char **argv) {
    tPlayer players[2];
    char *msg;
    int i;

    players[0].name = NULL;
    players[0].money = 0;
    players[1].name = NULL;
    players[1].money = 0;
    g_players = players;
    g_sales.list = NULL;
    g_sales.count = 0;
    g_line = NULL;
    for (i = 1; i < NSIG; i++) {
        if (i != SIGKILL && i != SIGSTOP) {
            signal(i, handler);
        }
    }
    if (argc != 2) {
        print("  Uso: ./S1 productos.txt\n");
        return 1;
    }
    if (!load_products(argv[1])) {
        print("  Error al cargar productos.\n");
        return 1;
    }
    asprintf(&msg, "  PID del proceso: %d\n", getpid());
    print(msg);
    free(msg);
    menu(players, &g_sales);
    return 0;
}
