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

        //printf("line: %s\n", line);

        char a[4][20];
        int i=0;
        char *tok = line, *end = line;
        while (tok != NULL) {
            //Sostituisce nella linea letta il primo " " che incontra con \0
            strsep(&end, " ");
            strcpy(a[i],tok);
            //printf("Step: %s\n",tok);
            tok = end;
            i++;
        }

        if(strcmp(line,"list")==0)
            list();
        if(strstr(line, "add") != NULL)
            add_device(a[1]);
        if(strstr(line, "del") != NULL)
            delete_device(atoi(a[1]));
        if(strstr(line, "link") != NULL)
            link_device(atoi(a[1]),atoi(a[3]));
        if(strstr(line, "switch") != NULL)
            printf("Comando: switch %s %s %s\n",a[1],a[2],a[3]);
        if(strstr(line, "info") != NULL)
            get_info(atoi(a[1]));
        
    }

    free(line);
    exit(EXIT_SUCCESS);
}

void list(){
    printf("Lista devices: \n");
}

int add_device(DEVICE_NAME device){
    if(strcmp(device,"hub")==0)
        printf("Adding %s...\n",device);
    if(strcmp(device,"timer")==0)
        printf("Adding %s...\n",device);
    if(strcmp(device,"bulb")==0)
        printf("Adding %s...\n",device);
    if(strcmp(device,"window")==0)
        printf("Adding %s...\n",device);
    if(strcmp(device,"fridge")==0)
        printf("Adding %s...\n",device);
}

int delete_device(DEVICE_ID device){
    printf("Delete %d\n",device);
}

int link_device(DEVICE_ID device1, DEVICE_ID device2){
    printf("Link %d to %d\n",device1,device2);
}

int get_info(DEVICE_ID device){
    printf("Info %d\n",device);
}