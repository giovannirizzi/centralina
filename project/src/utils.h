#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include "common.h"
#include "devices.h"


#define perror_and_exit(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define print_error(args...) \
            do { fprintf(stderr, ##args); } while (0)

#define print_error_and_exit(args...) \
            do { fprintf(stderr, ##args); exit(EXIT_FAILURE); } while (0)


/**
 * TODO???
 * @param out
 * @param format
 * @param ...
 */
void send_command(FILE* out, char* format, ...);

/**
 * TODO
 * @param path
 * @param mode
 * @return
 */
int open_fifo(const char* path, mode_t mode);

/**
 * TODO
 * @param sfd
 * @param signal_res
 */
void read_incoming_signal(int sfd, RTSignal *signal_res);

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
 * Mappa la stringa "on" con 1 e "off" con 0, -1 altrimenti
 */
int string_to_state(const char* device_state);

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


#endif // UTILS_H