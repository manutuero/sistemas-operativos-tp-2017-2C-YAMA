#ifndef FUNCIONESWORKER_H_
#define FUNCIONESWORKER_H_

#include <utils.h>

/* Variables globales */
t_log* workerLogger;
char* NODOARCHCONFIG;
char* IP_FILESYSTEM;
int PUERTO_FILESYSTEM;
char* ID_NODO;
int PUERTO_WORKER;
char* RUTA_DATABIN;

/* Firmas de funciones */
void crearLogger();
void cargarArchivoConfiguracion(char*);
int recibirArchivo(int cliente);
int recibirYDeserializar(int fd, t_archivo* miArchivo);
t_archivo* deserializarArchivo(void *buffer);

#endif
