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

$(ODIR)/centralina.o: centralina.c centralina.h devices.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/devices.o: devices.c devices.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/bulb.o: bulb.c bulb.h devices.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/manualcontrol.o: manualcontrol.c manualcontrol.h devices.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/utils.o: utils.c utils.h devices.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/iteration_device.o: iteration_device.c iteration_device.h devices.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/control_device.o: control_device.c control_device.h devices.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

centralina: $(ODIR)/centralina.o $(ODIR)/devices.o $(ODIR)/utils.o $(ODIR)/control_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^

bulb: $(ODIR)/bulb.o $(ODIR)/devices.o $(ODIR)/utils.o $(ODIR)/iteration_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^

manualcontrol : $(ODIR)/manualcontrol.o $(ODIR)/utils.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^

build: all

clean:
	rm -f -r $(ODIR)/* $(BIN)/* /tmp/centralina /tmp/centralina/devices

help:
	@echo Help
