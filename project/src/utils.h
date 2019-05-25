#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include "common.h"
#include "device.h"

//colori
#define RED   "\x1B[1;31m"
#define GRN   "\x1B[1;32m"
#define YLW   "\x1B[1;33m"
#define RESET "\x1B[0m"

#define MAX(a,b) (((a)>(b))?(a):(b))

#define perror_and_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define print_error(args...) \
            do { fprintf(stderr, ##args); } while (0)

#define print_error_and_exit(args...) \
            do { fprintf(stderr, ##args); exit(EXIT_FAILURE); } while (0)

/**
 * Funzione che apre la fifo
 * @param percorso dove verrà creato la fifo
 * @param modalità di utilizzo
 * @return
 */
int open_fifo(const char* path, mode_t mode);

/**
 * TODO
 * @param command
 */
int read_incoming_command(FILE* in, Command *c, LineBuffer *buffer);

/**
 * Funziona esattamente come la getline però toglie il carattere \n se presente
 */
ssize_t read_line(FILE* stream, LineBuffer *lb);

/**
 * TODO
 * @param command
 * @param command_bindings
 * @param n
 * @return
 */
int handle_command(const Command *c, const CommandBind c_bindings[], size_t n);

/**
 * TODO
 * @param singal
 * @param s_bindings
 * @param n
 * @return
 */
int handle_signal(const RTSignal *s, const SignalBind s_bindings[], size_t n);

/**
 * Mappa i nomi dei device al loro device type
 * @return INVALID_TYPE se viene passata una stringa che non corrisponde ha nessun device
 */
DeviceType device_string_to_type(const char* device_string);

/**
 * Mappa i device_type con i nomi corrispondenti
 */
const char* device_type_to_string(DeviceType device_type);

/**
 * Converte una stringa in un intero
 * @param string la stringa da convertire
 * @param id indirizzo di memoria di un intero dove verrà salvato il risultato
 * @return 0 se la conversione è andata a buon fine altrimenti 1
 */
int string_to_int(const char* string, int *id);


/**
 * mappa la stringa on con 1 e off con 0
 * @return 0 con stringhe "on" e "off", altrimenti -1
 */
int string_to_switch_state(const char *string, int *state);


/**
 * Dato lo stato di un device (0 , 1 , 2 , 3) restituisce on / off / open / close 
 * @param int device_state stato del device
 * @param DeviceType tipo di device
 * @return string relativa allo stato del device corretto
 */
const char* device_state_to_string(int device_state, DeviceType dt);

/**
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

/**
 * TODO
 * @return
 */
char* get_absolute_executable_dir();

int time_to_string(int time, char* string_time);

int seconds_to_string(int seconds, char* string);

int temperature_to_string(int temperature, char* string);

int percentage_to_string(int percentage, char* string);

int string_to_time(const char* string_time, int *time);

int create_timer(timer_t *timer);

int set_timer_tick(timer_t timer, _Bool tick);

int delete_timer(timer_t timer);

void print_tree(char* tree);
_Bool is_last_sibling(char* node[], int n);

#endif // UTILS_H