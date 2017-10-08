#ifndef FUNCIONESMASTER_H_
#define FUNCIONESMASTER_H_
#include <utils.h>

int chequearParametros(char *transformador, char *reductor, char *archivoAprocesar, char *direccionDeResultado);
int file_exists(char * fileName);
void iniciarMaster(char* transformador, char* reductor, char* archivoAprocesar, char* direccionDeResultado);
int conectarseAYama(int puerto, char* ip);
void mandarRutaArchivoAYama(int socketYama, char* archivoAprocesar);

#endif
