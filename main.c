#include "config.h"


int main(int argc, char *argv[]) {
    Configuracio configuracio;

    if (llegir_arguments(argc, argv, &configuracio) != 0) {
        return 1;
    }

    mostrar_configuracio(&configuracio);

    return 0;
}