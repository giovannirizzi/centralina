#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include "utils.h"
#include "manualcontrol.h"


/*
 * Possibile utilizzo:
 *
 * manualcontrol whois <id> (Se usiamo gli id e non i PID)
 * per restituire il PID dato un id
 *
 * manaulcontrol set <id/PID> <label> <value>
 * per inviare un segnale...
 *
 */

SignalMapping switch_labels_mapping[] = {{"open",SIG_OPEN},
                                        {"power",SIG_POWER},
                                        {"close",SIG_CLOSE}};

SignalMapping set_labels_mapping[] =    {{"time", SIG_TIME},
                                        {"delay", SIG_DELAY},
                                        {"percentage", SIG_PERC},
                                        {"temperature", SIG_TEMP}};
                    
//./manualcontrol switch 14 power on
//./manualcontrol set 15 temperature 45

Command input_command  = {NULL, 0, NULL, 0};

CommandBind command_bindings[] = {{"whois", &whois_command},
                                  {"switch", &switch_command},
                                  {"set", &set_command},
                                  {"help", &help_command}};

int main(int argc, char *argv[]){
    setlinebuf(stdout);

    if(argc>=2){
        input_command.name = argv[1];
        size_t i;
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

void send_signal(pid_t pid, RTSignalType signal, int val){

    union sigval val_union = {.sival_int = val };
    if(sigqueue(pid, SIGRTMIN + signal, val_union) != 0)
        perror("sigqueue");
}

void set_command(const char** args, const size_t n_args){
    //manca ancora il controllo su temperature, ecc. Anche il value secondo me dovrebbe avere un controllo però boh
    if(n_args !=3)
        print_error("usage: <set> <id> <register> <value>\n");
    else {
        device_id id;
        int id_control = string_to_int(args[0], &id);
        int value_control = string_to_int(args[2], &id);    //sta cosa è campata in aria, usa id solo perchè vuole un int
        if (id_control != 0)
            print_error_and_exit("invalid id %s\n", args[0]);
        if (value_control != 0)
            print_error_and_exit("invalid valued %s\n", args[2]);
        else{
            //ora posso mandare la signal
            int retval = get_signal_mapping(args[1], set_labels_mapping, 
                sizeof(set_labels_mapping)/sizeof(SignalMapping));
            if(retval == -1)
                print_error_and_exit("%s not valid\n",args[1]);  
            pid_t pid = whois(id);
            if(pid > 0)
                send_signal(pid, set_labels_mapping[retval].signal, value_control);
            else
                print_error_and_exit("failed to resolve id %d\n",id);
        }
    }
}

void switch_command(const char** args, const size_t n_args){
    if(n_args !=3)
        print_error("usage: <switch> <id> <label> <state>\n");
    else {
        device_id id;
        int id_control = string_to_int(args[0], &id);
        int state_control = string_to_state(args[2]);
        if (id_control != 0)
            print_error_and_exit("invalid id %s\n",args[0]);
        if (state_control == -1)
            print_error_and_exit("invalid state %s\n", args[2]);
        else{
            //ora posso mandare la signal
            int retval = get_signal_mapping(args[1], switch_labels_mapping, 
                sizeof(switch_labels_mapping)/sizeof(SignalMapping));
            if(retval == -1)
                print_error_and_exit("%s not valid\n",args[1]);  
            pid_t pid = whois(id);
            if(pid > 0)
                send_signal(pid, switch_labels_mapping[retval].signal, state_control);
            else
                print_error_and_exit("failed to resolve id %d\n",id);
        }
    }
}

void whois_command(const char** args, const size_t n_args){
    if(n_args !=1)
        print_error("usage: <whois> <id>\n");
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0){
            print_error_and_exit("invalid id %s\n",args[0]);
        }
        else{
            pid_t pid = whois(id);
            switch(pid){
            case -2:
                print_error_and_exit("invalid response\n");
                break;
            case -1:
                print_error_and_exit("timeout\n");
                break;
            default:
                printf("pid: %d\n",pid); 
            }
        }
    }
}

pid_t whois(device_id id){
    int device_id;
    int whois_fd_request, whois_fd_response;
    fd_set rfds;

    whois_fd_request = open_fifo(FIFO_WHOIS_REQUEST, O_RDWR);
    FILE* whois_stream_request = fdopen(whois_fd_request, "w");

    fprintf(whois_stream_request, "whois %d\n", id);
    fclose(whois_stream_request);

    whois_fd_response = open_fifo(FIFO_WHOIS_RESPONSE, O_RDWR);
    FILE* whois_stream_response = fdopen(whois_fd_response, "r");

    FD_ZERO(&rfds);
    FD_SET(whois_fd_response, &rfds);

    struct timeval timeout = {1, 0};
    int retval = select(whois_fd_response+1, &rfds, NULL, NULL, &timeout);

    if (retval == -1){
        perror_and_exit("select");
    }
    else if(retval){
        if(FD_ISSET(whois_fd_response, &rfds)){

            ssize_t nread = read_line(whois_stream_response, &input_command.name,
                    &input_command.len_name);

            pid_t pid; 
            if(string_to_int(input_command.name, &pid) != 0)
                return -2;
            else
                return pid;
        }
    }
    else{
        return -1; //timeout
    }

    close(whois_fd_response);
}

int get_signal_mapping(const char* label, const SignalMapping mappings[], const size_t n){
    int i;
    for(i=0; i<n; i++){
        if(strcmp(label,mappings[i].label) == 0)
            return i;
    }
    return -1;
}

void help_command(const char** args, const size_t n_args){
    printf("\
    available commands: \n\
    - set: set value of the identified device\n\
            usage: <set> <id> <register> <value>\n\
    - whois: return pid of the identified device\n\
            usage: <whois> <id>\n\
    - switch: turn on/off the identified device\n\
            usage: <switch> <id> <label> <on/off>\n\
    - help: show available commands\n\
            usage: <help>\
    \n");
}