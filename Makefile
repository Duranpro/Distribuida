CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -Wdeclaration-after-statement
LDLIBS =
TARGET = distribuida

$(TARGET): main.c config.c xarxa.c config.h xarxa.h
	$(CC) $(CFLAGS) main.c config.c xarxa.c -o $@ $(LDLIBS)

.PHONY: test
test: $(TARGET)
	python3 tests/integracio.py ./$(TARGET)
	python3 tests/peticions.py ./$(TARGET)
