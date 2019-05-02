#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "centralina.h"
#include "devices.h"
#include "utils.h"


int main(int argc, char *argv[]){

    //ESEMPIO DI CODICE PER IMPLEMENTARE IL PARSING DEI COMANDI
    
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    char *args[3];
    device_id id;

    printf("#>");

    //Legge una riga finche non si verifica un errore 
    while ((nread = getline(&line, &len, stdin)) != -1) {
        //Sostituisce l'ultimo carattere della riga letta \n con \0
        if(nread != -1)
            line[nread-1] = '\0';

        //Interprento la riga letta come: comando arg0 arg1 arg2
        size_t num_args = divide_line_into_substrings(line, args, sizeof(args)/sizeof(char*));
        
        int code = interpret_command((const char*)line, args, num_args);
        
        if(code == -1)
            fprintf(stderr, "unknown command %s\n", line);
        else if(code == 1)
            break;

        printf("#>");
    }

    free(line);
    exit(EXIT_SUCCESS);
}

void list(){
    printf("Lista devices: \n");
}

int add_device(DeviceType device){
    /*
     * Fork ed exec in base al device
     * Creazione e settaggio delle pipe per comunicare
     * Magari attendo una risposta
     */
    if(strcmp(device_type_to_string(device), "bulb") == 0){
        printf("Adding %s...\n", device_type_to_string(device));
        printf("PID padre: %d\n", getpid());
        pid_t pid = fork();
        if(pid == -1){
            printf("Forking failed!\n");
            return(1);
        }else if(pid == 0){
            char *args[]={"./bin/bulb",NULL};
            execve(args[0],args);
        }
    }else if(strcmp(device_type_to_string(device), "window") == 0){

    }
}

int delete_device(device_id device){

    printf("Delete %d\n",device);

    /*
     * Se è la centralina non posso eliminarla
     * mando un segnanle del <device> ha tutti i dispositivi
     * collegati
     * Magari attendo una risposta
     */
}

int link_device(device_id device1, device_id device2){

    printf("Link %d to %d\n",device1,device2);

    /*
     * Mando un info <device1>
     * Mando un del <device1>
     * Mando un add <device2> <info>
     * Se device2 è la centralina non mando niente,
     * eseguirò un metodo che aggiunge il dispositivo
     * alla centralina e fa un set
     * Magari attendo una risposta
     */
}

int switch_device(device_id device, const char* label, const char* pos){

    printf("Switch device %d, label: %s, pos: %s\n", device, label, pos);

    /*
     * Mando uno switch <device> <label> <pos>
     * Magari attendo una risposta
     */
}

int get_info(device_id device){

    printf("Info %d\n",device);

    /*
    * Mando una info <device>
    * Parsing della risosta e visualizzo a video le info
    */
}

size_t divide_line_into_substrings(char* line, char* args[], size_t max_args){

    char *start = line, *end = line;
    size_t num_args = 0;

    if(start != NULL){
        strsep(&end, " ");
        start = end;

        while (start != NULL && num_args < max_args) {
            //Sostituisce nella stringa il primo " " che incontra con \0
            strsep(&end, " ");
            args[num_args++] = start;
            start = end;
        }
    }
    
    return num_args;
}

int interpret_command(const char* command, char* args[], const size_t num_args){

    if(strcmp(command, "list") == 0){
        list();
    }
    else if(strcmp(command, "help") == 0){
        printf("Help...\n");
    }
    else if(strcmp(command, "exit") == 0){
        return 1;
    }
    else if(strcmp(command, "add") == 0){
        
        if(num_args != 1)
            fprintf(stderr, "usage: add <device>\n");
        else{
            DeviceType device = device_string_to_type(args[0]);
            if(device != INVALID_TYPE && device != CENTRALINA)
                add_device(device);
            else
                fprintf(stderr, "invalid device %s\n", args[0]);
        }
    }
    else if(strcmp(command, "del") == 0){

        if(num_args != 1)
            fprintf(stderr, "usage: del <id>\n");
        else{
            device_id id;
            if(!string_to_device_id(args[0], &id))
                fprintf(stderr, "invalid id %s\n", args[0]);
            else
                delete_device(id);
        }
    }
    else if(strcmp(command, "link") == 0){

        if(num_args != 3 || strcmp(args[1], "to") != 0)
            fprintf(stderr, "usage: link <id> to <id>\n");
        else{
            device_id id1, id2;
            if(!string_to_device_id(args[0], &id1))
                fprintf(stderr, "invalid id %s\n", args[0]);
            else if(!string_to_device_id(args[2], &id2))
                fprintf(stderr, "invalid id %s\n", args[2]);
            else
                link_device(id1, id2);     
        }
    }
    else if(strcmp(command, "switch") == 0){

        if(num_args != 3)
            fprintf(stderr, "usage: switch <id> <label> <pos>\n");
        else{
            device_id id;
            if(!string_to_device_id(args[0], &id))
                fprintf(stderr, "invalid id %s\n", args[0]);
            else
                switch_device(id, args[1], args[2]);
        }
    }
    else if(strcmp(command, "info") == 0){

        if(num_args != 1)
            fprintf(stderr, "usage: info <id>\n");
        else{
            device_id id;
            if(!string_to_device_id(args[0], &id))
                fprintf(stderr, "invalid id %s\n", args[0]);
            else
                get_info(id);
        }
    }
    else
        return -1;

    return 0;
}