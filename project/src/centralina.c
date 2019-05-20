#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/signalfd.h>
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
    g_running = 1;

    init_centralina();

    fd_set rfds;
    int stdin_fd = fileno(stdin);
    int whois_request_fd = fileno(g_whois_request_stream);

    printf("#>  ");
    fflush(stdout);



    while(g_running){

        FD_ZERO(&rfds);
        FD_SET(whois_request_fd, &rfds);
        FD_SET(stdin_fd, &rfds);

        if(/*select(whois_request_fd+1, &rfds, NULL, NULL, NULL)*/
        pselect(whois_request_fd+1, &rfds, NULL, NULL, NULL, NULL)== -1)
            perror_and_exit("select");
        else{

            //STDIN
            if (FD_ISSET(stdin_fd, &rfds)) {

                //Legge un comando (una linea)
                read_incoming_command(stdin, &input_command, &line_buffer);

                if(input_command.name[0] != '\0'){
                    if(handle_command(&input_command, shell_command_bindings, 8) == -1)
                        fprintf(stdout, "unknown command %s\n",
                                input_command.name);
                }

                if(g_running){
                    printf("#>  ");
                    fflush(stdout);
                }
            }

            //WHOIS REQUEST
            if (FD_ISSET(whois_request_fd, &rfds)) {
                
                read_incoming_command(g_whois_request_stream, &input_command, &line_buffer);

                handle_command(&input_command, whois_request_bindings,
                        sizeof(whois_request_bindings)/sizeof(CommandBind));
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
    char signal_fd_str[5];

    sprintf(exec_path, "%s/%s", get_absolute_executable_dir()
            ,device_type_to_string(device));

    sprintf(device_id_str, "%d", id);
    sprintf(signal_fd_str, "%d", g_signal_fd);

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
                        "-e", exec_path, device_id_str, signal_fd_str,
                        0};

        freopen("/dev/null", "w", stderr);
        execv("/usr/bin/xterm", argv);
#else

        char *argv[] = {exec_path, device_id_str,
                        signal_fd_str, 0};
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

int delete_device(device_id device, _Bool non_block_wait){

    if(g_devices_request_stream[device] == NULL) return -1;

    if(fclose(g_devices_request_stream[device]) != 0){
        perror("delete_device: fclose:");
        return -1;
    }

    g_devices_request_stream[device] = NULL;

    int child_exit_code, options = 0;

    if(non_block_wait)
        options = WNOHANG;

    //La wait devo farla solo il device da rimuovere è figlio
    pid_t child_pid = waitpid(-1, &child_exit_code, options);

    if(child_pid == -1){
        perror("delete_device: wait");
        return -1;
    }
    else if(child_pid == 0) {
        print_error("wait pid: nothing changed\n");
        return -1;
    }
    else
        print_error("Deleted PID: %d, exit code: %d\n", child_pid, WEXITSTATUS(child_exit_code));

    return 0;
    /*
     * Devo leggere devices_response_stream per leggere gli
     * id figli che devo rimuovere
     */
}

int link_device(device_id device1, device_id device2){

    printf("Link %d to %d\n",device1,device2);

    /*
     * Mando un info <device1>
     * Mando un del <device1>
     * Mando un add <device2> <info>
     * Se device2 è la centralina non mando niente,
     * eseguirò un metodo che aggiunge il dispositivo
     * alla centralina e fa un set
     * Magari attendo una risposta
     */
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
        print_error("usage: add <device>\n");
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
            else if(delete_device(id, false) == -1)
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
        if(string_to_int(args[0], &id1) != 0)
            print_error(RED "[-] invalid id %s\n" RESET, args[0]);
        else if(string_to_int(args[2], &id2) != 0)
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
    id\ttype\
    \n");
    int i;
    LineBuffer line_buffer = {NULL, 0};
    for(i=0; i<MAX_DEVICES; i++){
        if(g_devices_request_stream[i] != NULL){
            send_command_to_device(i, "gettype\n");
            int retval = read_device_response(&line_buffer);
            if(retval>0)
                printf("    %d\t%s\n", i, line_buffer.buffer);
        }
    }
    for(i=0; i<MAX_DEVICES; i++){
        if(g_devices_request_stream[i] != NULL){
            send_command_to_device(i, "isconttrolled\n");
            int retval = read_device_response(&line_buffer);
            if(retval>0)
                printf("    %d\t%s\n", i, line_buffer.buffer);
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
    //Maschero e creo il signal fd per i real time signal.
    sigset_t mask;

    mask = set_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE, SIG_BEGIN, SIG_END,
            SIG_DELAY, SIG_PERC, SIG_TEMP, SIG_TICK);
    g_signal_fd = signalfd(-1, &mask, 0);
    if (g_signal_fd == -1)
        perror_and_exit("init_base_device: signalfd");

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

        char signal_fd_str[5];
        sprintf(signal_fd_str, "%d", g_signal_fd);

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

        char* tmp[3] = {" ", "0", signal_fd_str};
        init_control_device(tmp, 3);

        stdin = g_fifo_in_stream;

        device_loop(NULL, 0, NULL, 0);

        clean_base_device();

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

void send_command_to_device(device_id id, const char* command){

    if(g_devices_request_stream[id] != NULL){
        int n_write;
        n_write = fprintf(g_devices_request_stream[id], "%s", command);
        if(command[strlen(command)-1] != '\n')
            n_write = fprintf(g_devices_request_stream[id],"\n");
        if(n_write == -1){
            //La pipe è stata chiusa dal device
            if(errno == EPIPE)
                delete_device(id, true);
            else
                print_error("error send_command_to_device\n");
        }
    }
    else
        print_error("invalid id send_command_to_device\n");
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
        delete_device(i, false);
}