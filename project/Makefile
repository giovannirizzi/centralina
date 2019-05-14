CC = gcc
CFLAGS = -std=gnu90
ODIR = ./obj
SDIR = ./src
BIN = ./bin
VPATH = $(SDIR)

.PHONY: clean help
DEFAULT: help

#Lista di tutti gli eseguibili da compilare
LIST = fifo centralina bulb manualcontrol

all : $(LIST)

#Contiene la lista di tutti i possibili file oggetto separati da spazio es. /obj/centralina.o
OBJ_FILES = $(patsubst src/%.c, obj/%.o, $(wildcard src/*.c))

#I file oggetto hanno bisogno della directory $(ODIR)
$(OBJ_FILES): | $(ODIR)
#I file eseguibili hanno bisono della directory $(BIN)
$(LIST): | $(BIN)

fifo:
	@mkdir -p /tmp/centralina /tmp/centralina/devices

$(ODIR) $(BIN):
	@mkdir -p $@

$(ODIR)/centralina.o: centralina.c centralina.h device.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/device.o: device.c device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/bulb.o: bulb.c bulb.h device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/manualcontrol.o: manualcontrol.c manualcontrol.h device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/utils.o: utils.c utils.h device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/iteration_device.o: iteration_device.c iteration_device.h device.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/control_device.o: control_device.c control_device.h device.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

centralina: $(ODIR)/centralina.o $(ODIR)/device.o $(ODIR)/utils.o $(ODIR)/control_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^

bulb: $(ODIR)/bulb.o $(ODIR)/device.o $(ODIR)/utils.o $(ODIR)/iteration_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^ -lrt

manualcontrol : $(ODIR)/manualcontrol.o $(ODIR)/utils.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^

build: all

clean:
	rm -f -r $(ODIR)/* $(BIN)/* /tmp/centralina /tmp/centralina/devices

help:
	@echo Help
