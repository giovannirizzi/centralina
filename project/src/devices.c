#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include "devices.h"

int create_signalfd(int fd, SignalType signal1, ...){

    va_list ap;
    int i;
    sigset_t mask;

    sigemptyset(&mask);

    va_start(ap, signal1); 

    for (i = signal1; i >= 0; i = va_arg(ap, int))
        sigaddset(&mask, SIGRTMIN + i);

    va_end(ap);

    /* Block signals so that they aren't handled
        according to their default dispositions */
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        handle_error("sigprocmask");

    return signalfd(fd, &mask, 0);
}

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

int string_to_device_id(const char* string, device_id *id){

    int n;
    return sscanf(string, "%d %n", id, &n) == 1 && !string[n];
}