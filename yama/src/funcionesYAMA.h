/*
 * funcionesYAMA.h
 *
 *  Created on: 20/9/2017
 *      Author: utnso
 */

#ifndef FUNCIONESYAMA_H_
#define FUNCIONESYAMA_H_

#include "utils.h"



typedef struct rutaArchivo{
	int tamanio;
	char* ruta;
}__attribute__((packed)) t_rutaArchivo;


struct sockaddr_in direccionFS;
struct sockaddr_in direccionYama;
int socketYama;


void cargarArchivoDeConfiguracion();
void conectarseAFS();
void yamaEscuchando();
void recibirRutaDeArchivoAProcesar(int);
t_rutaArchivo* deserializarRutaArchivo(void* buffer);

#endif /* FUNCIONESYAMA_H_ */
