#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <libgen.h>
#include <sys/signalfd.h>
#include <linux/limits.h>
#include "centralina.h"
#include "utils.h"
#include "control_device.h"

Command input_command = {NULL, 0, NULL, 0};

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

    fd_set rfds;
    int stdin_fd = fileno(stdin);

    int whois_fd_request = open_fifo(FIFO_WHOIS_REQUEST, O_RDWR);
    FILE* whois_stream_request = fdopen(whois_fd_request, "r");

    init_centralina(argv[0]);

    while(1){

        printf("#>");
        fflush(stdout);

        FD_ZERO(&rfds);
        FD_SET(whois_fd_request, &rfds);
        FD_SET(stdin_fd, &rfds);

        if(select(whois_fd_request+1, &rfds, NULL, NULL, NULL) == -1)
            perror_and_exit("select");
        else{

            //STDIN
            if (FD_ISSET(stdin_fd, &rfds)) {

                //Legge un comando (una linea)
                read_incoming_command(stdin, &input_command);

                if(handle_command(&input_command, shell_command_bindings, 8) == -1)
                    fprintf(stdout, "Unknown command %s\n",
                            input_command.name);

            }

            //WHOIS REQUEST
            if (FD_ISSET(whois_fd_request, &rfds)) {

                read_incoming_command(whois_stream_request, &input_command);

                handle_command(&input_command, whois_request_bindings,
                        sizeof(whois_request_bindings)/sizeof(CommandBind));

            }
        }
    }

    free(input_command.name);
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

        if(devices_in_stream[i] == NULL){
            id = i;
            break;
        }
    }
    if(id == -1)
        return -1;

    char exec_path[PATH_MAX];
    char device_id_str[10];
    char signal_fd_str[5];

    char xterm_title[100];
    sprintf(xterm_title, "%s, id:%d", device_type_to_string(device), id);

    sprintf(exec_path, "%s/%s", get_absolute_executable_dir()
            ,device_type_to_string(device));

    sprintf(device_id_str, "%d", id);
    sprintf(signal_fd_str, "%d", device_data.signal_fd);

    pid_t pid = fork();
    if(pid == -1){
        perror("[-] Error add_device: fork\n");
        fclose(devices_in_stream[id]);
        devices_in_stream[id] = NULL;
        return -1;
    }
    else if(pid == 0){

        char *argv[] = {"/usr/bin/xterm",
                        "-T", xterm_title,
                        "-e", exec_path, device_id_str, signal_fd_str,
                        0};

        execv("/usr/bin/xterm", argv);
        fprintf(stderr, "execl path: %s\n", exec_path);
        perror_and_exit("[-] Error execl\n");
    }
    else{

        //Creo la FIFO per inviare comandi al device
        char fifo_path[50];
        sprintf(fifo_path, "/tmp/centralina/devices/%d", id);
        int fd = open_fifo(fifo_path, O_WRONLY);
        devices_in_stream[id] = fdopen(fd, "w");
        setlinebuf(devices_in_stream[id]);
    }

}

int delete_device(device_id device){

    printf("Delete %d\n",device);

    /*
     * Se è la centralina non posso eliminarla
     * mando un segnanle del <device> ha tutti i dispositivi
     * collegati
     * Magari attendo una risposta
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

int get_info(device_id device){

    printf("Info %d\n",device);

    /*
    * Mando una info <device>
    * Parsing della risosta e visualizzo a video le info
    */
}

void add_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        print_error("usage: add <device>\n");
    else{
        DeviceType device = device_string_to_type(args[0]);
        if(device != INVALID_TYPE && device != CENTRALINA)
            add_device(device);
        else
            print_error("invalid device %s\n", args[0]);
    }
}
void del_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        print_error("usage: del <id>\n");
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            print_error("invalid id %s\n", args[0]);
        else
            delete_device(id);
    }
}
void link_shell_command(const char** args, const size_t n_args){

    if(n_args != 3 || strcmp(args[1], "to") != 0)
        print_error("usage: link <id> to <id>\n");
    else{
        device_id id1, id2;
        if(string_to_int(args[0], &id1) != 0)
            print_error("invalid id %s\n", args[0]);
        else if(string_to_int(args[2], &id2) != 0)
            print_error("invalid id %s\n", args[2]);
        else
            link_device(id1, id2);
    }
}
void list_shell_command(const char** args, const size_t n_args){

    printf("Lista devices: \n");
}
void switch_shell_command(const char** args, const size_t n_args){

    if(n_args != 3)
        print_error("usage: switch <id> <label> <pos>\n");
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            print_error("invalid id %s\n", args[0]);
        else
            switch_device(id, args[1], args[2]);
    }
}

void info_shell_command(const char** args, const size_t n_args){

    if(n_args != 1)
        print_error("usage: info <id>\n");
    else{
        device_id id;
        if(string_to_int(args[0], &id) != 0)
            print_error("invalid id %s\n", args[0]);
        else
            get_info(id);
    }
}

void help_shell_command(const char** args, const size_t n_args){

    printf("Help...\n");
}

void exit_shell_command(const char** args, const size_t n_args){

    free(input_command.name);
    exit(EXIT_SUCCESS);
}

void whois_command(const char** args, const size_t n_args){

    fprintf(stdout, "whois command reviced with id: %s\n", n_args > 0 ? args[0] : " ");
    device_id id;
    int id_control = string_to_int(args[0], &id);
    if (id_control != 0 && id<0 && id>=MAX_DEVICES && devices_in_stream[id] == NULL)
        print_error("invalid id %s\n", args[0]);
    else{
        fprintf(devices_in_stream[id], "getpid\n");

        read_incoming_command(devices_response_stream, &input_command);
    }

}

void init_centralina(char* arg0){

    //Creo la FIFO per ricevere comandi dai devices
    int fd = open_fifo(FIFO_DEVICES_RESPONSE, O_RDWR);
    devices_response_stream = fdopen(fd, "r");

    //Maschero e creo il signal fd per i real time signal.
    sigset_t mask;
    mask = set_signal_mask(SIG_POWER, SIG_OPEN, SIG_CLOSE, SIG_DELAY,
                           SIG_PERC, SIG_TIME);
    device_data.signal_fd = signalfd(-1, &mask, 0);
    if (device_data.signal_fd == -1)
        perror_and_exit("init_base_device: signalfd");

    pid_t pid = fork();
    if(pid == -1){
        perror_and_exit("[-] error, init_centralina: fork");
    }
    else if(pid == 0){

        char signal_fd_str[5];
        sprintf(signal_fd_str, "%d", device_data.signal_fd);
        char* tmp[3] = {arg0, "0", signal_fd_str};

        init_control_device(tmp, 3);

        //Codice device centralina

        //TODO

        exit(EXIT_SUCCESS);
    }
    else{

        //Creo la FIFO per inviare comandi manuali alla centralina
        int fd = open_fifo("/tmp/centralina/devices/0", O_WRONLY);
        devices_in_stream[0] = fdopen(fd, "w");
        setlinebuf(devices_in_stream[0]);
    }
}