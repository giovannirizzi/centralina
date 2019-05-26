#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "control_device.h"

/*
 * Implementa il comando getinfo dell' hub, esso maschera quello di default
 * perché l'hub oltre a stampare le sue caratteristica deve fare anche il
 * mirroring dei registri figli
 */
void getinfo_hub_command(const char** args, size_t n_args);

/*
 * Il comando canadd ha come unico argomento il realtype del device che
 * voglio aggiungere come figlio, da in risposta "yes" se è possibile aggiungere tale
 * dispositivo, altrimenti risponde con un errore.
 */
void canadd_command(const char** args, size_t n_args);

/* funzione per prendere i registri dei figli */
void getrecords(char *buffer);

LineBuffer line_buffer = {NULL, 0};

int main(int argc, char *argv[]){

    CommandBind hub_commands[] = {
            {"canadd", &canadd_command},
            {"getinfo", &getinfo_hub_command}
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

    device_loop(NULL, 0, hub_commands, 2);

    clean_control_device();

    if(line_buffer.length > 0) free(line_buffer.buffer);

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
    else if(g_children_devices.size <= 0){
        send_response("yes");
    }
    else{ //Calcolo il mio tipo in base ai figli
        DeviceType my_type;
        _Bool done = false;

        int child = 0;
        while(child < g_children_devices.size && !done){

            if(send_command_to_child(child, "getrealtype") == 0){

                read_child_response(child, &line_buffer);
                string_to_int(line_buffer.buffer, &my_type);
                my_type = device_string_to_type(device_type_to_string(my_type));
                done = true;
            }
            else
                child++;
        }

        if(!done || my_type == new_device_type)
            send_response("yes");
        else
            send_response("you can link only %s devices", device_type_to_string(my_type));
    }
}

void getinfo_hub_command(const char** args, size_t n_args){

    char info_string[400], tmp[400], state_str[50];
    _Bool override = false;

    const char *state = device_state_to_string(g_device.state, g_device.type);

    //Controllo se c'è un incongurenza con lo stato dei figli (override)
    int i, child_state;
    for(i=0; i<g_children_devices.size; i++){
        if(send_command_to_child(i, "getstate") == 0){
            read_child_response(i, &line_buffer);
            if(string_to_int(line_buffer.buffer, &child_state) == 0){
                if(child_state != g_device.state){
                    override = true;
                    break;
                }
            }
        }
        else
            i--;
    }

    if(override)
        sprintf(state_str, "%s with override", state);
    else
        sprintf(state_str, "%s", state);

    sprintf(info_string, "id=%d|type=%s|state=%s|connected devices=%d", g_device.id,
            device_type_to_string(g_device.type), state_str, g_children_devices.size);

    //Se sto controllando dispositivi prendo anche i loro registri
    if(g_children_devices.size > 0){
        memset(tmp, 0, sizeof(tmp)/ sizeof(char));
        getrecords(tmp);
        strcat(info_string, tmp);
    }

    send_response("%s", info_string);
}

void getrecords(char *buffer){

    //Prende i registri del figlio
    int last = 3;
    char* substrings[10];
    if(g_children_devices.size > 0)
        if(send_command_to_child(0, "getinfo") == 0){
            read_child_response(0, &line_buffer);
            last = divide_string(line_buffer.buffer, substrings, 10, "|");
        }

    //Calcola il massimo usage time tra i figli
    if(strstr(substrings[last-1], "usage time") != NULL){
        int maxtime = 0, tmptime = 0;
        int trovato = divide_string(substrings[last-1], substrings, 1, "=");
        sscanf(substrings[0], "%d", &maxtime);
        print_error("Trovato %d\n",trovato);
        print_error("MAX:TIME: %d\n",maxtime);
        int i;
        for(i=1; i<g_children_devices.size; i++){
            if(send_command_to_child(i, "getinfo") == 0){
                read_child_response(i, &line_buffer);

                last = divide_string(line_buffer.buffer, substrings, 5, "|");
                divide_string(substrings[last-1], substrings, 1, "=");
                sscanf(substrings[0], "%d", &tmptime);
                if(tmptime > maxtime)
                    maxtime = tmptime;
            }
            else
                i--;
        }
        char secocnds_str[20];
        seconds_to_string(maxtime, secocnds_str);
        sprintf(buffer, "|max usage time=%s", secocnds_str);
    }
    else{
        int i;
        char registry[100];
        for(i=2; i<last; i++){
            sprintf(registry, "|%s", substrings[i]);
            strcat(buffer, registry);
        }
    }


}