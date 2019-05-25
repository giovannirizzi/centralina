#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include "common.h"
#include "device.h"

//colori per la shell
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
 * Funzione che crea se non esiste e apre la fifo, se si verifica
 * un errore diverso da ENXIO fa terminare il programma.
 * @param path il percorso del file
 * @param mode modalità di apertura della fifo
 * @return il fd della fifo aperta, -1 se open ha fallito con errno ENXIO
 */
int open_fifo(const char* path, mode_t mode);

/**
 * Esegue la read_line e divide la linea e fa il wrapping di essa
 * in un comando (dvidendo la linea nel formato: nomecomando args0 args1 args2)
 */
int read_incoming_command(FILE* in, Command *c, LineBuffer *buffer);

/**
 * //Esegue la getline e toglie il carattere \n se presente
 * @param stream lo stream dove leggere con la getline
 * @param lb buffer dove salvare i byte, se buffer.buffer = NULL,
 * la getline alloca spazio in memoria abbastanza per contenere i dati letti
 * e salva la sua lunghezza in buffer.length
 * @return il numero di byte letti, -1 se legge un EOF
 */
ssize_t read_line(FILE* stream, LineBuffer *lb);

/**
 * Cerca se è presente un binding per il nome del comando passato come parametro,
 * se lo trova esegue l'azione associata passando come parametri gli argomenti del comando
 * @param command il comando da gestire
 * @param command_bindings array di associazioni nome comando -> azione
 * @param n numero di elementi nell'array
 * @return 0 se è stata trovata un associazione, -1 altrimenti
 */
int handle_command(const Command *c, const CommandBind c_bindings[], size_t n);

/**
 * Cerca se è presente un binding per il segnale passasto come parametro,
 * se lo trova esegue la sua azione associata passando come parametro il valore
 * del segnale
 * @param singal il segnale da gestire
 * @param s_bindings array di associazioni segnale -> azione
 * @param n numero di elementi nell'array
 * @return 0 se è stata trovata un associazione, -1 altrimenti
 */
int handle_signal(const RTSignal *s, const SignalBind s_bindings[], size_t n);

/**
 * Mappa i nomi dei device al loro device type
 * @return INVALID_TYPE se viene passata una stringa che non corrisponde ha nessun device
 */
DeviceType device_string_to_type(const char* device_string);

//Mappa i device_type con i nomi corrispondenti
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

//Interpreta in stringa lo stato di un dispositivo
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

//Restituisce l'absolute path dell'eseguibile.
char* get_absolute_executable_dir();

int time_to_string(int time, char* string_time);
int seconds_to_string(int seconds, char* string);

int string_to_temperature(const char* string, int *id);
int string_to_action(const char* string, int *id);
int string_to_time(const char* string_time, int *time);

// Funzioni per la gestione dei timer che ogni secondo mandano il segnale SIG_TICK
int create_timer(timer_t *timer);
int set_timer_tick(timer_t timer, _Bool tick);
int delete_timer(timer_t timer);

/**
 * Stampa l'albero dato in input
 * @param tree stringa di formato id|type id|type| # id|type # #
 */
void print_tree(char* tree);
_Bool is_last_sibling(char* node[], int n);


#endif // UTILS_H