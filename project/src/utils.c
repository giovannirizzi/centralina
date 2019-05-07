#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/signalfd.h>
#include "utils.h"

const char* device_type_to_string(DeviceType device_type){

    switch(device_type){

        case CENTRALINA:
            return "centralina";
        case HUB:
            return "hub";
        case TIMER:
            return "timer";
        case BULB:
            return "bulb";
        case WINDOW:
            return "window";
        case FRIDGE:
            return "fridge";
        default:
            return "invalid device type";
    }
}

DeviceType device_string_to_type(const char* device_string){

    if(strcmp("centralina", device_string) == 0)
        return CENTRALINA;
    else if(strcmp("hub", device_string) == 0)
        return HUB;
    else if(strcmp("timer", device_string) == 0)
        return TIMER;
    else if(strcmp("bulb", device_string) == 0)
        return BULB;
    else if(strcmp("window", device_string) == 0)
        return WINDOW;
    else if(strcmp("fridge", device_string) == 0)
        return FRIDGE;
    else
        return INVALID_TYPE;
}
int string_to_state(const char* device_state){
    if(strcmp("on", device_state) == 0)
        return 0;
    if(strcmp("off", device_state) == 0)
        return 1;
    else
        return -1;    
}
int string_to_int(const char* string, device_id *id){

    int n;
    return !(sscanf(string, "%d %n", id, &n) == 1 && !string[n]);
}

size_t divide_string(char *line, char **substrings, size_t max_substrings, const char *delimiter){

    char *start = line, *end = line;
    size_t num_substrings = 0;

    if(start != NULL){
        strsep(&end, delimiter);
        start = end;

        while (start != NULL && num_substrings < max_substrings) {
            //Sostituisce nella stringa il primo delimitatore che incontra con \0
            strsep(&end, delimiter);
            substrings[num_substrings++] = start;
            start = end;
        }
    }

    return num_substrings;
}

int handle_command(const Command *c, const CommandBind c_bindings[], const size_t n){

    int i;
    for(i=0; i<n; i++)
        if(strcmp(c->name, c_bindings[i].command_name) == 0){
            c_bindings[i].validate_and_exec_command((const char**)c->args, c->n_args);
            return 0;
        }
    return -1;
}

int handle_signal(const Signal *s, const SignalBind s_bindings[], const size_t n){

    int i;
    for(i=0; i<n; i++)
        if(s->signal_type == s_bindings[i].type){
            s_bindings[i].exec_command(s->signal_val);
            return 0;
        }
    return -1;
}

ssize_t read_line(FILE* stream, char** buffer, size_t *n){

    ssize_t nread = getline(buffer, n, stream);

    if(nread < 0)
        perror_and_exit("[-] Error getline");

    //Se alla fine della linea letta c'è un \n lo sostituisce con \0
    if(nread > 0 && (*buffer)[nread-1] == '\n')
        (*buffer)[nread-1] = '\0';

    return nread;
}

void read_incoming_signal(int sfd, Signal *signal){

    static struct signalfd_siginfo fdsi;
    ssize_t s;

    s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo))
        perror_and_exit("read signalfd");

    //Il cast non dovrebbe dare problemi...
    signal->signal_type = (SignalType)fdsi.ssi_signo - SIGRTMIN;
    signal->signal_val = fdsi.ssi_int;
}

int add_child(ChildrenDevices* c, ChildDevice d){
    if(c->size == MAX_CHILDREN)
        return -1;
    c->children[c->size] = d;
    c->size++;
    printf("SIZE = %d\n",c->size);
    return 0;
}

int delete_child(ChildrenDevices* c, int i){
    if(c->size == 0)
        return -1;
    for ( ; i <= c->size; i++)
        c->children[i] = c->children[i + 1];
    c->size--;
    return 0;
}