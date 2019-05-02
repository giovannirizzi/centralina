#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/signalfd.h>
#include "manualcontrol.h"
#include "utils.h"


/*
 * Possibile utilizzo:
 *
 * manualcontrol whois <id> (Se usiamo gli id e non i PID)
 * per restituire il PID dato un id
 *
 * manaulcontrol set <id/PID> <label interruttore> <posizione>
 * set 18039 power on
 * per inviare un segnale...
 *
 */


int main(int argc, char *argv[]){
    
    //Controllo dei comandi set o whois con i relativi parametri che vengono passati
    int control = interpret_command(argv, argc);
    if(control == -1)
        printf("Unknown Command, bye");
    return EXIT_SUCCESS;
}

void send_signal(pid_t pid, SignalType signal, int val){

    union sigval val_union = {.sival_int = val };
    sigqueue(pid, SIGRTMIN + signal, val_union);
}

int interpret_command(char* args[], const size_t num_args){

    if(strcmp(args[1], "set") == 0){
        if(num_args !=5)
            printf("Numero parametri comandi incorretto...");
        else{
            int device_id;
            int id_control= string_to_device_id(args[2], &device_id);
            int state_control = string_to_state(args[4]);
                if(id_control == 0)
                    printf("Conversion failed");
                if(state_control == -1)
                    printf("Stato on/off not valid");
                else
                    //ora posso mandare la signal
                    printf("Signal1\n");
                    //send_signal(device_id, state_control);
        }
    }
    else if(strcmp(args[1], "whois") == 0){
        if(num_args !=3)
            printf("Numero parametri comando incorretto...");
        else{
            int device_id;
            int control= string_to_device_id(args[2], &device_id);
                if(control == 0)
                    printf("Conversion failed");
                else
                    //ora posso mandare la signal
                    printf("Signal2\n");
        }
    }
    else
        return -1;
    
}