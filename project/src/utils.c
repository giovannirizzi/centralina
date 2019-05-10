#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <linux/limits.h>
#include <fcntl.h>
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

int handle_signal(const RTSignal *s, const SignalBind s_bindings[], const size_t n){

    int i;
    for(i=0; i<n; i++)
        if(s->type == s_bindings[i].type){
            s_bindings[i].exec_command(s->value);
            return 0;
        }
    return -1;
}

ssize_t read_line(FILE* stream, char** buffer, size_t *n){

    ssize_t nread = getline(buffer, n, stream);

    if(nread < 0)
        perror_and_exit("error getline");

    //Se alla fine della linea letta c'è un \n lo sostituisce con \0
    if(nread > 0 && (*buffer)[nread-1] == '\n')
        (*buffer)[nread-1] = '\0';

    return nread;
}

void read_incoming_command(FILE* in, Command *c){

    read_line(in, &(c->name), &(c->len_name));

    c->n_args = divide_string(c->name, c->args, MAX_COMMAND_ARGS, " ");
}

int open_fifo(const char* path, mode_t access_mode){

    int fd;
    if(mkfifo(path, S_IRWXU) == -1 && errno != EEXIST)
        perror_and_exit("mkfifo");
    else{
        fd = open(path, access_mode);
        if(fd == -1)
            perror_and_exit("open_fifo");
    }

    return fd;
}


void read_incoming_signal(int sfd, RTSignal *signal){

    static struct signalfd_siginfo fdsi;
    ssize_t s;

    s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo))
        perror_and_exit("read signalfd");

    //Il cast non dovrebbe dare problemi...
    signal->type = (RTSignalType)fdsi.ssi_signo - SIGRTMIN;
    signal->value = fdsi.ssi_int;
}

void send_command(FILE* out, char* format, ...){

    va_list args;
    va_start(args,format);
    vfprintf(out, format, args);
    if(format[strlen(format)-1] != '\n')
        fprintf(out,"\n");
    va_end(args);
}

char* get_absolute_executable_dir(){

    static char path[PATH_MAX] = {0};

    if(*path == 0){

        ssize_t  pra = readlink("/proc/self/exe", path, PATH_MAX - 1);
        const char ch = '/';
        char *last_slash = strrchr(path, ch);
        if (last_slash)
            *last_slash = '\0';
        else
            print_error("error: get_absolute_executable_dir\n");
    }
    return path;
}