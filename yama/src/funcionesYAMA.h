#ifndef FUNCIONESYAMA_H_
#define FUNCIONESYAMA_H_

#include <utils.h>

/* Variables globales */
struct sockaddr_in direccionFS;
struct sockaddr_in direccionYama;
int socketYama;

/* Firmas de funciones*/
void cargarArchivoDeConfiguracion();
void conectarseAFS();
void yamaEscuchando();
void recibirRutaDeArchivoAProcesar(int);
t_rutaArchivo* deserializarRutaArchivo(void* buffer);

#endif
