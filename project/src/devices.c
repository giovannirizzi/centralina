#include "devices.h"
#include <string.h>

const char* device_type_to_string(DeviceType device_type){

    switch(device_type){

        case CENTRALINA:
            return "centralina";
        case BULB:
            return "bulb";

        default:
            return "invalid device type";
    }
}

DeviceType device_string_to_type(const char* device_string){

    if(strcmp("centralina", device_string) == 0)
        return CENTRALINA;
    else if(strcmp("bulb", device_string) == 0)
        return BULB;
    else
        return INVALID_TYPE;
}