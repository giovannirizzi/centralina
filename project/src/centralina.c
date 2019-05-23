#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <signal.h>
#include "centralina.h"
#include "utils.h"
#include "control_device.h"

CommandBind shell_command_bindings[] = {{"add", &add_shell_command},
                                      {"list", &list_shell_command},
                                      {"help", &help_shell_command},
                                      {"exit", &exit_shell_command},
                                      {"del", &del_shell_command},
                                      {"set", &set_shell_command},
                                      {"switch", &switch_shell_command},
                                      {"link", &link_shell_command},
                                      {"unlink", &unlink_shell_command},
                                      {"info", &info_shell_command}};

CommandBind whois_request_bindings[] = {{"whois", &whois_command}};

CommandBind controller_command_bindings[] = {{"switchdevice", &switchdevice_command},
                                             {"setdevice", &setdevice_command}};

int main(int argc, char *argv[]){

    LineBuffer line_buffer = {NULL, 0};
    Command input_command;

    init_centralina();

    fd_set rfds;
    sigset_t emptyset;
    sigemptyset(&emptyset);

    int stdin_fd = fileno(stdin);
    int whois_request_fd = fileno(g_whois_request_stream);

    printf("#>  ");
    fflush(stdout);

    while(g_running){

        FD_ZERO(&rfds);
        FD_SET(whois_request_fd, &rfds);
        FD_SET(stdin_fd, &rfds);

        if(pselect(whois_request_fd+1, &rfds, NULL, NULL, NULL, &emptyset) == -1){

            if(errno != EINTR)
                perror("pselect");
            else {
            }
        }
        else {

            //STDIN
            if (FD_ISSET(stdin_fd, &rfds)) {

                //Legge un comando (una linea)
                read_incoming_command(stdin, &input_command, &line_buffer);

                if (input_command.name[0] != '\0') {
                    if (handle_command(&input_command, shell_command_bindings, 8) == -1)
                        fprintf(stdout, "unknown command %s\n",
                                input_command.name);
                }

                if (g_running) {
                    printf("#>  ");
                    fflush(stdout);
                }
            }

            //WHOIS REQUEST
            if (FD_ISSET(whois_request_fd, &rfds)) {

                read_incoming_command(g_whois_request_stream, &input_command, &line_buffer);

                handle_command(&input_command, whois_request_bindings,
                               sizeof(whois_request_bindings) / sizeof(CommandBind));
            }
        }
    }

    clean_centralina();

    free(line_buffer.buffer);
    exit(EXIT_SUCCESS);
}

int add_device(DeviceType device){
    /*
     * Fork ed exec in base al device
     * Creazione e settaggio delle pipe per comunicare
     * Magari attendo una risposta
     */

    device_id id = -1;
    int i;

    for(i=1; i<MAX_DEVICES; i++){

        if(g_devices_request_stream[i] == NULL){
            id = i;
            break;
        }
    }
    if(id == -1)
        return -2;

    char exec_path[PATH_MAX];
    char device_id_str[10];

    sprintf(exec_path, "%s/%s", get_absolute_executable_dir()
            ,device_type_to_string(device));

    sprintf(device_id_str, "%d", id);

    pid_t pid = fork();
    if(pid == -1){
        perror("error add_device: fork");
        g_devices_request_stream[id] = NULL;
    }
    else if(pid == 0) {

//#define XTERM

#ifdef XTERM

        char xterm_title[100];
        sprintf(xterm_title, "%s, id:%d", device_type_to_string(device), id);

        char *argv[] = {"/usr/bin/xterm",
                        "-T", xterm_title,
                        "-e", exec_path, device_id_str,
                        0};

        freopen("/dev/null", "w", stderr);
        execv("/usr/bin/xterm", argv);
#else

        char *argv[] = {exec_path, device_id_str, 0};
        //Cambio l'stdin
        //fclose(stdin);
        //fclose ("/tmp/centralina/null", "r", stdin);
        fclose(stdin);
        fclose(stdout);
        freopen("/dev/null", "w", stderr);

        execv(exec_path, argv);
#endif

        perror_and_exit("error add_device: execl\n");
    }
    else{
        //Creo la FIFO per inviare comandi al device
        char fifo_path[PATH_MAX];
        sprintf(fifo_path, "/tmp/centralina/devices/%d", id);
        int fd = open_fifo(fifo_path, O_WRONLY | O_CLOEXEC);
        g_devices_request_stream[id] = fdopen(fd, "w");
        if(g_devices_request_stream[id] == NULL){
            perror("add_device: fopen");
            return -1;
        }
        setlinebuf(g_devices_request_stream[id]);
    }

    return id;
}

int delete_device(const device_id device){

    if(g_devices_request_stream[device] == NULL) return -1;

    if(fclose(g_devices_request_stream[device]) != 0){
        perror("delete_device: fclose:");
        return -1;
    }

    g_devices_request_stream[device] = NULL;

    //print_error("Deleted device id: %d\n", device);

    return 0;
}

int link_device(device_id device1, device_id device2){

    LineBuffer line_buffer = {NULL, 0};
    LineBuffer line_buffer2 = {NULL, 0};

    if(send_command_to_device(device2, "gettype")==0){
        read_device_response(&line_buffer);
    } else
        return -1;

    DeviceType type = device_string_to_type(line_buffer.buffer);

    if(type >= 0){
        print_error("error, can't link to an iteraction device\n");
        return -1;
    }

    if(send_command_to_device(device1, "getrealtype") == 0){
        read_device_response(&line_buffer);
    } else
        return -1;

    char command[40];
    sprintf(command, "canadd %s", line_buffer.buffer);

    if(send_command_to_device(device2, command) == 0)
        read_device_response(&line_buffer);
    else
        return -1;

    if(strcmp(line_buffer.buffer, "yes") != 0 &&
            strcmp(line_buffer.buffer, INV_COMMAND) != 0){

        print_error("error, %s\n", line_buffer.buffer);
        return -1;
    }
    else{

        char *devices[100];
        char command[100];
        int i;

        if(send_command_to_device(device1, "getconf")==0)
            read_device_response(&line_buffer);
        else
            return -1;

        send_command_to_device(device1, "del");

        int num_devices = divide_string(line_buffer.buffer, devices+1, 100-1, " ");
        devices[0] = line_buffer.buffer;

        sprintf(command, "add %s", devices[0]);
        if(send_command_to_device(device2, command)==0)
            read_device_response(&line_buffer2);
        else
            return -1;

        int parent_id;
        sscanf(devices[0], "%d|", &parent_id);

        LineBuffer buffer = {NULL, 0};

        add_child_devices_recursive(parent_id, devices + 1, devices + num_devices-1, &buffer);

        if(buffer.length > 0) free(buffer.buffer);
    }

    if(line_buffer.length > 0) free(line_buffer.buffer);
}

void add_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        print_error(YLW "usage: add <device>\n" RESET);
    else{
        DeviceType device = device_string_to_type(args[0]);
        if(device == INVALID_TYPE || device == CENTRALINA)
            print_error(RED "[-] invalid device %s\n" RESET, args[0]);
        else{
            int retval = add_device(device);
            switch(retval){
                case -2:
                    printf(RED "[-] maximum number of devices reached\n" RESET);
                    break;
                case -1:
                    printf(RED "[-] error, device was not added\n" RESET);
                    break;
                default:
                    printf(GRN "[+] %s successfully added with id: %d\n" RESET, args[0], retval);
            }
        }
    }
}
void del_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        print_error(YLW "usage: del <id>\n" RESET);
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            print_error(RED "[-] invalid id %s\n" RESET, args[0]);
        else{
            if(id == 0)
                printf(RED "[-] centralina cant be deleted\n" RESET);
            else if(delete_device(id) == -1)
                print_error(RED "[-] no device found with id %d\n" RESET, id);
            else
                printf(GRN "[+] device %d deleted\n" RESET, id);
        }
    }
}

void link_shell_command(const char** args, const size_t n_args){

    if(n_args != 3 || strcmp(args[1], "to") != 0)
        print_error(YLW "usage: link <id> to <id>\n" RESET);
    else{
        device_id id1, id2;
        if(string_to_int(args[0], &id1) != 0 || id1 == 0 ||
                g_devices_request_stream[id1] == NULL)
            print_error(RED "[-] invalid id %s\n" RESET, args[0]);
        else if(string_to_int(args[2], &id2) != 0 ||
                g_devices_request_stream[id2] == NULL)
            print_error(RED "[-] invalid id %s\n" RESET, args[2]);
        else
            link_device(id1, id2);
    }
}

void unlink_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        print_error(YLW "usage: unlink <id>\n" RESET);
    else{
        device_id id1;
        if(string_to_int(args[0], &id1) != 0 || id1 == 0 ||
                g_devices_request_stream[id1] == NULL)
            print_error(RED "[-] invalid id %s\n" RESET, args[0]);
        else{


        }

    }
}

void list_shell_command(const char** args, const size_t n_args){
    printf("\
    \033[1;37mavailable devices:\033[0m \n\
    interaction devices: \n\
    - bulb:\n\
            switch power : turns on/off the bulb\n\
    - window:\n\
            switch open/close: open/close the window\n\
    - fridge:\n\
            switch power: turns on/off the fridge\n\
            set delay: closes automatically the fridge after the time set\n\
            set percentage: (only manually) add/remove content from the fridge\n\
            set temperature: allows to manage and set the internal temperature\n\
    control devices: \n\
    - hub:\n\
            allows multiple devices of the same type to be connected in parallel\n\
            switch power: turns on/off the hub\n\
    - timer:\n\
            allows to define a schedule to control a connected device\n\
            switch power: turns on/off the timer\n\
            set begin: indicates the activation time of the timer\n\
            set end: indicates when the timer will be deactivated\n\n\
    \033[1;37mactive devices:\033[0m \n\
    \n");
    LineBuffer tree = {NULL, 0};
    int i, retval;
    if(send_command_to_device(0, "gettree\n") == 0){
        retval = read_device_response(&tree);
        if(retval>0)
            print_tree(tree.buffer);
    }
    for(i=1; i<MAX_DEVICES; i++){
        if(g_devices_request_stream[i] != NULL){
            if(send_command_to_device(i, "iscontrolled\n")==0){
                retval = read_device_response(&tree);
                    if(retval>0 && (strcmp(tree.buffer, "no") == 0)){
                        if(send_command_to_device(i, "gettree\n") == 0){
                            retval = read_device_response(&tree);
                            if(retval>0)
                                print_tree(tree.buffer);
                        }
                    }
            }
        }
    }
    if(tree.buffer) free(tree.buffer);
}

void switch_shell_command(const char** args, const size_t n_args){

    if(n_args != 3)
        print_error(YLW "usage: switch <id> <label> <on/off>\n" RESET);
    else{
        device_id id;
        int state;
        if(string_to_int(args[0], &id) != 0)
            print_error(RED "[-] invalid id %s\n" RESET, args[0]);
        else if(string_to_switch_state(args[2], &state) != 0)
            print_error(RED "[-] invalid switch state %s\n" RESET, args[2]);
        else{

            char command[100];
            _Bool success = false;

            /*sprintf(command, "switch %s %s", args[1], args[2]);


            if(id == 0){
                if(send_command_to_device(0, command))
            }
            */
            sprintf(command, "switchdevice %d %s %s", id, args[1], args[2]);
            if(send_command_to_device(0, command)==0){
                LineBuffer buffer = {NULL, 0};
                read_device_response(&buffer);


                printf("Ho ricevuto %s\n", buffer.buffer);
            }
        }
    }
}

void set_shell_command(const char** args, const size_t n_args){

    if(n_args != 3)
        print_error(YLW "usage: set <id> <register> <value>\n" RESET);
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            print_error(RED "[-] invalid id %s\n" RESET, args[0]);
        else{


        }

    }
}

void info_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        print_error(YLW "usage: info <id>\n" RESET);
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            print_error(RED "[-] invalid id %s\n" RESET, args[0]);
        else{

            if(g_devices_request_stream[id] == NULL) {
                print_error(RED "[-] no device found with id %s\n" RESET, args[0]);
                return;
            }
            //PRINT INFO
            LineBuffer line_buffer = {NULL, 0};
            send_command_to_device(id, "getinfo");
            int retval = read_device_response(&line_buffer);
            if(retval>0){
                char* pipe_str[10], *substrings[1];
                int i, num_pipe, num_sub;
                num_pipe=divide_string(line_buffer.buffer, pipe_str, 10, "|");
                num_sub=divide_string(line_buffer.buffer, substrings, 1, "=");
                for(i=0; i<num_pipe; i++){
                    divide_string(pipe_str[i], substrings, 1, "=");
                    printf("    %-20s: %s\n", pipe_str[i], substrings[0]);
                }
            }

            if(line_buffer.buffer) free(line_buffer.buffer);
        }
    }
}

void help_shell_command(const char** args, const size_t n_args){

    printf("\
    \033[1;37mavailable commands:\033[0m \n\
    - list: show the list of available and active devices\n\
            usage: <list>\n\
    - add: add a new device to the system\n\
            usage: <add> <device>\n\
    - del: remove the identified device from the system\n\
            it also remove connected devices, if it's a control device \n\
            usage: <del> <id>\n\
    - link: connect the first device to the second\n\
            usage: <link> <id> <id>\n\
    - unlink: disconnect the device from his controller device\n\
            usage: <unlink> <id>\n\
    - switch: turn on/off the identified device \n\
            usage: <switch> <id> <label> <on/off>\n\
    - set: set the register of the identified device \n\
            usage: <set> <id> <register> <value>\n\
    - info: show details of the identified device \n\
            usage: <info> <id>\n\
    - exit: close the controller\n\
            usage: <exit> <id>\
    \n");
}

void exit_shell_command(const char** args, const size_t n_args){

    g_running = 0;
}

void whois_command(const char** args, const size_t n_args){

    int whois_response_fd = open_fifo(FIFO_WHOIS_RESPONSE, O_NONBLOCK | O_WRONLY);
    if(whois_response_fd == -1){
        print_error("whois_command: open_fifo: int switch_device(device_id device, const char* label, const char* pos);no readers\n");
        return;
    }

    g_whois_response_stream = fdopen(whois_response_fd, "w");
    if(g_whois_response_stream == NULL){
        perror("whois_command: fdopen");
        return;
    }

    device_id id;
    int id_control = string_to_int(args[0], &id);
    if (id_control != 0 || id<0 || id>=MAX_DEVICES
        || g_devices_request_stream[id] == NULL){

        fprintf(g_whois_response_stream, "not found\n");
    }
    else{

        send_command_to_device(id, "getpid\n");

        LineBuffer line_buffer;
        int retval = read_device_response(&line_buffer);

        if(retval <= 0)
            fprintf(g_whois_response_stream, "not found\n");
        else
            fprintf(g_whois_response_stream, "%s\n", line_buffer.buffer);
    }

    fclose(g_whois_response_stream);
}

void init_centralina(){

    /*^
     * Ignoro il segnale SIGPIPE generato quando si tenta di scrivere
     * su una read end di una pipe chiusa.
     */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, sigchild_handler);

    set_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE, SIG_BEGIN, SIG_END,
            SIG_DELAY, SIG_PERC, SIG_TEMP, SIG_TICK);

    g_running = true;

    //Apro la fifo per le richieste di whois
    int whois_request_fd = open_fifo(FIFO_WHOIS_REQUEST, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    g_whois_request_stream = fdopen(whois_request_fd, "r");

    pid_t pid = fork();
    if(pid == -1){
        perror_and_exit(RED "[-] error, init_centralina: fork" RESET);
    }
    else if(pid == 0){

        fclose(stdin);
        fclose(stdout);
        freopen("/dev/null", "w", stderr);

        DeviceData controller = {
                CENTRALINA, //DEVICE TYPE
                0, //ID
                1, //STATE
                NULL,
                0, //NUM RECORDS
                NULL,
                0 //NUM SWITCHES
        };

        g_device = controller;

        char* tmp[2] = {" ", "0"};
        init_control_device(tmp, 2);

        stdin = g_fifo_in_stream;

        device_loop(NULL, 0, controller_command_bindings, 2);

        clean_control_device();

        exit(EXIT_SUCCESS);
    }
    else{

        //Creo la FIFO per inviare comandi manuali alla centralina
        int fd = open_fifo("/tmp/centralina/devices/0", O_WRONLY | O_CLOEXEC);
        g_devices_request_stream[0] = fdopen(fd, "w");
        setlinebuf(g_devices_request_stream[0]);

        //Creo la FIFO per ricevere comandi dai devices
        int fd_response = open_fifo(FIFO_DEVICES_RESPONSE, O_RDONLY | O_CLOEXEC);
        g_devices_response_stream = fdopen(fd_response, "r");
    }
}

int send_command_to_device(device_id id, const char* command){

    if(g_devices_request_stream[id] == NULL) {
        print_error("invalid id send_command_to_device\n");
        return -1;
    }
    int n_write;
    n_write = fprintf(g_devices_request_stream[id], "%s", command);
    if(command[strlen(command)-1] != '\n')
        n_write = fprintf(g_devices_request_stream[id],"\n");
    if(n_write == -1){
        //La pipe è stata chiusa dal device
        if(errno == EPIPE)
            delete_device(id);
        else
            print_error("error send_command_to_device\n");
        return -1;
    }
    return 0;
}

int read_device_response(LineBuffer *buffer){

    fd_set rfds;
    struct timeval tv  = {1, 0};
    int devices_response_fd = fileno(g_devices_response_stream);
    FD_ZERO(&rfds);
    FD_SET(devices_response_fd, &rfds);

    int retval = select(devices_response_fd+1, &rfds, NULL, NULL, &tv);
    if(retval == -1){
        perror("select");
        return -1;
    }
    else if(retval) {
        return read_line(g_devices_response_stream, buffer);
    }
    else
        return 0;
}

void clean_centralina(){

    fclose(g_whois_request_stream);
    fclose(g_devices_response_stream);

    int i;
    for(i=0; i<MAX_DEVICES; i++)
        delete_device(i);
}

void print_tree(char *tree){

    char* nodes[200];
    char delimiter[200];
    strcpy(delimiter, "    ");
    int i=0, id, type;
    int num = divide_string(tree, nodes+1, sizeof(nodes)/ sizeof(char) -1, " ");
    nodes[0] = tree;
    char* delim1 = "|    ";
    char* delim2 = "     ";
    for(i=0; i<=num; i++){

        if(*nodes[i] == '#'){
            int length = strlen(delimiter);
            delimiter[length-strlen(delim1)] = '\0';

        }else{
            sscanf(nodes[i], "%d|%d|", &id, &type);
            printf("%s+-(%d)-%s\n", delimiter, id, device_type_to_string(type));

            if(is_last_sibling(nodes + i, num -i))
                strcat(delimiter, delim2);
            else
                strcat(delimiter, delim1);
        }
    }
}

_Bool is_last_sibling(char* node[], int n){

    int i=0;
    int counter = 0;
    while (i<n && counter >= -1){
        i++;
        if(*node[i] == '#')
            counter--;
        else{
            counter++;
            if(counter == 0)
                return false;
        }
    }
    return true;
}

void add_child_devices_recursive(int parent, char **start, char **end, LineBuffer *buffer){

    char command[100];
    if(start >= end){
        return;
    }

    char** curr = start;
    int parent_id;

    do{
        sprintf(command, "add %s", *curr);
        send_command_to_device(parent, command);
        read_device_response(buffer);
        sscanf(*curr, "%d|", &parent_id);

        char** tmp_start = ++curr;
        int counter = 1;

        do{
            if(*curr[0] == '#')
                counter--;
            else
                counter++;

            if(counter==0)
                break;

            curr++;

        }while(curr < end);

        add_child_devices_recursive(parent_id, tmp_start, curr, buffer);

        if(curr < end)
            curr++;

    }while (curr < end || *curr[0] != '#');
}

void switchdevice_command(const char** args, size_t n_args){

    _Bool found = false;
    int i;
    char command[100];
    LineBuffer buffer = {NULL, 0};

    sprintf(command, "switch %s %s", args[1], args[2]);

    for(i=0; i<children_devices.size; i++){
        if(send_command_to_child(i, "getid") == 0){
            if(read_child_response(i, &buffer) > 0){
                if(strcmp(buffer.buffer, args[0]) == 0){
                    if(send_command_to_child(i, command) == 0){
                        read_child_response(i, &buffer);
                        found = true;
                        break;
                    } else
                        break;
                }
            }
        }
        else
            i--;
    }

    if(!found)
        send_response(INV_ID);
    else
        send_response(buffer.buffer);

    if(buffer.length > 9) free(buffer.buffer);
}

void setdevice_command(const char** args, size_t n_args){

    _Bool found = false;
    int i;
    char command[100];
    LineBuffer buffer = {NULL, 0};

    sprintf(command, "set %s %s", args[1], args[2]);

    for(i=0; i<children_devices.size; i++){
        if(send_command_to_child(i, "getid") == 0){
            if(read_child_response(i, &buffer) > 0){
                if(strcmp(buffer.buffer, args[0]) == 0){
                    if(send_command_to_child(i, command) == 0){
                        read_child_response(i, &buffer);
                        found = true;
                        break;
                    } else
                        break;
                }
            }
        }
        else
            i--;
    }

    if(!found)
        send_response(INV_ID);
    else
        send_response(buffer.buffer);

    if(buffer.length > 9) free(buffer.buffer);
}