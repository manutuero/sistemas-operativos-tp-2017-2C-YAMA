/*
 * funcionesWorker.c

 *
 *  Created on: 13/9/2017
 *      Author: utnso
 */

#include "utils.h"
#include "funcionesWorker.h"

int recibirArchivo(int bytesRecibidos, int cliente, int len, FILE* fich) {

	fich = fopen("/home/utnso/Escritorio/archivoSalida", "wb");

	archivo* archivo;

	void *buffer;
		header header;
		bytesRecibidos = recibirHeader(cliente, &header);

			buffer = malloc(header.tamanio+1);

			bytesRecibidos = recibirPorSocket(cliente,buffer,header.tamanio);
			//Se podria hacer RecibirPaquete, dependiendo si la deserializacion va a utils o no.

			archivo = deserializarArchivo(buffer,header.tamanio);

	printf("Al worker le llego como contenido del archivo: \n%s\n",archivo->contenido);

	fwrite(archivo->contenido, 1, archivo->tamanio, fich);

	fclose(fich);
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
