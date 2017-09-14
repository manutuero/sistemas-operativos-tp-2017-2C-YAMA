#include "utils.h"

void* deserializar(void* mensaje, header header) {
  void* buffer;

  switch(header.id) {
    case SOLICITUD_EJECUTAR_COMANDO_CONSOLA:
      buffer = deserializarSolicitudEjecutarComando(mensaje);
      break;
    default:
      perror("Error. Se ha recibido un header desconocido.");
      break;
  }

  return buffer;
}

void* deserializarSolicitudEjecutarComando(mensaje) {
  int desplazamiento = 0, bytesACopiar;

  comando* comando = malloc(sizeof(comando));

  bytesACopiar = sizeof(comando->funcion);
  memcpy(&comando->funcion, mensaje + desplazamiento, bytesACopiar);
  desplazamiento += bytesACopiar;

  bytesACopiar = sizeof(comando->opcion);
  memcpy(&comando->opcion, mensaje + desplazamiento, bytesACopiar);
  desplazamiento += bytesACopiar;

  bytesACopiar = sizeof(comando->parametro1);
  memcpy(&comando->parametro1, mensaje + desplazamiento, bytesACopiar);
  desplazamiento += bytesACopiar;

  bytesACopiar = sizeof(comando->parametro2);
  memcpy(&comando->parametro2, mensaje + desplazamiento, bytesACopiar);
  desplazamiento += bytesACopiar;

  bytesACopiar = sizeof(comando->bloque);
  memcpy(&comando->bloque, mensaje + desplazamiento, bytesACopiar);
  desplazamiento += bytesACopiar;

  bytesACopiar = sizeof(comando->idNodo);
  memcpy(&comando->idNodo, mensaje + desplazamiento, bytesACopiar);
  desplazamiento += bytesACopiar;

  return comando;
}



int recibirHeader(int socket, header* myHeader){ // myHeader es parametro de salida por eso usamos puntero

	int bytesRecibidos;
	void * buffer = malloc(sizeof(header));

	if ((bytesRecibidos = recv(socket, &((*myHeader).id), sizeof((*myHeader).id), 0)) <= 0) return bytesRecibidos;

				bytesRecibidos = recv(socket, &((*myHeader).tamanio), sizeof((*myHeader).tamanio), 0);
	return bytesRecibidos;
}

void * recibirPaquete(int socket, header header){

	void * mensaje = malloc(myHeader.tamanio);

	recibirPorSocket(socket, mensaje, header.tamanio);

	void * buffer = deserializar(mensaje, header);

	free(mensaje);

	return buffer;
}  // Recordar castear
