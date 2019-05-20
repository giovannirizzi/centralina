#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "device.h"
#include "utils.h"

const CommandBind BASE_COMMANDS[] = {{"getinfo", &getinfo_command},
                                     {"del", &del_command},
                                     {"getconf", &getconf_command},
                                     {"setconf", &setconf_command},
                                     {"getpid", &getpid_command},
                                     {"gettype", &gettype_command},
                                     {"iscontrolled", &iscontrolled_command},
                                     {"switch", &switch_command},
                                     {"set", &set_command},
                                     {"getrealtype", &getrealtype_command}};

sigset_t set_signal_mask(RTSignalType signal1, ...){

    va_list ap;
    int i;
    sigset_t mask;

    sigemptyset(&mask);

    va_start(ap, signal1);

    for (i = signal1; i >= 0; i = va_arg(ap, int))
        sigaddset(&mask, SIGRTMIN + i);

    va_end(ap);

    sigaddset(&mask, SIGCHLD);

    /* Blocca i segnali settati nella mask cosi da prevenire
     * la chiamata del handler di default */
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        perror_and_exit("sigprocmask");

    return mask;
}

void init_base_device(char *args[], size_t n_args){

    /*
     * esempio ./bulb <id>
     * se l'id non è presente significa che il dispositivo non viene controllato
     * (usato per debuging) quindi il device accetta comandi solo da stdin
     */

    device_id id;
    g_running = true;
    g_curr_out_stream = stdout;
    setlinebuf(stdout);

    if(n_args >= 2 && string_to_int(args[1], &id) == 0 && id >= 0){

        char fifo_path[100];

        sprintf(fifo_path, "/tmp/centralina/devices/%d", id);
        int fifo_in_fd = open_fifo(fifo_path, O_RDONLY | O_CLOEXEC);
        g_fifo_in_stream = fdopen(fifo_in_fd, "r");
        if(g_fifo_in_stream == NULL)
            perror_and_exit("init_base_device: fdopen g_fifo_in_stream");
        g_device.id = id;

        int fifo_out_fd = open_fifo(FIFO_DEVICES_RESPONSE, O_WRONLY | O_CLOEXEC);
        g_fifo_out_stream = fdopen(fifo_out_fd, "w");
        if(g_fifo_out_stream == NULL)
            perror_and_exit("init_base_device: fdopen g_fifo_out_stream");
        setlinebuf(g_fifo_out_stream);
    }
    else{
        g_fifo_out_stream = NULL;
        g_fifo_in_stream = NULL;
        g_device.id = -1;
    }

    print_error("Debug mode, id: %d\n", g_device.id);
}

void getpid_command(const char** args, const size_t n_args){

    print_error("Device %d: received getpid command\n", g_device.id);

    send_response("%d", getpid());
}
void gettype_command(const char** args, const size_t n_args){

    print_error("Device %d: received gettype command\n", g_device.id);

    send_response("%s", device_type_to_string(g_device.type));
}
void iscontrolled_command(const char** args, const size_t n_args){

    print_error("Device %d: received iscontrolled command\n", g_device.id);
    
    if(is_controlled())
        send_response("yes");
    else
        send_response("no");
}

void del_command(const char** args, const size_t n_args){

    print_error("Device %d: received del command\n", g_device.id);
    g_running = false;
}

void setconf_command(const char** args, const size_t n_args){

    print_error("Device %d: received setconf command\n", g_device.id);

    char *records[1];
    int value;

    int num = divide_string((char*)args[0], records, 1, "|");
    if(num == 1){
        string_to_int(args[0], &value);
        g_device.state = value;
        int n_records = set_records_from_string(records[0]);
        send_response(OK_DONE);
    }
    else
        send_response(INV_ARGS);
}

_Bool is_controlled(){

    return g_fifo_in_stream &&
                fileno(g_fifo_in_stream) != STDIN_FILENO;
}

void device_loop(const SignalBind signal_bindings[], const size_t n_sb,
        const CommandBind extra_commands[], const size_t n_dc){

    LineBuffer line_buffer = {NULL, 0};
    Command input_command;
    RTSignal input_signal;

    fd_set rfds;
    sigset_t emptyset;
    sigemptyset(&emptyset);
    FILE *curr_in;
    int stdin_fd = fileno(stdin);

    int fifo_fd = -1;
    if(g_fifo_in_stream)
        fifo_fd = fileno(g_fifo_in_stream);

    int max_fd = MAX(STDIN_FILENO, fifo_fd);

    while(g_running){
        FD_ZERO(&rfds);

        FD_SET(stdin_fd, &rfds);
        if(is_controlled())
            FD_SET(fifo_fd, &rfds);

        if(pselect(max_fd+1, &rfds, NULL, NULL, NULL, &emptyset) == -1){

            if(errno != EINTR)
                perror("pselect");
            else {
                print_error("select interrupted by a signal\n");
            }
        }
        else{

            if (FD_ISSET(stdin_fd, &rfds)) {
                curr_in = stdin;
                g_curr_out_stream = stdout;
            }

            if(FD_ISSET(fifo_fd, &rfds)){
                curr_in = g_fifo_in_stream;
                g_curr_out_stream = g_fifo_out_stream;
            }

            //Legge un comando (una linea)
            if(read_incoming_command(curr_in, &input_command, &line_buffer) == -1)
                break;

            if(handle_device_command(&input_command, extra_commands, n_dc) == -1)
                send_response(INV_COMMAND);
        }
    }
    free(line_buffer.buffer);
}

void clean_base_device(){

    if(g_fifo_in_stream) fclose(g_fifo_in_stream);
    if(g_fifo_out_stream ) fclose(g_fifo_out_stream);

    print_error("Device %d: sto terminando\n", g_device.id);
}

int send_response(char* response, ...){

    int n_write = 0;
    if(g_curr_out_stream != NULL){
        va_list args;
        va_start(args,response);
        n_write = vfprintf(g_curr_out_stream, response, args);
        if(strlen(response) > 0 && response[strlen(response)-1] != '\n')
            n_write = fprintf(g_curr_out_stream,"\n");
        va_end(args);
    }
    return n_write;
}

int get_records_string(char* buffer){

    int i;
    _Bool first = true;
    char tmp[50];
    memset(tmp, 0, sizeof(tmp) / sizeof(char));
    for(i=0; i<g_device.num_records; i++){
        if(g_device.records[i].is_settable){
            if(first){
                sprintf(tmp, "%s=%d", g_device.records[i].label, g_device.records[i].value);
                first = false;
            }
            else
                sprintf(tmp, "&%s=%d", g_device.records[i].label, g_device.records[i].value);
            strcat(buffer, tmp);
        }
    }
    return i;
}

int set_records_from_string(char *records){

    if(strlen(records) == 0)
        return 0;

    char* registry[10];
    int i,j, value, first = false;
    int num_registry = divide_string(records, registry, 10, "&");

    char* value_str[1];
    divide_string(records, value_str, 1, "=");
    string_to_int(value_str[0], &value);

    for(i=0; i<g_device.num_records; i++)
        if(strcmp(g_device.records[i].label, records)==0){
            g_device.records[i].value = value;
            first = true;
            break;
        }

    for (i = 0; i < num_registry; i++) {

        divide_string(registry[i], value_str, 1, "=");
        string_to_int(value_str[0], &value);
        for (j = 0; j < g_device.num_records; j++)
            if (strcmp(g_device.records[j].label, registry[i]) == 0){
                g_device.records[j].value = value;
                break;
            }
    }
    if(first)
        return num_registry+1;
    else
        return num_registry;
}

