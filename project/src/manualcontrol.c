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
 * manaulcontrol set <id/PID> <label> <value>
 * set 18039 temperature 34
 * per inviare un segnale...
 *
 */

Command input_command  = {NULL, 0, NULL, 0};

CommandBind command_bindings[] = {{"whois", &whois_command},
                                  {"switch", &switch_command},
                                  {"set", &set_command},
                                  {"help", &help_command}};

int main(int argc, char *argv[]){
    setlinebuf(stdout);

    if(argc>=2){
        input_command.name = argv[1];
        int i;
        for(i=0; i<argc-2 && i<MAX_COMMAND_ARGS; i++){
            input_command.args[i] = argv[i+2];
        }
        input_command.n_args = i;
        int code = handle_command(&input_command, command_bindings,
                                  sizeof(command_bindings)/sizeof(CommandBind));

        if(code == -1){
            print_error("uknown command\n");
            help_command(NULL, 0);
            exit(EXIT_FAILURE);
        }
    }
    else{
        help_command(NULL, 0);
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void send_signal(pid_t pid, SignalType signal, int val){

    union sigval val_union = {.sival_int = val };
    if(sigqueue(pid, SIGRTMIN + signal, val_union) != 0)
        perror("sigqueue");
}

void set_command(const char** args, const size_t n_args){
    //manca ancora il controllo su temperature, ecc. Anche il value secondo me dovrebbe avere un controllo però boh
    if(n_args !=3){
        print_error("usage: <set> <id> <register> <value>\n");
        exit(EXIT_FAILURE);
    }
    else {
        int device_id;
        int id_control = string_to_int(args[0], &device_id);
        int value_control = string_to_int(args[2], &device_id);
        if (id_control != 0){
            print_error("conversion failed, id not valid\n");
            exit(EXIT_FAILURE);
        }
        if (value_control != 0){
            print_error("conversion failed, value not valid\n");
            exit(EXIT_FAILURE);
        }
        else
            //ora posso mandare la signal
            printf("signal1\n");
        //send_signal(device_id, state_control);
    }
}

void whois_command(const char** args, const size_t n_args){
   
    if(n_args !=1){
        print_error("usage: <whois> <id>\n");
    }
    else{
        int device_id;
        int control = string_to_int(args[0], &device_id);
        if(control != 0){
            print_error("Conversion failed");
            exit(EXIT_FAILURE);
        }
        else{
            int fd_whois_request, fd_whois_response;
            fd_set rfds;
            char buffer[80];
            sprintf(buffer, "whois %d", device_id);
            mkfifo(FIFO_WHOIS_REQUEST, 0666);

            fd_whois_request = open(FIFO_WHOIS_REQUEST, O_WRONLY);
            write(fd_whois_request, buffer, strlen(buffer)+1);
            close(fd_whois_request);

            fd_whois_response = open(FIFO_WHOIS_RESPONSE, O_RDWR);

            FD_ZERO(&rfds);
            FD_SET(fd_whois_response, &rfds);
            struct timeval timeout = {5, 0};
            int retval = select(fd_whois_response+1, &rfds, NULL, NULL, &timeout);

            if (retval == -1){
                perror("select()");
                exit(EXIT_FAILURE);
            }
            else if(retval){
                if(FD_ISSET(fd_whois_response, &rfds)){
                    int nread = read(fd_whois_response, buffer, 80); // read from FIFO
                    printf("Received string: %s, num bytes %d\n", buffer, nread);
                    int control = string_to_int(args[0], &device_id);
                    if(control != 0){
                        print_error("Conversion failed");
                        exit(EXIT_FAILURE);        
                    }
                    else{
                        printf("id: %d", device_id);
                        exit(EXIT_SUCCESS);
                    }
                }
            }
            else{
                printf("timeout\n");
                exit(EXIT_FAILURE);
            }

            close(fd_whois_response);
        }
    }
}

void switch_command(const char** args, const size_t n_args){
    //manca ancora il controllo su open-power-close 
    if(n_args !=3){
        print_error("usage: <switch> <id> <label> <state>\n");
        exit(EXIT_FAILURE);
    }
    else {
        int device_id;
        int id_control = string_to_int(args[0], &device_id);
        int state_control = string_to_state(args[2]);
        if (id_control != 0){
            print_error("conversion failed, id not valid\n");
            exit(EXIT_FAILURE);
        }
        if (state_control == -1){
            print_error("state on/off not valid\n");
            exit(EXIT_FAILURE);
        }
        else
            //ora posso mandare la signal
            printf("signal1\n");
        //send_signal(device_id, state_control);
    }
}


void help_command(const char** args, const size_t n_args){
    printf("available commands: \n");
    printf(" - set: set value of the identified device\n");
    printf("   usage: <set> <id> <register> <value>\n");
    printf(" - whois: return pid of the identified device\n");
    printf("   usage: <whois> <id>\n");
    printf(" - switch: change status of the identified device\n");
    printf("   usage: <switch> <id> <label> <state>\n");
    printf(" - help: show available commands\n");
    printf("   usage: <help>\n");
}

void list(){
    printf("available devices: \n");
    printf("interaction devices: \n");
    printf(" -  bulb\n");
    printf(" -  window\n");
    printf(" -  fridge\n");
    printf("interaction devices: \n");
    printf(" - controller\n");
    printf(" - hub\n");
    printf(" - timer\n");
    printf("active devices...\n");
}