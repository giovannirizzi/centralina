#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <signal.h>
#include "controller.h"
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
                                      {"info", &info_shell_command}};

CommandBind whois_request_bindings[] = {{"whois", &whois_command}};

//Comandi extra del dispositivo controller
CommandBind controller_command_bindings[] = {{"devicecommand", &devicecommand_controller_command},
                                             {"switch", &switch_controller_command},
                                             {"getinfo", &getinfo_controller_command}};
LineBuffer g_buffer = {NULL, 0};

int main(int argc, char *argv[]){

    LineBuffer line_buffer = {NULL, 0};
    Command input_command;

    init_controller();

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

        //Rimango in attesa di input da stdin e dalla FIFO per il whois
        if(pselect(whois_request_fd+1, &rfds, NULL, NULL, NULL, &emptyset) == -1){

            //Se la chiamata è stata iterrotta da un segnale non faccio nulla
            if(errno != EINTR)
                perror("pselect");
        }
        else {

            //STDIN
            if (FD_ISSET(stdin_fd, &rfds)) {

                //Legge un comando (una linea)
                read_incoming_command(stdin, &input_command, &line_buffer);

                if (input_command.name[0] != '\0') {
                    if (handle_command(&input_command, shell_command_bindings, 10) == -1)
                        fprintf(stdout, RED "[-] unknown command %s\n" RESET,
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

    clean_controller();

    if(line_buffer.length > 0) free(line_buffer.buffer);
    if(g_buffer.length > 0) free(g_buffer.buffer);

    exit(EXIT_SUCCESS);
}

void add_child_devices_recursive(int parent, char **start, char **end, LineBuffer *buffer){

    char command[100];
    if(start >= end){
        return;
    }

    char** curr = start;
    int parent_id;

    do{
        /*Costruisco e mando il comando "add" al device con id <parent>
         * *curr contiene l'argomento del comando add (<id>|<type>|...)
         */
        sprintf(command, "add %s", *curr);
        send_command_to_device(parent, command);
        read_device_response(buffer);
        sscanf(*curr, "%d|", &parent_id);

        char** tmp_start = ++curr;
        int counter = 1;

        //Scorro curr finché arrivo alla conf del suo ultimo figlio
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

int add_device(DeviceType device){

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
        fclose(stdin); //dopo la exec stdin diverrà la fifo
        fclose(stdout);
        freopen("/dev/null", "w", stderr); //sostituisco stderr con /dev/null

        char *argv[] = {exec_path, device_id_str, 0};
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

    return 0;
}

int link_device(device_id device1, device_id device2){

    if(send_command_to_device(device2, "gettype")==0){
        read_device_response(&g_buffer);
    } else
        return -1;

    DeviceType type = device_string_to_type(g_buffer.buffer);

    //Controllo voglio linkare ad un interaction device
    if(type >= 0){
        printf(RED "[-] can't link to an iteraction device\n" RESET);
        return -1;
    }

    //Chiedo il realtype del dispositivo, un hub e un timer senza figli ha type invalid
    if(send_command_to_device(device1, "getrealtype") == 0){
        read_device_response(&g_buffer);
    } else
        return -1;

    char command[100];
    sprintf(command, "canadd %s", g_buffer.buffer);

    //Chiedo al device2 se posso aggiungere un dispositivo di un tale tipo
    if(send_command_to_device(device2, command) == 0)
        read_device_response(&g_buffer);
    else
        return -1;

    //Se il device2 mi ha risposto con un errore
    if(strcmp(g_buffer.buffer, "yes") != 0 &&
            strcmp(g_buffer.buffer, INV_COMMAND) != 0){

        printf(RED "[-] %s\n" RESET, g_buffer.buffer);
        return -1;
    }
    else{

        char *devices[100];

        //Chiedo la sua configurazione al device1, può essere un intero albero di dispositivi
        if(send_command_to_device(device1, "getconf")==0)
            read_device_response(&g_buffer);
        else
            return -1;

        send_command_to_device(device1, "del");

        /* Una configurazione hai il formato: id|type|state|registry1=value
         * la getconf su un control device mi restituisce l'albero con il formato
         * <conf> <conf_figlio1> # <conf_figlio2> <conf_nipote1> # # #
         */

        int num_devices = divide_string(g_buffer.buffer, devices+1, 100-1, " ");
        devices[0] = g_buffer.buffer;

        LineBuffer buffer = {NULL, 0};

        //Aggiungo il primo dispositivo al device 2

        sprintf(command, "add %s", devices[0]);
        if(send_command_to_device(device2, command)==0)
            read_device_response(&buffer);
        else
            return -1;

        int parent_id;
        sscanf(devices[0], "%d|", &parent_id);

        //Ricorsivamente aggiungo tutti gli altri per ricostruire l'albero dei processi corretto
        add_child_devices_recursive(parent_id, devices + 1, devices + num_devices-1, &buffer);

        if(buffer.length > 0) free(buffer.buffer);
    }
    return 0;
}

void init_controller(){

    /*^
     * Ignoro il segnale SIGPIPE generato quando si tenta di scrivere
     * su una read end di una pipe chiusa.
     */
    signal(SIGPIPE, SIG_IGN);
    //Registro l'handler per fare la wait sui figli terminati
    signal(SIGCHLD, sigchild_handler);

    set_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE, SIG_BEGIN, SIG_END,
                    SIG_DELAY, SIG_PERC, SIG_TEMP, SIG_TICK);

    g_running = true;

    //Apro la fifo per le richieste di whois
    int whois_request_fd = open_fifo(FIFO_WHOIS_REQUEST, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    g_whois_request_stream = fdopen(whois_request_fd, "r");

    //Creo il processo per il dispositivo controller
    pid_t pid = fork();
    if(pid == -1){
        perror_and_exit("error, init_controller: fork\n");
    }
    else if(pid == 0){

        fclose(stdin);
        fclose(stdout);
        freopen("/dev/null", "w", stderr);

        Switch switches[] = {
                {"power", &switch_state_controller}
        };

        SignalBind signal_bindings[] = {
                {SIG_POWER, &switch_state_controller}
        };

        DeviceData controller = {
                CONTROLLER, //DEVICE TYPE
                0, //ID
                1, //STATE
                NULL,
                0, //NUM RECORDS
                switches,
                1 //NUM SWITCHES
        };

        g_device = controller;

        char* tmp[2] = {" ", "0"};
        init_control_device(tmp, 2);

        stdin = g_fifo_in_stream;

        device_loop(signal_bindings, 1, controller_command_bindings, 3);

        clean_control_device();

        exit(EXIT_SUCCESS);
    }
    else{

        //Creo la FIFO per inviare comandi alla centralina
        int fd = open_fifo("/tmp/centralina/devices/0", O_WRONLY | O_CLOEXEC);
        g_devices_request_stream[0] = fdopen(fd, "w");
        setlinebuf(g_devices_request_stream[0]);

        //Creo la FIFO per ricevere comandi dai devices
        int fd_response = open_fifo(FIFO_DEVICES_RESPONSE, O_RDONLY | O_CLOEXEC);
        g_devices_response_stream = fdopen(fd_response, "r");
    }
}

void clean_controller(){

    fclose(g_whois_request_stream);
    fclose(g_devices_response_stream);

    int i;
    for(i=0; i<MAX_DEVICES; i++)
        delete_device(i);
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

void pretty_print(const char* feedback){
    if(strcmp(feedback, OK_DONE)==0)
        printf(GRN "[+] completed succesfully\n" RESET);
    else if (strcmp(feedback, OK_NO_CHANGES)==0)
        printf(GRN "[+] nothing changed\n" RESET);
    else if (strcmp(feedback, INV_SWITCH)==0)
        printf(RED "[-] switch not found\n" RESET);
    else if (strcmp(feedback, INV_SET_VALUE)==0)
        printf(RED "[-] invalid registry value \n" RESET);
    else if (strcmp(feedback, INV_REG)==0)
        printf(RED "[-] invalid registry name\n" RESET);
    else if (strcmp(feedback, INV_ID)==0)
        printf(RED "[-] the controller does not control the specified device\n" RESET);
    else if (strcmp(feedback, ERR_REG_UNSETTABLE)==0)
        printf(RED "[-]  registry not settable\n" RESET);
    else if (strcmp(feedback, ERR_CONTROLLER_OFF)==0)
        printf(RED "[-]  controller offline\n" RESET);
    else if (strcmp(feedback, ERR_NO_DEVICES) == 0)
        printf(RED "[-]  the device does not control any device\n" RESET);
    else if (strcmp(feedback, ERR)==0)
        printf(RED "[-] unknown error\n" RESET);
    else
        printf(RED "[-] unknown error\n" RESET);

}

void whois_command(const char** args, const size_t n_args){

    int whois_response_fd = open_fifo(FIFO_WHOIS_RESPONSE, O_NONBLOCK | O_WRONLY);
    if(whois_response_fd == -1){
        printf("whois_command: open_fifo: int switch_device(device_id device, const char* label, const char* pos);no readers\n");
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

        int retval = read_device_response(&g_buffer);

        if(retval <= 0)
            fprintf(g_whois_response_stream, "not found\n");
        else
            fprintf(g_whois_response_stream, "%s\n", g_buffer.buffer);
    }

    fclose(g_whois_response_stream);
}

/*
 * Funzioni che implementano i comandi della shell
 */
void add_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        printf(YLW "usage: add <device>\n" RESET);
    else{
        DeviceType device = device_string_to_type(args[0]);
        if(device == INVALID_TYPE || device == CONTROLLER)
            printf(RED "[-] invalid device %s\n" RESET, args[0]);
        else{
            int retval = add_device(device);
            switch(retval){
                case -2:
                    printf(RED "[-] maximum number of devices reached\n" RESET);
                    break;
                case -1:
                    printf(RED "[-] device was not added\n" RESET);
                    break;
                default:
                    printf(GRN "[+] %s successfully added with id: %d\n" RESET, args[0], retval);
            }
        }
    }
}
void del_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        printf(YLW "usage: del <id>\n" RESET);
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            printf(RED "[-] invalid id %s\n" RESET, args[0]);
        else{
            if(id == 0)
                printf(RED "[-] controller cant be deleted\n" RESET);
            else if(delete_device(id) == -1)
                printf(RED "[-] no device found with id %d\n" RESET, id);
            else
                printf(GRN "[+] device %d deleted\n" RESET, id);
        }
    }
}

void link_shell_command(const char** args, const size_t n_args){

    if(n_args != 3 || strcmp(args[1], "to") != 0)
        printf(YLW "usage: link <id> to <id>\n" RESET);
    else{
        device_id id1, id2;
        if(string_to_int(args[0], &id1) != 0 || id1 == 0 ||
                g_devices_request_stream[id1] == NULL)
            printf(RED "[-] invalid id %s\n" RESET, args[0]);
        else if(string_to_int(args[2], &id2) != 0 ||
                g_devices_request_stream[id2] == NULL)
            printf(RED "[-] invalid id %s\n" RESET, args[2]);
        else
            if(link_device(id1, id2)==0)
                printf(GRN "[+] link succesfully\n" RESET);
    }
}

void list_shell_command(const char** args, const size_t n_args){
    printf("\
    \033[1;37mavailable devices:\033[0m \n\
    interaction devices: \n\
    - bulb:\n\
            switch power : turns on/off the bulb\n\
    - window:\n\
            switch open: opens the window\n\
            switch close: closes the window\n\
    - fridge:\n\
            switch open: opens/closes the fridge\n\
            register delay: closes automatically the fridge after the time set\n\
                accepted value: <seconds> e.g. <20>\n\
            register percentage: (only manually) add/remove content from the fridge\n\
                accepted value: <percentage> min=0, max=100 e.g. <80>\n\
            register temperature: allows to set the internal temperature\n\
                accepted value: <grades> min=-20, max=20 e.g. <5>\n\
    control devices: \n\
    - hub:\n\
            allows multiple devices of the same type to be connected in parallel,\n\
            it's also possible to connect an hub to another,\n\
            but the connected hub must have the same type of parent's children\n\
            state and switches of the hub are a mirroring of its children\n\
    - timer:\n\
            allows to define a schedule to control a connected device\n\
            state and switches of the timer are a mirroring of its child\n\
            register action: action sent to the controlled device\n\
                accepted value: <switch name>-<on/off> e.g. power-on\n\
            register begin: indicates the activation time of the timer\n\
                accepted value: <HH:MM> e.g. 06:15 (24H)\n\
            register end: indicates when the timer will be deactivated\n\
                accepted value: <HH:MM> e.g. 23:15 (24H)\n\n\
    \033[1;37mactive devices:\033[0m \n\
    \n");

    int i, retval;
    if(send_command_to_device(0, "gettree\n") == 0){
        retval = read_device_response(&g_buffer);
        if(retval>0)
            print_tree(g_buffer.buffer);
    }
    //Stampo la foresta dei dispositivi attivi
    for(i=1; i<MAX_DEVICES; i++){
        if(g_devices_request_stream[i] != NULL){
            if(send_command_to_device(i, "iscontrolled\n")==0){
                retval = read_device_response(&g_buffer);
                    //Se il dispositivo non è controllato
                    if(retval>0 && (strcmp(g_buffer.buffer, "no") == 0)){
                        if(send_command_to_device(i, "gettree\n") == 0){
                            retval = read_device_response(&g_buffer);
                            if(retval>0)
                                print_tree(g_buffer.buffer);
                        }
                    }
            }
        }
    }
}

void switch_shell_command(const char** args, const size_t n_args){

    if(n_args != 3)
        printf(YLW "usage: switch <id> <switch name> <on/off>\n" RESET);
    else{
        device_id id;
        int state;
        if(string_to_int(args[0], &id) != 0)
            printf(RED "[-] invalid id %s\n" RESET, args[0]);
        else if(string_to_switch_state(args[2], &state) != 0)
            printf(RED "[-] invalid switch state %s\n" RESET, args[2]);
        else{

            char command[100];
            //Se è il controller
            if(id == 0){
                sprintf(command, "switch %s %s", args[1], args[2]);
                if(send_command_to_device(0, command)==0){
                    read_device_response(&g_buffer);
                    pretty_print(g_buffer.buffer);
                }
            }
            else {
                //Mando al controller per fare la switch del dispositivo
                sprintf(command, "devicecommand switch %d %s %s", id, args[1], args[2]);
                if(send_command_to_device(0, command)==0){
                    read_device_response(&g_buffer);
                    pretty_print(g_buffer.buffer);
                }
            }
        }
    }
}

void set_shell_command(const char** args, const size_t n_args){

    if(n_args != 3)
        printf(YLW "usage: set <id> <register> <value>\n" RESET);
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            printf(RED "[-] invalid id %s\n" RESET, args[0]);
        else{

            char command[100];
            //Mando al controller per fare la set del dispositivo
            sprintf(command, "devicecommand set %d %s %s", id, args[1], args[2]);
            if(send_command_to_device(0, command)==0){
                read_device_response(&g_buffer);
                pretty_print(g_buffer.buffer);
            }
        }

    }
}

void info_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        printf(YLW "usage: info <id>\n" RESET);
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            printf(RED "[-] invalid id %s\n" RESET, args[0]);
        else{

            if(g_devices_request_stream[id] == NULL) {
                printf(RED "[-] no device found with id %s\n" RESET, args[0]);
                return;
            }
            //PRINT INFO
            send_command_to_device(id, "getinfo");
            int retval = read_device_response(&g_buffer);
            if(retval>0){
                char* pipe_str[10], *substrings[1];
                int i, num_pipe;
                num_pipe=divide_string(g_buffer.buffer, pipe_str, 10, "|");
                for(i=0; i<num_pipe; i++){
                    divide_string(pipe_str[i], substrings, 1, "=");
                    printf("    %-20s: %s\n", pipe_str[i], substrings[0]);
                }
            }
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
    - switch: turn on/off the related switch of the device \n\
            usage: <switch> <id> <switch name> <on/off>\n\
    - set: set the register of the identified device \n\
            usage: <set> <id> <register> <value>\n\
    - info: show details of the identified device \n\
            usage: <info> <id>\n\
    - exit: close the controller\n\
            usage: <exit>\
    \n");
}

void exit_shell_command(const char** args, const size_t n_args){

    g_running = false;
}

/*
 * Funzioni che implementano i comandi extra e la logica
 * dell'interruttore generale del dispositivo controller
 */

void devicecommand_controller_command(const char **args, size_t n_args){

    //Formato comando: devicecommand <set/switch> <id> <registro/interruttore> <valore>

    //Se sono spento
    if(g_device.state == 0){
        send_response(ERR_CONTROLLER_OFF);
        return;
    }

    _Bool found = false;
    int i;
    char command[100];
    LineBuffer buffer = {NULL, 0};

    sprintf(command, "%s %s %s", args[0], args[2], args[3]);

    //Controllo se ho un dispositivo figlio con tale id
    for(i=0; i<g_children_devices.size; i++){
        if(send_command_to_child(i, "getid") == 0){
            if(read_child_response(i, &buffer) > 0){
                //Se l'ho trovato
                if(strcmp(buffer.buffer, args[1]) == 0){
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

    if(buffer.length > 0) free(buffer.buffer);
}

void getinfo_controller_command(const char** args, size_t n_args) {

    char info_string[200];
    const char* state_str = device_state_to_string(g_device.state, g_device.type);

    sprintf(info_string, "id=%d|type=%s|state=%s|%s=%d", g_device.id,
            device_type_to_string(g_device.type), state_str,
            "connected devices", g_children_devices.size);
    send_response(info_string);
}

void switch_controller_command(const char** args, const size_t n_args){

    int device_state;
    if(string_to_switch_state(args[1], &device_state)==-1){
        send_response(INV_SWITCH_STATE);
        return;
    }

    int i;
    for(i=0; i<g_device.num_switches; i++)
        if(strcmp(g_device.switches[i].label, args[0]) == 0) {
            g_device.switches[i].action(device_state);
            return;
        }
    send_response(INV_SWITCH);
}

void switch_state_controller(int state){

    if(state == g_device.state){
        send_response(OK_NO_CHANGES);
        return;
    }
    g_device.state = state;
    send_response(OK_DONE);
}