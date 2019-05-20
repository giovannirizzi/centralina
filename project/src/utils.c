#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200809L
#define  _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <linux/limits.h>
#include <fcntl.h>
#include "utils.h"
#include <time.h>

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

int string_to_switch_state(const char *string, int *state){

    if(strcmp("on", string) == 0)
       *state = 1;
    else if(strcmp("off", string) == 0)
        *state= 0;
    else
        return -1;

    return 0;
}
const char* device_state_to_string(int device_state, DeviceType dt){
    if(device_state == 0){
        if(dt == WINDOW || dt == FRIDGE)
            return "close";
        else  
            return "off";
    }
    else if(device_state == 1){
        if(dt == WINDOW || dt == FRIDGE)
            return "open";
        else  
            return "on";
    }
    else
        return "invalid args";
}

int string_to_int(const char* string, int *id){

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

ssize_t read_line(FILE* stream, LineBuffer *lb){

    ssize_t nread = getline(&lb->buffer, &lb->length, stream);

    //se ho letto un eof
    if(nread < 0){
        return -1;
    }

    //Se alla fine della linea letta c'è un \n lo sostituisce con \0
    if(nread > 0 && (lb->buffer)[nread-1] == '\n')
        lb->buffer[nread-1] = '\0';

    return nread;
}

int read_incoming_command(FILE* in, Command *c, LineBuffer *buffer){

    if(read_line(in, buffer) < 0)
        return -1;

    c->name = buffer->buffer;
    c->n_args = divide_string(c->name, c->args, MAX_COMMAND_ARGS, " ");

    return 0;
}

int open_fifo(const char* path, mode_t access_mode){

    int fd;
    if(mkfifo(path, S_IRWXU) == -1 && errno != EEXIST)
        perror_and_exit("mkfifo");
    else{
        fd = open(path, access_mode);
        if(fd == -1 && errno != ENXIO)
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

int time_to_string(int time, char* string_time){
    struct tm info;

    int hour = time & 0xffff;
    int min = (time >> 16) & 0xffff;

    info.tm_min = min;
    info.tm_hour = hour;

    if(strftime(string_time,80,"%H:%M", &info) == 0)
        return -1;
    return 0;
}

int string_to_time(const char* string_time, int* time){
    struct tm info;

    char* retval = strptime(string_time, "%H:%M", &info);

    if(retval == NULL || *retval != '\0')
        return -1;

    *time = info.tm_min;
    *time = *time << 16;
    *time = *time | info.tm_hour;

    return 0;
}


int create_timer(timer_t *timer){

    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGRTMIN+SIG_TICK;
    sigev.sigev_value.sival_ptr = timer;

    if (timer_create(CLOCK_MONOTONIC, &sigev, timer) != 0){
        perror("error timer_create");
        return -1;
    }
    return 0;
}

int set_timer_tick(timer_t timer, _Bool tick){

    struct itimerspec value = {{0,0},{0,0}};

    if(tick){
        value.it_value.tv_sec = 1;
        value.it_interval.tv_sec = 1;
    }

    if (timer_settime(timer, 0, &value, NULL) != 0) {
        perror("time_settime error!");
        return -1;
    }

    return 0;
}

int delete_timer(timer_t timer){

    if(timer_delete(timer) != 0){
        perror("time_settime error!");
        return -1;
    }
    return 0;
}

int seconds_to_string(int seconds, char* string){
    sprintf(string, "%d sec", seconds);
}