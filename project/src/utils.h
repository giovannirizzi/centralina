#ifndef UTILS_H
#define UTILS_H


#define MAX_CHILDREN 50

#include <sys/types.h>
#include <errno.h>
#include "devices.h"

#define perror_and_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define print_error(args...) \
            do { fprintf(stderr, ##args); } while (0)



typedef struct{
    SignalType type;
    singal_func_ptr exec_command;
} SignalBind;

/*
 * TODO
 */
typedef struct{
    SignalType signal_type;
    int signal_val;
} Signal;

typedef struct{
    FILE* in;
    FILE* out;
} ChildDevice;

typedef struct{
    ChildDevice children[MAX_CHILDREN];
    int size;
} ChildrenDevices;

void send_command(FILE* out, char* format, ...);

int add_child(ChildrenDevices* c, ChildDevice d);

int delete_child(ChildrenDevices* c, int i);

/*
 * TODO
 */
void read_incoming_signal(int sfd, Signal *signal_res);


/**
 * TODO
 * @param command
 */
void read_incoming_command(FILE *stream, Command *command);

/**
 * Funziona esattamente come la getline però toglie il carattere \n se presente
 */
ssize_t read_line(FILE* stream, char** buffer, size_t *n);

/**
 *
 * @param command
 * @param command_bindings
 * @param n
 * @return
 */
int handle_command(const Command *c, const CommandBind c_bindings[], const size_t n);

/**
 *
 * @param singal
 * @param s_bindings
 * @param n
 * @return
 */
int handle_signal(const Signal *s, const SignalBind s_bindings[], const size_t n);

/*
 * Mappa i nomi dei device al loro device type
 * @return INVALID_TYPE se viene passata una stringa che non
 * corrisponde ha nessun device
*/
DeviceType device_string_to_type(const char* device_string);

/*
 * Mappa i device_type a delle stringhe
*/
const char* device_type_to_string(DeviceType device_type);

/*
 * Converte una stringa in un intero
 * @param *id indirizzo di memoria di un intero dove verrà salvato il risultato
 * @return 0 se la conversione è andata a buon fine altrimenti 1
*/
int string_to_int(const char* string, int *id);
/*
 * Converte una string in uno stato, ovvero controlla che venga
 * passato on / off e @return 0 / 1
 * @return -1 se ho un parametro non valido 
*/
int string_to_state(const char* device_state);

/**else if(strcmp(command, "info") == 0){
 * Divde una stringa in sottostringhe delimitate dal carattere delimitatore passato come parametro
 * La funzione agisce sostituendo ogni carattere delimitatore nella stringa con il carattere '\0'.
 * Salva i riferimenti delle sottostringhe a partire dalla seconda nell'array passato come parametro.
 * @param line la stringa da dividere (viene modificata dopo la chiamata)
 * @param substrings array di puntatori a stringa dove salvare il riferimento alle sottostringhe
 * @param max_substrings il numero masssimo di sottostringhe da salvare nell'array
 * @param delimiter carattere delimitatore
 * @return il numero di sottostringhe salvate <= max_args, uguale al numero di elementi inseriti in args
 */
size_t divide_string(char *line, char **substrings, size_t max_substrings, const char *delimiter);

#endif // UTILS_H