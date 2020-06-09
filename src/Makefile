TARGET = ../target

CC = gcc
CFLAGS = -g -Wall -pthread -I.
LDLIBS = -lrt -lncurses

BOLD=\e[1m
NC=\e[0m

all: simulador monitor jefe nave

clean:
	rm -r -f $(TARGET)

simulador:
	mkdir -p $(TARGET)
	$(CC) $(CFLAGS) mapa.c simulador.c manejadores.c -o $(TARGET)/simulador -lrt -lm

monitor:
	mkdir -p $(TARGET)
	$(CC) $(CFLAGS) gamescreen.c mapa.c monitor.c manejadores.c -o $(TARGET)/monitor -lrt -lncurses -lm

jefe:
	mkdir -p $(TARGET)
	$(CC) $(CFLAGS) jefe.c mapa.c manejadores.c -o $(TARGET)/jefe -lrt -lncurses -lm

nave:
	mkdir -p $(TARGET)
	$(CC) $(CFLAGS) nave.c mapa.c manejadores.c -o $(TARGET)/nave -lrt -lncurses -lm
