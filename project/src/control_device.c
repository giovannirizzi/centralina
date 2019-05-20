#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "control_device.h"
#include "utils.h"

const const CommandBind CONTROL_DEVICE_COMMANDS[] = {"add", &add_command};

int handle_device_command(const Command *c, const CommandBind extra_commands[], const size_t n){

    if(handle_command(c, BASE_COMMANDS,
            sizeof(BASE_COMMANDS)/ sizeof(CommandBind)) == 0)
        return 0;
    if(handle_command(c, CONTROL_DEVICE_COMMANDS, 1) == 0)
        return 0;
    return handle_command(c, extra_commands, n);
}

void getinfo_command(const char **args, size_t n_args){

    print_error("Device %d: received getinfo command\n", g_device.id);

    char info_string[200], tmp[50];
    sprintf(info_string, "%d|%d|%d", g_device.id, g_device.type, g_device.state);
    int i;
    for(i=0; i<g_device.num_records; i++){
        sprintf(tmp, "|%s=%d", g_device.records[i].label, g_device.records[i].value);
        strcat(info_string, tmp);
    }

    send_response("%s", info_string);
}

void getconf_command(const char** args, const size_t n_args){

    /**
     * Deve restituire la sua configurazione e quella di tutti
     * i suoi figli
     */

    print_error("Device %d: received getconf command\n", g_device.id);

    char conf_str[200], records[150];
    memset(records, 0, sizeof(records) / sizeof(char));

    fprintf(g_curr_out_stream, "%d|%d|%d|", g_device.id, g_device.type, g_device.state);

    int num = get_records_string(records);
    if(num > 0)
        fprintf(g_curr_out_stream, "%s", records);

    int i;
    LineBuffer line_buffer = {NULL, 0};
    for(i=0; i<children_devices.size; i++){
        send_command_to_child(i, "getconf");
        read_child_response(i, &line_buffer);
        fprintf(g_curr_out_stream, " %s", line_buffer.buffer);
    }

    if(i== 0) //non ci sono child devices
        fprintf(g_curr_out_stream, " #");

    fprintf(g_curr_out_stream, "\n");
}

void add_command(const char** args, const size_t n_args){

    print_error("Device %d: received add command\n", g_device.id);

    DeviceType type;
    device_id id;
    const char* device_str;

    int num_var = sscanf(args[0], "%d|%d|", &id, &type);
    device_str = device_type_to_string(type);
    if(num_var != 2 || id < 0 || device_string_to_type(device_str) == INVALID_TYPE){
        send_response(INV_ARGS);
        return;
    }

    print_error("add device: id: %d, type: %s\n", id, device_str);

    int pos_child = add_child_device(id, type);

    if(pos_child<0){
        send_response(ERR);
        return;
    }

    //print_error("added device in pos: %d\n", pos_child);

    char* tmp[1];
    int num = divide_string((char*)args[0], tmp, 1, "|");

    if(num != 1){
        send_response(INV_ARGS);
        return;
    }

    char setconf_str[200];
    sprintf(setconf_str, "setconf %s", tmp[0]+2);

    //print_error("Sending command: %s\n", setconf_str);

    /*send_command_to_child(pos_child, setconf_str);

    LineBuffer line_buffer = {NULL, 0};

    read_child_response(pos_child, &line_buffer);
    send_response(line_buffer.buffer);

    free(line_buffer.buffer);*/
    send_response(OK_DONE);
}

void switch_command(const char** args, const size_t n_args){



}

void set_command(const char** args, const size_t n_args){

    send_command_to_child(0, "SWAGG\n");
    LineBuffer buffer = {NULL, 0};
    read_child_response(0, &buffer);
    print_error("Ho ricevuto da child: %s", buffer.buffer);
}

int add_child(ChildrenDevices* c, ChildDevice d){
    if(c->size == MAX_CHILDREN)
        return -1;
    c->children[c->size] = d;
    c->size++;
    return c->size-1;
}

int delete_child(ChildrenDevices* c, int i){
    if(c->size == 0 || i<0 || i>=MAX_CHILDREN)
        return -1;

    for ( ; i < c->size && i<MAX_CHILDREN-1; i++)
        c->children[i] = c->children[i + 1];
    c->size--;
    return 0;
}

void init_control_device(char *args[], size_t n_args){

	init_base_device(args, n_args);

    /*^
     * Ignoro il segnale SIGPIPE generato quando si tenta di scrivere
     * su una read end di una pipe chiusa.
     */
    signal(SIGPIPE, SIG_IGN);
    children_devices.size = 0;
}

int add_child_device(const int id, const DeviceType d_type){

    char path[50];
    sprintf(path, "/usr/bin/xterm ./%s", device_type_to_string(d_type));

    if(children_devices.size >= MAX_CHILDREN)
        return -1;

    print_error("Device %d: adding %s\n", g_device.id, device_type_to_string(d_type));

    int fd_request[2];
    int fd_response[2];

    if(pipe(fd_request) != 0 || pipe(fd_response) != 0){
        perror("add_child_device: pipe");
        return -1;
    }

    char exec_path[PATH_MAX];
    char device_id_str[10];

    sprintf(exec_path, "%s/%s", get_absolute_executable_dir()
            ,device_type_to_string(d_type));

    sprintf(device_id_str, "%d", id);

    char xterm_title[100];
    sprintf(xterm_title, "%s, id:%d", device_type_to_string(d_type), id);

    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        return -1;
    }
    else if(pid == 0){

        close(fd_request[1]);
        dup2(fd_request[0], STDIN_FILENO);
        close(fd_request[0]);

        close(fd_response[0]);
        dup2(fd_response[1], STDOUT_FILENO);
        close(fd_response[1]);


       // char *argv[] = {exec_path, device_id_str, 0};
        char *argv[] = {exec_path, NULL};
        execv(exec_path, argv);

        perror_and_exit("error add_child_device: execl\n");
    }
    else{
        close(fd_request[0]);
        close(fd_response[1]);

        FILE *in = fdopen(fd_request[1], "w");
        FILE *out = fdopen(fd_response[0], "r");

        if(in == NULL || out == NULL){
            perror("add_child_device: fopen");
            return -1;
        }

        setlinebuf(in);
        ChildDevice child_device = { in, out};
        return add_child(&children_devices, child_device);
    }
}

void send_command_to_child(int child, const char* command){

    if(child < children_devices.size){
        int n_write;
        n_write = fprintf(children_devices.children[child].in, "%s", command);
        if(command[strlen(command)-1] != '\n')
            n_write = fprintf(children_devices.children[child].in,"\n");
        if(n_write == -1){
            //La pipe è stata chiusa dal device
            if(errno == EPIPE){
                //delete_device(id, true);
                print_error("send_command_to_child: pipe chiusa\n");
            }
            else
                print_error("error send_command_to_device\n");
        }
    }
    else
        print_error("invalid id send_command_to_child\n");
}

int read_child_response(int child, LineBuffer *buffer){

    if(child >= children_devices.size){
        print_error("invalid id read_child_response\n");
        return -1;
    }

    fd_set rfds;
    struct timeval tv  = {1, 0};
    int child_response_fd = fileno(children_devices.children[child].out);
    FD_ZERO(&rfds);
    FD_SET(child_response_fd, &rfds);

    int retval = select(child_response_fd+1, &rfds, NULL, NULL, &tv);
    if(retval == -1){
        perror("select");
        return -1;
    }
    else if(retval) {
        return read_line(children_devices.children[child].out, buffer);
    }
    else
        return 0;
}

