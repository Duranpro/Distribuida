CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement
LDLIBS =
TARGET = distribuida
ifeq ($(OS),Windows_NT)
TARGET = distribuida.exe
LDLIBS = -lws2_32
endif

$(TARGET): main.c config.c xarxa.c config.h xarxa.h
	$(CC) $(CFLAGS) main.c config.c xarxa.c -o $@ $(LDLIBS)

.PHONY: test
test: $(TARGET)
	python tests/integracio.py ./$(TARGET)
	python tests/peticions.py ./$(TARGET)
