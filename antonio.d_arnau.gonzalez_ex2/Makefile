CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Wdeclaration-after-statement
COMUNS = xarxa.c rellotges.c
PROGRAMES = ProcessA ProcessB ProcessLWA1 ProcessLWA2 ProcessLWA3 ProcessLWB1 ProcessLWB2 ProcessLWB3

all: $(PROGRAMES)

ProcessA: pesant.c $(COMUNS) comu.h
	$(CC) $(CFLAGS) -DGRUP=0 pesant.c $(COMUNS) -o $@

ProcessB: pesant.c $(COMUNS) comu.h
	$(CC) $(CFLAGS) -DGRUP=1 pesant.c $(COMUNS) -o $@

ProcessLWA1: lleuger_a.c $(COMUNS) comu.h
	$(CC) $(CFLAGS) -DIDENTIFICADOR=0 lleuger_a.c $(COMUNS) -o $@

ProcessLWA2: lleuger_a.c $(COMUNS) comu.h
	$(CC) $(CFLAGS) -DIDENTIFICADOR=1 lleuger_a.c $(COMUNS) -o $@

ProcessLWA3: lleuger_a.c $(COMUNS) comu.h
	$(CC) $(CFLAGS) -DIDENTIFICADOR=2 lleuger_a.c $(COMUNS) -o $@

ProcessLWB1: lleuger_b.c $(COMUNS) comu.h
	$(CC) $(CFLAGS) -DIDENTIFICADOR=0 lleuger_b.c $(COMUNS) -o $@

ProcessLWB2: lleuger_b.c $(COMUNS) comu.h
	$(CC) $(CFLAGS) -DIDENTIFICADOR=1 lleuger_b.c $(COMUNS) -o $@

ProcessLWB3: lleuger_b.c $(COMUNS) comu.h
	$(CC) $(CFLAGS) -DIDENTIFICADOR=2 lleuger_b.c $(COMUNS) -o $@

clean:
	rm -f $(PROGRAMES)

.PHONY: all clean
