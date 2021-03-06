CC = gcc
CFLAGS = -std=gnu90
ODIR = ./obj
SDIR = ./src
BIN = ./bin
VPATH = $(SDIR)

.PHONY: clean help fifo
DEFAULT: help

#Lista di tutti gli eseguibili da compilare
LIST = fifo controller bulb manualcontrol window fridge hub timer

all : $(LIST)

#Contiene la lista di tutti i possibili file oggetto separati da spazio es. /obj/controller.o
OBJ_FILES = $(patsubst src/%.c, obj/%.o, $(wildcard src/*.c))

#I file oggetto hanno bisogno della directory $(ODIR)
$(OBJ_FILES): | $(ODIR)
#I file eseguibili hanno bisono della directory $(BIN)
$(LIST): | $(BIN)

fifo:
	@mkdir -p /tmp/centralina /tmp/centralina/devices

$(ODIR) $(BIN):
	@mkdir -p $@

$(ODIR)/controller.o: controller.c controller.h device.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/device.o: device.c device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/bulb.o: bulb.c device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/window.o: window.c device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/fridge.o: fridge.c device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/hub.o: hub.c device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/timer.o: timer.c device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/manualcontrol.o: manualcontrol.c manualcontrol.h device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/utils.o: utils.c utils.h device.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/iteration_device.o: iteration_device.c iteration_device.h device.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/control_device.o: control_device.c control_device.h device.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

controller: $(ODIR)/controller.o $(ODIR)/device.o $(ODIR)/utils.o $(ODIR)/control_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^ -lrt

bulb: $(ODIR)/bulb.o $(ODIR)/device.o $(ODIR)/utils.o $(ODIR)/iteration_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^ -lrt

manualcontrol : $(ODIR)/manualcontrol.o $(ODIR)/utils.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^ -lrt

window: $(ODIR)/window.o $(ODIR)/device.o $(ODIR)/utils.o $(ODIR)/iteration_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^ -lrt

fridge: $(ODIR)/fridge.o $(ODIR)/device.o $(ODIR)/utils.o $(ODIR)/iteration_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^ -lrt

hub: $(ODIR)/hub.o $(ODIR)/device.o $(ODIR)/utils.o $(ODIR)/control_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^ -lrt

timer: $(ODIR)/timer.o $(ODIR)/device.o $(ODIR)/utils.o $(ODIR)/control_device.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^ -lrt

build: all

clean:
	rm -f -r $(ODIR)/* $(BIN)/* /tmp/centralina /tmp/centralina/devices

help:
	@echo "    available commands:"
	@echo "    - build: builds all the project"
	@echo "    - <binary file> : builds only the identified binary file: "
	@echo "          controller, hub, bulb, fridge, window, timer, manualcontrol"
	@echo "    - clean: deletes all the executables file"
	@echo "    - help: shows this info"

