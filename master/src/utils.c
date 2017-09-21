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

void* deserializarSolicitudEjecutarComando(void *mensaje) {
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

void enviarPaquete(int socket, void* mensaje, header header){
	int desplazamiento = 0;
	int tamanioTotal = sizeof(header) + header.tamanio;
    void *buffer = malloc(tamanioTotal);

	memcpy(buffer + desplazamiento, &header.id, sizeof(header.id));
	desplazamiento += sizeof(header.id);
	memcpy(buffer + desplazamiento, &header.tamanio, sizeof(header.tamanio));
	desplazamiento += sizeof(header.tamanio);
	memcpy(buffer + desplazamiento, mensaje, header.tamanio);

	enviarPorSocket(socket, buffer, tamanioTotal);

	free(buffer);
	free(mensaje);
}

int recibirHeader(int socket, header* header){ // myHeader es parametro de salida por eso usamos puntero

	int bytesRecibidos;

	if ((bytesRecibidos = recv(socket, &((*header).id), sizeof((*header).id), 0)) <= 0) return bytesRecibidos;

	bytesRecibidos = recv(socket, &((*header).tamanio), sizeof((*header).tamanio), 0);
	return bytesRecibidos;
}
int recibirPorSocket(int unSocket, void * buffer, int tamanio) {
	int total = 0;
	int bytesRecibidos;

	while (total < tamanio) {
		bytesRecibidos = recv(unSocket, buffer + total, tamanio, 0);
		if (bytesRecibidos == -1) {
			// Error
			perror("[ERROR] Funcion recv");
			break;
		}
		if (bytesRecibidos == 0) {
			// Desconexion
			break;
		}
		total += bytesRecibidos;
		tamanio -= bytesRecibidos;
	}
	return bytesRecibidos;
}

void * recibirPaquete(int socket, header header){

	void * mensaje = malloc(header.tamanio);

	recibirPorSocket(socket, mensaje, header.tamanio);

	void * buffer = deserializar(mensaje, header);

	free(mensaje);

	return buffer;
}  // Recordar castear

int enviarPorSocket(int socket, const void * mensaje, int tamanio) {
	int bytes_enviados;
	int total = 0;

	while (total < tamanio) {
		bytes_enviados = send(socket, mensaje + total, tamanio, 0);
		if (bytes_enviados == -1) {
			break;
		}
		total += bytes_enviados;
		tamanio -= bytes_enviados;
	}
	if (bytes_enviados == -1) perror("[ERROR] Funcion send");

	return bytes_enviados;
}

