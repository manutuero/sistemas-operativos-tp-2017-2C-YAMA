/*
 * funcionesWorker.c

 *
 *  Created on: 13/9/2017
 *      Author: utnso
 */

#include "utils.h"
#include "funcionesWorker.h"


int recibirArchivo(int cliente){
	int file = open("/home/utnso/Escritorio/archivoSalida",O_WRONLY);
	archivo* archivo;
	void *buffer;
	header header;

	int bytesRecibidos = recibirHeader(cliente, &header);
	buffer = malloc(header.tamanio+1);
	bytesRecibidos = recibirPorSocket(cliente,buffer,header.tamanio);
	//Se podria hacer RecibirPaquete, dependiendo si la deserializacion va a utils o no.
	archivo = deserializarArchivo(buffer,header.tamanio);

	printf("Al worker le llego un archivo de %d bytes: \n%s\n",archivo->tamanio, archivo->contenido);
	write(file, archivo->contenido, archivo->tamanio);
	close(file);
	return bytesRecibidos;
}

archivo* deserializarArchivo(void *buffer, int tamanio){
	archivo *miArchivo = malloc(sizeof(archivo));

	int desplazamiento = 0;
	memcpy(&miArchivo->tamanio, buffer, sizeof(miArchivo->tamanio));
	desplazamiento += sizeof(miArchivo->tamanio);

	miArchivo->contenido = malloc(miArchivo->tamanio);
	memcpy(miArchivo->contenido, buffer+desplazamiento, miArchivo->tamanio);

	return miArchivo;

}
