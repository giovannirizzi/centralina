#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "centralina.h"


int main(int argc, char *argv[]){

    //ESEMPIO DI CODICE PER IMPLEMENTARE IL PARSING DEI COMANDI

    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    //Legge una riga finche non si verifica un errore 
    while ((nread = getline(&line, &len, stdin)) != -1) {
        //Sostituisce l'ultimo carattere della riga letta \n con \0
        if(nread != -1)
            line[nread-1] = '\0';

        printf("line: %s\n", line);

        char *tok = line, *end = line;
        while (tok != NULL) {
            //Sostituisce nella linea letta il primo " " che incontra con \0
            strsep(&end, " ");
            printf("Step: %s\n",tok);
            tok = end;
        }
        
    }

    free(line);
    exit(EXIT_SUCCESS);
}