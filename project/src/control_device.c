#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "control_device.h"
#include "utils.h"

//I control devices implementano tutti il comando add per aggiungere un figlio
const CommandBind CONTROL_DEVICE_COMMANDS[] = {{"add", &add_command}};


int handle_device_command(const Command *c, const CommandBind extra_commands[], const size_t n){

    //Un extra command può mascherare un comando base se ha lo stesso nome
    if(handle_command(c, extra_commands, n) == 0)
        return 0;

    if(handle_command(c, CONTROL_DEVICE_COMMANDS, 1) == 0)
        return 0;

    return handle_command(c, BASE_COMMANDS,
            sizeof(BASE_COMMANDS)/ sizeof(CommandBind));
}

void getinfo_command(const char **args, size_t n_args){

    char info_string[200], tmp[200], value_str[50], state_str[50];
    _Bool override = false;
    LineBuffer line_buffer = {NULL, 0};

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

    sprintf(info_string, "id=%d|type=%s|state=%s|", g_device.id,
            device_type_to_string(g_device.type), state_str);

    //Aggiungo i registri del dispositivo
    for(i=0; i<g_device.num_records; i++){
        g_device.records[i].format_value(g_device.records[i].value, value_str);
        if(i==0)
            sprintf(tmp, "%s=%s", g_device.records[i].description, value_str);
        else
            sprintf(tmp, "|%s=%s", g_device.records[i].description, value_str);

        strcat(info_string, tmp);
    }

    send_response("%s", info_string);
    if(line_buffer.length > 0) free(line_buffer.buffer);
}

void getconf_command(const char** args, const size_t n_args){

    char records[150];
    memset(records, 0, sizeof(records) / sizeof(char));

    fprintf(g_curr_out_stream, "%d|%d|%d|", g_device.id, g_device.type, g_device.state);

    //Invio anche i registri settabili del dispositivo
    int num = get_records_string(records);
    if(num > 0)
        fprintf(g_curr_out_stream, "%s", records);

    int i;
    LineBuffer line_buffer = {NULL, 0};

    //Aggiungo anche la configurazione dei figli
    for(i=0; i<g_children_devices.size; i++){
        if(send_command_to_child(i, "getconf") == 0){
            read_child_response(i, &line_buffer);
            fprintf(g_curr_out_stream, " %s", line_buffer.buffer);
        }
        else
            i--;
    }

    if(line_buffer.length > 0) free(line_buffer.buffer);

    fprintf(g_curr_out_stream, " #");

    send_response("\n");
}

void add_command(const char** args, const size_t n_args){

    DeviceType type;
    device_id id;
    const char* device_str;
    LineBuffer line_buffer = {NULL, 0};

    int num_var = sscanf(args[0], "%d|%d|", &id, &type);
    device_str = device_type_to_string(type);
    type = device_string_to_type(device_str);
    if(num_var != 2 || id < 0 || type == INVALID_TYPE || type == CONTROLLER){
        send_response(INV_ARGS);
        return;
    }

    int pos_child = add_child_device(id, type);
    if(pos_child<0){
        send_response(ERR);
        return;
    }

    char* tmp[1];
    int num = divide_string((char*)args[0], tmp, 1, "|");

    if(num != 1){
        send_response(INV_ARGS);
        return;
    }

    //Setto il nuovo figlio con la configurazione passata come parametro
    char setconf_str[200];
    sprintf(setconf_str, "setconf %s", tmp[0]+2);
    send_command_to_child(pos_child, setconf_str);
    read_child_response(pos_child, &line_buffer);

    if(line_buffer.length > 0) free(line_buffer.buffer);

    print_error("RICEVUTO ADD: %d\n", g_device.state);
    send_response(OK_DONE);
}

void switch_command(const char** args, const size_t n_args){

    if(g_children_devices.size == 0){
        send_response(ERR_NO_DEVICES);
        return;
    }

    int i;
    char command[100];
    sprintf(command, "switch %s %s", args[0], args[1]);

    _Bool done = false;
    LineBuffer buffer = {NULL, 0};
    //Invio a tutti i figli il comando
    for(i=0; i<g_children_devices.size; i++){
        if(send_command_to_child(i, command) == 0){
            read_child_response(i, &buffer);
            if(strcmp(buffer.buffer, OK_DONE) == 0)
                done = true;
        }
        else
            i--;
    }

    //Aggiorno lo stato in base a quello dei figli
    LineBuffer buffer2 = {NULL, 0};
    if(send_command_to_child(0, "getstate")==0)
        if(read_child_response(0, &buffer2)>0)
            if(string_to_int(buffer2.buffer, &g_device.state)!=0)
                g_device.state = 0;

    if(strcmp(buffer.buffer, OK_NO_CHANGES) == 0 && done)
        send_response(OK_DONE);
    else
        send_response(buffer.buffer);

    if(buffer.length > 0) free(buffer.buffer);
    if(buffer2.length > 0) free(buffer2.buffer);
}

void set_command(const char** args, const size_t n_args){

    if(n_args != 2){
        send_response(INV_ARGS);
        return;
    }

    int device_value;
    int i;

    /*Cerco se il dispositivo ha il registro passato come parametro
     * se lo trovo cerco di settarne il valore*/
    for(i=0; i<g_device.num_records; i++){
        if(strcmp(g_device.records[i].label, args[0]) == 0){
            if(g_device.records[i].is_settable){
                if(g_device.records[i].convert_value(args[1], &device_value) == 0){
                    g_device.records[i].value = device_value;
                    send_response(OK_DONE);
                    return;
                } else {
                    send_response(INV_SET_VALUE);
                    return;
                }
            } else {
                send_response(ERR_REG_UNSETTABLE);
                return;
            }
        }
    }

    send_response(INV_REG);
}

void init_control_device(char *args[], size_t n_args){

	init_base_device(args, n_args);

    /*^
     * Ignoro il segnale SIGPIPE generato quando si tenta di scrivere
     * su una read end di una pipe chiusa.
     */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, sigchild_handler);
    g_children_devices.size = 0;
}

int add_child_device(const int id, const DeviceType d_type){

    if(g_children_devices.size >= MAX_CHILDREN)
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

    sprintf(device_id_str, "%d", id);;

    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        return -1;
    }
    else if(pid == 0){

        /*Collego la read end di una pipe all' stdin e la write end
         * dell'altra pipe al stdout del figlio
        */
        close(fd_request[1]);
        dup2(fd_request[0], STDIN_FILENO);
        close(fd_request[0]);

        close(fd_response[0]);
        dup2(fd_response[1], STDOUT_FILENO);
        close(fd_response[1]);


        char *argv1[] = {exec_path, device_id_str, 0};
        char *argv2[] = {exec_path, NULL};

        if(g_device.id < 0)
            execv(exec_path, argv2); //per debug senza id
        else
            execv(exec_path, argv1);

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
        return add_child(&g_children_devices, child_device);
    }
}

int send_command_to_child(int child, const char* command){

    if(child < g_children_devices.size){
        int n_write;
        n_write = fprintf(g_children_devices.children[child].in, "%s", command);
        //Se il comando non include uno \n finale lo invia
        if(command[strlen(command)-1] != '\n')
            n_write = fprintf(g_children_devices.children[child].in,"\n");
        if(n_write == -1){
            //La pipe è stata chiusa dal device
            if(errno == EPIPE){
                delete_child_device(child);
                return -1;
            }
            else{
                print_error("error send_command_to_child\n");
                return -2;
            }
        }
        return 0;
    }
    else
        print_error("invalid id send_command_to_child\n");
    return -2;
}

int read_child_response(int child, LineBuffer *buffer){

    if(child >= g_children_devices.size){
        print_error("invalid id read_child_response\n");
        return -1;
    }

    fd_set rfds;
    struct timeval tv  = {1, 0};
    int child_response_fd = fileno(g_children_devices.children[child].out);
    FD_ZERO(&rfds);
    FD_SET(child_response_fd, &rfds);

    int retval = select(child_response_fd+1, &rfds, NULL, NULL, &tv);
    if(retval == -1){
        perror("select");
        return -1;
    }
    else if(retval) {
        return read_line(g_children_devices.children[child].out, buffer);
    }
    else //timeout
        return 0;
}

void getrealtype_command(const char** args, const size_t n_args) {

    int child = 0;
    _Bool done = false;
    LineBuffer line_buffer = {NULL, 0};
    //Ciclo finché ho un figlio che mi ha dato la risposta
    while(child < g_children_devices.size && !done){

        if(send_command_to_child(child, "getrealtype") == 0){

            read_child_response(child, &line_buffer);
            send_response("%s", line_buffer.buffer);
            done = true;
        }
        else
            child++;
    }

    if(!done)
        send_response("%d", INVALID_TYPE);

    if(line_buffer.length > 0) free(line_buffer.buffer);
}

void delete_child_device(int child){

    if(g_children_devices.children[child].out)
        fclose(g_children_devices.children[child].out);

    if(g_children_devices.children[child].in)
        fclose(g_children_devices.children[child].in);

    delete_child(&g_children_devices, child);
}

void clean_control_device(){
    clean_base_device();

    int i;
    for(i=g_children_devices.size-1; i>= 0; i--){
        delete_child_device(i);
    }
}

void sigchild_handler(int signum) {

    while (waitpid(-1, NULL, WNOHANG) > 0);

    LineBuffer buffer = {NULL, 0};

    //Elimino i dispositivi che sono terminati
    int i;
    for(i=0; i<g_children_devices.size; i++){
        //
        if(send_command_to_child(i, "isalive") == 0)
            read_child_response(i, &buffer);
        else
            i--;
    }

    free(buffer.buffer);
}

void gettree_command(const char** args, size_t n_args){

    fprintf(g_curr_out_stream, "%d|%d|", g_device.id, g_device.type);

    int i;
    LineBuffer line_buffer = {NULL, 0};

    //Aggiungo l'albero dei figli
    for(i=0; i<g_children_devices.size; i++){
        if(send_command_to_child(i, "gettree") == 0){
            read_child_response(i, &line_buffer);
            fprintf(g_curr_out_stream, " %s", line_buffer.buffer);
        }
        else
            i--;
    }

    if(line_buffer.length > 0) free(line_buffer.buffer);

    fprintf(g_curr_out_stream, " #");

    send_response("\n");
}

/*
 * Funzioni per gestire l'array di dispositivi figli
 */
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
