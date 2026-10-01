all: distribuida

main.o: main.c config.h xarxa.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c main.c -o main.o

config.o: config.c config.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c config.c -o config.o

xarxa.o: xarxa.c xarxa.h config.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c xarxa.c -o xarxa.o

distribuida: main.o config.o xarxa.o
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 main.o config.o xarxa.o -o distribuida

valgrind: distribuida
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --trace-children=yes ./distribuida 0 127.0.0.1 5000 READ_WRITE

clean:
	rm -f main.o config.o xarxa.o distribuida
