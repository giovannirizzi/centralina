#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "control_device.h"


int main(int argc, char *argv[]){

    CommandBind hub_commands[] = {
            {"canadd", &canadd_command}
    };

    DeviceData hub = {
            HUB, //DEVICE TYPE
            -1, //ID
            0, //STATE
            NULL,
            0, //NUM RECORDS
            NULL,
            0 //NUM SWITCHES
    };

    g_device = hub;

    init_control_device(argv, argc);

    device_loop(NULL, 0, hub_commands, 1);

    clean_control_device();

    exit(EXIT_SUCCESS);
}

void canadd_command(const char** args, size_t n_args){

    if(n_args != 1){
        send_response(INV_ARGS);
        return;
    }

    int tmp;
    if(string_to_int(args[0], &tmp)!=0) {
        send_response(INV_ARGS);
        return;
    }

    DeviceType new_device_type = device_string_to_type(device_type_to_string(tmp));
    print_error("New device type: %d\n", new_device_type);

    if(new_device_type == INVALID_TYPE) {
        send_response("can't link a control device that does not control devices");
    }
    else if(children_devices.size <= 0){
        send_response("yes");
    }
    else{
        DeviceType my_type;

        _Bool done = false;
        LineBuffer line_buffer = {NULL, 0};
        int child = 0;
        while(child < children_devices.size && !done){

            if(send_command_to_child(child, "getrealtype") == 0){

                read_child_response(child, &line_buffer);
                string_to_int(line_buffer.buffer, &my_type);
                my_type = device_string_to_type(device_type_to_string(my_type));

                print_error("My_tipe: %d\n",my_type);
                done = true;
            }
            else
                child++;
        }

        if(line_buffer.length > 0) free(line_buffer.buffer);

        if(!done || my_type == new_device_type)
            send_response("yes");
        else
            send_response("you can add only %s devices", device_type_to_string(my_type));
    }
}