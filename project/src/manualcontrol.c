#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <sys/stat.h> 
#include <sys/types.h> 
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

Command input_command  = {NULL, NULL, 0};

CommandBind command_bindings[] = {{"whois", &whois_command},
                                  {"switch", &switch_command},
                                  {"set", &set_command},
                                  {"help", &help_command}};

int main(int argc, char *argv[]){
    setlinebuf(stdout);
    int fd;
    char *myfifo = "/tmp/myfifo";
    mkfifo(myfifo, 0666); // mkfifo(<pathname>, <permission>) 
    char str2[80];
        while(fgets(str2, 80, stdin)){
            fd = open(myfifo, O_WRONLY);
            write(fd, str2, strlen(str2)+1); // write and close
        } // input from user, maxlen=80 
        close(fd);
    

    if(argc <= 1){
        help_command(NULL, 0);
        exit(EXIT_FAILURE);
    }
    else{

        input_command.name = argv[1];
        int i;
        for(i=0; i<argc && i<MAX_COMMAND_ARGS+2; i++){
            input_command.args[i] = argv[i+2];
        }
        input_command.n_args = i;

        int code = handle_command(&input_command, command_bindings,
                                  sizeof(command_bindings)/sizeof(CommandBind));

        if(code == -1){
            print_error("Invalid Command\n");
            help_command(NULL, 0);
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}

void send_signal(pid_t pid, SignalType signal, int val){

    union sigval val_union = {.sival_int = val };
    if(sigqueue(pid, SIGRTMIN + signal, val_union) != 0)
        perror("sigqueue");
}

void set_command(const char** args, const size_t n_args){

    if(n_args !=5)
        print_error("Numero parametri comandi incorretto...");
    else {
        int device_id;
        int id_control = string_to_int(args[2], &device_id);
        int state_control = string_to_state(args[4]);
        if (id_control != 0)
            print_error("Conversion failed");
        if (state_control == -1)
            print_error("Stato on/off not valid");
        else
            //ora posso mandare la signal
            printf("Signal1\n");
        //send_signal(device_id, state_control);
    }
}

void whois_command(const char** args, const size_t n_args){

    if(n_args !=3)
        print_error("Numero parametri comando incorretto...");
    else{
        int device_id;
        int control= string_to_int(args[2], &device_id);
        if(control != 0)
            print_error("Conversion failed");
        else
            //ora posso mandare la signal
            printf("Signal2\n");
    }
}

void switch_command(const char** args, const size_t n_args){

}


void help_command(const char** args, const size_t n_args){

}