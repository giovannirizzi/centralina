#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "centralina.h"
#include "devices.h"



int main(int argc, char *argv[]){
    
    DeviceBase bulb;
    bulb.type = BULB;
    bulb.state = 0;

    return 0;
}