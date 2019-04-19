CC = gcc
CFLAGS = -std=gnu90
ODIR = obj
SDIR = src
VPATH = $(SDIR)

.PHONY: clean help
DEFAULT: help

all : centralina

#Contiene la lista di tutti i possibili file oggetto separati da spazio es. /obj/centralina.o
OBJ_FILES = $(patsubst src/%.c, obj/%.o, $(wildcard src/*.c))

#I file oggetto hanno bisogno della directori $(ODIR)
$(OBJ_FILES): | $(ODIR)

$(ODIR):
	@mkdir -p $@


$(ODIR)/centralina.o: centralina.c centralina.h
	$(CC) $(CFLAGS) -c $< -o $@
	
centralina: $(ODIR)/centralina.o
	${CC} ${CFLAGS} -o $@ $(ODIR)/centralina.o

build: all

clean:
	rm -f $(ODIR)/*.o all

help:
	@echo Ciao