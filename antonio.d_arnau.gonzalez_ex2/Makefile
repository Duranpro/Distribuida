all: ProcessA ProcessB ProcessLWA1 ProcessLWA2 ProcessLWA3 ProcessLWB1 ProcessLWB2 ProcessLWB3

xarxa.o: xarxa.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c xarxa.c -o xarxa.o

rellotges.o: rellotges.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -c rellotges.c -o rellotges.o

ProcessA.o: pesant.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -DGRUP=0 -c pesant.c -o ProcessA.o

ProcessA: ProcessA.o xarxa.o rellotges.o
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 ProcessA.o xarxa.o rellotges.o -o ProcessA

ProcessB.o: pesant.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -DGRUP=1 -c pesant.c -o ProcessB.o

ProcessB: ProcessB.o xarxa.o rellotges.o
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 ProcessB.o xarxa.o rellotges.o -o ProcessB

ProcessLWA1.o: lleuger_a.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -DIDENTIFICADOR=0 -c lleuger_a.c -o ProcessLWA1.o

ProcessLWA1: ProcessLWA1.o xarxa.o rellotges.o
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 ProcessLWA1.o xarxa.o rellotges.o -o ProcessLWA1

ProcessLWA2.o: lleuger_a.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -DIDENTIFICADOR=1 -c lleuger_a.c -o ProcessLWA2.o

ProcessLWA2: ProcessLWA2.o xarxa.o rellotges.o
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 ProcessLWA2.o xarxa.o rellotges.o -o ProcessLWA2

ProcessLWA3.o: lleuger_a.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -DIDENTIFICADOR=2 -c lleuger_a.c -o ProcessLWA3.o

ProcessLWA3: ProcessLWA3.o xarxa.o rellotges.o
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 ProcessLWA3.o xarxa.o rellotges.o -o ProcessLWA3

ProcessLWB1.o: lleuger_b.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -DIDENTIFICADOR=0 -c lleuger_b.c -o ProcessLWB1.o

ProcessLWB1: ProcessLWB1.o xarxa.o rellotges.o
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 ProcessLWB1.o xarxa.o rellotges.o -o ProcessLWB1

ProcessLWB2.o: lleuger_b.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -DIDENTIFICADOR=1 -c lleuger_b.c -o ProcessLWB2.o

ProcessLWB2: ProcessLWB2.o xarxa.o rellotges.o
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 ProcessLWB2.o xarxa.o rellotges.o -o ProcessLWB2

ProcessLWB3.o: lleuger_b.c comu.h
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 -DIDENTIFICADOR=2 -c lleuger_b.c -o ProcessLWB3.o

ProcessLWB3: ProcessLWB3.o xarxa.o rellotges.o
	gcc -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement -std=c99 -g -O0 ProcessLWB3.o xarxa.o rellotges.o -o ProcessLWB3

clean:
	rm -f xarxa.o rellotges.o ProcessA.o ProcessB.o ProcessLWA1.o ProcessLWA2.o ProcessLWA3.o ProcessLWB1.o ProcessLWB2.o ProcessLWB3.o ProcessA ProcessB ProcessLWA1 ProcessLWA2 ProcessLWA3 ProcessLWB1 ProcessLWB2 ProcessLWB3

