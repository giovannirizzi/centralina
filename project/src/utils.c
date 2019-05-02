#include <stdio.h>
#include <string.h>
#include <sys/types.h>
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
int string_to_device_id(const char* string, device_id *id){

    int n;
    return sscanf(string, "%d %n", id, &n) == 1 && !string[n];
}