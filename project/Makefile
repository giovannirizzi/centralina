CC = gcc
CFLAGS = -std=gnu90
ODIR = ./obj
SDIR = ./src
BIN = ./bin
VPATH = $(SDIR)

.PHONY: clean help
DEFAULT: help

#Lista di tutti gli eseguibili da compilare
LIST = centralina bulb manualcontrol

all : $(LIST)

#Contiene la lista di tutti i possibili file oggetto separati da spazio es. /obj/centralina.o
OBJ_FILES = $(patsubst src/%.c, obj/%.o, $(wildcard src/*.c))

#I file oggetto hanno bisogno della directory $(ODIR)
$(OBJ_FILES): | $(ODIR)
#I file eseguibili hanno bisono della directory $(BIN)
$(LIST): | $(BIN)

$(ODIR) $(BIN):
	@mkdir -p $@

$(ODIR)/centralina.o: centralina.c centralina.h devices.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/devices.o: devices.c devices.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/bulb.o: bulb.c bulb.h devices.h
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/manualcontrol.o: manualcontrol.c manualcontrol.h devices.h
	$(CC) $(CFLAGS) -c $< -o $@

centralina: $(ODIR)/centralina.o $(ODIR)/devices.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^

bulb: $(ODIR)/bulb.o $(ODIR)/devices.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^

manualcontrol : $(ODIR)/manualcontrol.o
	${CC} ${CFLAGS} -o $(BIN)/$@ $^

build: all

clean:
	rm -f -r $(ODIR)/* $(BIN)/*

help:
	@echo Help
