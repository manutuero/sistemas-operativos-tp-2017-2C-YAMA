#ifndef DATANODEAPI_H_
#define DATANODEAPI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/string.h>
#include <sys/stat.h>
#include <sys/mman.h>

/* Macros */
#define UN_MEGABYTE 1048576
#define UN_BLOQUE sizeof(char)*UN_MEGABYTE

/* Variables globales */
char* IP_FILESYSTEM;
int   PUERTO_FILESYSTEM;
char* ID_NODO;
int   PUERTO_WORKER;
char* RUTA_DATABIN;
char* NODOARCHCONFIG;

/* Firmas de funciones */
void cargarArchivoConfiguracion(char*);
void setBloque(int numero, char* datos);
char* getBloque(int numero);

#endif
