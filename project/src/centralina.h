#ifndef CENTRALINA_H
#define CENTRALINA_H

#include "devices.h"

void list();
int add_device(DeviceType device);
int delete_device(device_id device);
int link_device(device_id device1, device_id device2);
int switch_device(device_id device, const char* label, const char* pos);
int get_info(device_id device);

/*
* Separa una stringa in sottostringhe delimitate da " " sostituendo ogni delimitatore con il carattere '\0'.
* Salva i riferimenti delle sottostringhe a partire dalla seconda nell'array passato come parametro.
* @param la stringa da separare, viene modificata dopo la chiamata 
* @param args[] array di puntatori a stringa dove salvare il riferimento agli argomenti trovati
* @parm max_args il numero masssimo di sottostringhe da salvare nell'array
* @return il numero di sottostringhe salvate <= max_args, uguale al numero di elementi inseriti in args
*/
size_t divide_line_into_substrings(char* line, char* args[], size_t max_args);

/*
* Interpreta il comando passato come parametro con gli
* @return 1 se command è uguale a exit, -1 se il comando non è stato trovato, 0 altrimenti
*/
int interpret_command(const char* command, char* args[], const size_t num_args);

#endif // CENTRALINA_H