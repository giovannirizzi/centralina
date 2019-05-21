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
                                      {"switch", &switch_shell_command},
                                      {"link", &link_shell_command},
                                      {"info", &info_shell_command}};

CommandBind whois_request_bindings[] = {{"whois", &whois_command}};

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
                print_error("select interrupted by a signal\n");
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
        //freopen("/dev/null", "w", stderr);

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

    print_error("Deleted device id: %d\n", device);

    return 0;
}

int link_device(device_id device1, device_id device2){

    printf("Link %d to %d\n",device1,device2);

    LineBuffer line_buffer = {NULL, 0};
    LineBuffer line_buffer2 = {NULL, 0};

    send_command_to_device(device2, "gettype");
    read_device_response(&line_buffer);
    DeviceType type = device_string_to_type(line_buffer.buffer);
    print_error("gettype result %s\n", line_buffer.buffer);

    if(type >= 0){
        print_error("error, can't link to an iteraction device\n");
        return -1;
    }

    send_command_to_device(device1, "getrealtype");
    read_device_response(&line_buffer);
    print_error("getrealtype: %s\n", line_buffer.buffer);

    char command[40];
    sprintf(command, "canadd %s", line_buffer.buffer);
    print_error("send command: %s\n", command);

    send_command_to_device(device2, line_buffer.buffer);
    read_device_response(&line_buffer);

    if(strcmp(line_buffer.buffer, "yes") != 0 &&
            strcmp(line_buffer.buffer, INV_COMMAND) != 0){

        print_error("error, %s\n", line_buffer.buffer);
        return -1;
    }
    else{

        char *devices[50];
        char command[100];

        send_command_to_device(device1, "getconf");
        read_device_response(&line_buffer);

        send_command_to_device(device1, "del");

        int num_devices = divide_string(line_buffer.buffer, devices, 50, " ");

        sprintf(command, "add %s", line_buffer.buffer);
        print_error("sending: %s", command);
        send_command_to_device(device2, command);
        read_device_response(&line_buffer2);

        print_error("Received: %s", line_buffer2.buffer);




    }


    /*
     * Mando un info <device1>
     * Mando un del <device1>
     * Mando un add <device2> <info>
     * Se device2 è la centralina non mando niente,
     * eseguirò un metodo che aggiunge il dispositivo
     * alla centralina e fa un set
     * Magari attendo una risposta
     */
    if(line_buffer.length > 0) free(line_buffer.buffer);
}

int switch_device(device_id device, const char* label, const char* pos){

    printf("Switch device %d, label: %s, pos: %s\n", device, label, pos);

    /*
     * Mando uno switch <device> <label> <pos>
     * Magari attendo una risposta
     */
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
    id%-9stype%-15scontrolled\
    \n", "", "");
    int i, retval;
    LineBuffer line_buffer = {NULL, 0};
    for(i=0; i<MAX_DEVICES; i++){
        if(g_devices_request_stream[i] != NULL){
            if(send_command_to_device(i, "gettype\n") == 0){
                retval = read_device_response(&line_buffer);
                if(retval>0)
                    printf("    %-10d %-15s", i, line_buffer.buffer);
            }
            if(send_command_to_device(i, "iscontrolled\n") == 0){
                retval = read_device_response(&line_buffer);
                if(retval>0)
                    printf("    %s\n", line_buffer.buffer);
            }
        }
    }
    if(line_buffer.buffer) free(line_buffer.buffer);
}
void switch_shell_command(const char** args, const size_t n_args){

    if(n_args != 3)
        print_error(YLW "usage: switch <id> <label> <pos>\n" RESET);
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            print_error(RED "[-] invalid id %s\n" RESET, args[0]);
        else
            switch_device(id, args[1], args[2]);
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
                //id=1|type=bulb|state=on|time since bulb on=10|temperature of gays=20
                char* pipe_str[10], *substrings[1];
                int i, num_pipe, num_sub;
                num_pipe=divide_string(line_buffer.buffer, pipe_str, 10, "|");
                num_sub=divide_string(line_buffer.buffer, substrings, 1, "=");
                printf("    %-20s: %s\n", line_buffer.buffer, substrings[0]);
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
    - switch: turn on/off the identified device \n\
            usage: <switch> <id> <label> <on/off>\n\
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
        print_error("whois_command: open_fifo: no readers\n");
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

        device_loop(NULL, 0, NULL, 0);

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
    strcpy(delimiter, "");
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
                strcat(delimiter, delim1);
            else
                strcat(delimiter, delim2);
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
                return true;
        }
    }
    return false;
}