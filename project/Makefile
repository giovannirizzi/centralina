CC = gcc
CFLAGS = -std=gnu90
ODIR = obj
SDIR = src
BDIR = bin

help: ciao
	echo $<

centralina.o:
	$(CC) $(CFLAGS) -c $(SDIR)/centralina.c -o $(ODIR)/$@ 
	
centralina: centralina.o
	${CC} ${CFLAGS} -o $(BDIR)/centralina $(ODIR)/centralina.o