#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include "centralina.h"
#include "utils.h"


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

    int whois_request_fd = open_fifo(FIFO_WHOIS_REQUEST, O_RDWR);
    FILE* whois_request_in = fdopen(whois_request_fd, "r");

    while(1){

        printf("#>");
        fflush(stdout);

        FD_ZERO(&rfds);
        FD_SET(whois_request_fd, &rfds);
        FD_SET(stdin_fd, &rfds);

        if(select(whois_request_fd+1, &rfds, NULL, NULL, NULL) == -1)
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
            if (FD_ISSET(whois_request_fd, &rfds)) {

                read_incoming_command(whois_request_in, &input_command);

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

    char path[50];

    sprintf(path, "/usr/bin/xterm ./%s", device_type_to_string(device));

    printf("Adding %s...\n", device_type_to_string(device));
    printf("PID padre: %d\n", getpid());

    int fd_parent_to_child[2];
    int fd_child_to_parent[2];
    char *line_buffer = NULL;
    size_t len = 0;
    char buffer[20] = {0};

    if(pipe(fd_parent_to_child) != 0){
        perror("pipe");
        return -1;
    }
    if(pipe(fd_child_to_parent) != 0){
        perror("pipe");
        return -1;
    }
    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        return -1;
    }
    else if(pid == 0){
        dup2(fd_parent_to_child[0], STDIN_FILENO);
        dup2(fd_child_to_parent[1], STDOUT_FILENO);

        close(fd_parent_to_child[0]);
        close(fd_parent_to_child[1]);
        close(fd_child_to_parent[0]);
        close(fd_child_to_parent[1]);

        execl(path, NULL);
        perror_and_exit("[-] Error execl");
    }
    else{
        close(fd_parent_to_child[0]);
        close(fd_child_to_parent[1]);
        close(fd_parent_to_child[1]);

        FILE* file = fdopen(fd_child_to_parent[0],"r");
        if(file == NULL){
            perror("[-] Error fdopen");
        }

        printf("Ciao\n");

        ssize_t byteread = read_line(file, &line_buffer, &len);
        printf("Figlio: %s, num byte %d\n", line_buffer, (int) byteread);
        free(line_buffer);
        close(fd_child_to_parent[0]);
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
}

void init_centralina(){

    //fork();

    pid_t pid = fork();
    if(pid == -1){
        perror_and_exit("[-] error, init_centralina: fork");
    }
    else if(pid == 0){

        //Codice device centralina
    }
    else{

        //devicesIn[0] = m;
    }
}