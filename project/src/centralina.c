#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/time.h> 
#include <sys/types.h> 
#include "centralina.h"
#include "devices.h"
#include "utils.h"
#include "fcntl.h"

Command input_command  = {NULL, 0, NULL, 0};

CommandBind shell_command_bindings[] = {{"add", &add_command},
                                      {"list", &list_command},
                                      {"help", &help_command},
                                      {"exit", &exit_command},
                                      {"del", &del_command},
                                      {"switch", &switch_command},
                                      {"link", &link_command},
                                      {"info", &info_command}};


int main(int argc, char *argv[]){
    setlinebuf(stdin);
    int fd_1, fd_2;
    fd_set rfds;
    fd_set wfds;
    fd_set efds;
    char *myfifo = "/tmp/myfifo";
    char str1[80];
    int retval;
    /* Create the FIFO if it does not exist */
    //prima di leggere un comando provo a sentire se ricevo un whois
    //questo va sistemato però per ora non capisco come dovrebbe essere permanentemente in ascolto
    mkfifo(myfifo, 0666);
    fd_1 = open(myfifo, O_RDONLY); // Open FIFO for read only 
    fd_2 = fileno(stdin);

    printf("fd_1: %d, fd_2: %d", fd_1, fd_2);
  
    FD_ZERO(&wfds);
    FD_ZERO(&efds);    

    while(1){

        FD_ZERO(&rfds);
        FD_SET(fd_1, &rfds);
        FD_SET(fd_2, &rfds);

        int maxfd = fd_1 > fd_2 ? fd_1 : fd_2;

        retval = select(maxfd+1, &rfds, &wfds, &efds, NULL);
        if (retval == -1)
            perror("select()");
        else if(retval > 0){
            printf("Retval %d\n", retval);

            if(FD_ISSET(fd_2, &rfds)){
                printf("STDIN\n");
                //Legge un comando (una linea)
                read_incoming_command(stdin, &(input_command.name));
                input_command.n_args = divide_string(input_command.name,
                        input_command.args, MAX_COMMAND_ARGS, " ");

                int code = handle_command(&input_command, shell_command_bindings,
                        sizeof(shell_command_bindings)/sizeof(CommandBind));

                if(code == -1)
                    print_error("Unknown command %s\n", input_command.name);
                else if(code == 1){
                    close(fd_1);
                    exit(EXIT_SUCCESS);
                }
            }
            if(FD_ISSET(fd_1, &rfds)){
                int nread = read(fd_1, str1, 80); // read from FIFO
                printf("Received string: %s, num bytes %d\n", str1, nread);
                
            }
            
        }
        else{
            printf("Time out\n");
        }
    }

    /*printf("#>");
    //Legge un comando (una linea)
    while (read_line(stdin, &input_command.name, &input_command.len_name)) {

        input_command.n_args = divide_string(input_command.name,
                input_command.args, MAX_COMMAND_ARGS, " ");

        int code = handle_command(&input_command, shell_command_bindings,
                sizeof(shell_command_bindings)/sizeof(CommandBind));

        if(code == -1)
            print_error("Unknown command %s\n", input_command.name);
        else if(code == 1)
            break;

        printf("#>");
    }     
    free(input_command.name);
    exit(EXIT_SUCCESS);*/
}

int add_device(DeviceType device){
    /*
     * Fork ed exec in base al device
     * Creazione e settaggio delle pipe per comunicare
     * Magari attendo una risposta
     */

    char path[50];

    sprintf(path, "./%s", device_type_to_string(device));

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

        int byteread = read_line(file, &line_buffer, &len);
        printf("Figlio: %s, num byte %d\n", line_buffer, byteread);
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

void add_command(const char** args, const size_t n_args){

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
void del_command(const char** args, const size_t n_args){

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
void link_command(const char** args, const size_t n_args){

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
void list_command(const char** args, const size_t n_args){

    printf("Lista devices: \n");
}
void switch_command(const char** args, const size_t n_args){

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
void info_command(const char** args, const size_t n_args){

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
void help_command(const char** args, const size_t n_args){

    printf("Help...\n");
}
void exit_command(const char** args, const size_t n_args){

    free(input_command.name);
    exit(EXIT_SUCCESS);
}