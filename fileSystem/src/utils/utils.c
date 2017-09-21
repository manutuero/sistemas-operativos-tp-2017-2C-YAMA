#include "utils.h"

void* deserializar(void* mensaje, t_header header) {
	void* buffer = NULL;

	switch (header.id) {
	default:
		perror("Error. Se ha recibido un header desconocido.");
		break;
	}

	return buffer;
}

int recibirHeader(int socket, t_header *header) {
	int bytesRecibidos;

	if ((bytesRecibidos = recv(socket, &((*header).id), sizeof((*header).id), 0)) <= 0)
		return bytesRecibidos;
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

void* recibirPaquete(int socket, t_header header) {
	void * mensaje = malloc(header.tamanio);
	recibirPorSocket(socket, mensaje, header.tamanio);
	void * buffer = deserializar(mensaje, header);
	free(mensaje);
	return buffer;
}  // Recordar castear

int enviarPorSocket(int socket, const void* mensaje, int tamanio) {
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
	if (bytes_enviados == -1)
		perror("Error. Funcion send");

	return bytes_enviados;
}

int nuevoSocket() {
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
		perror("Error. Funcion socket");
	return sd;
}

int conectarSocket(int sockfd, const char * ipDestino, int puerto) {
	struct sockaddr_in datosServidor;

	datosServidor.sin_family = AF_INET;
	datosServidor.sin_port = htons(puerto);
	datosServidor.sin_addr.s_addr = inet_addr(ipDestino);
	memset(&(datosServidor.sin_zero), '\0', 8);

	int funcionConnect = connect(sockfd, (struct sockaddr *) &datosServidor,
			sizeof(struct sockaddr));
	if (funcionConnect == -1)
		return -1;
	return 0;
}

void enviarPaquete(int socket, void* mensaje, t_header header) {
	int desplazamiento = 0;
	int tamanioTotal = sizeof(header) + header.tamanio;
	void* buffer = malloc(tamanioTotal);
	memcpy(buffer + desplazamiento, &header.id, sizeof(header.id));
	desplazamiento += sizeof(header.id);
	memcpy(buffer + desplazamiento, &header.tamanio, sizeof(header.tamanio));
	desplazamiento += sizeof(header.tamanio);
	memcpy(buffer + desplazamiento, mensaje, header.tamanio);

	enviarPorSocket(socket, buffer, tamanioTotal);

	//free(buffer);
	//free(mensaje);
}

void cerrarSocket(int socket) {
	int funcionClose = shutdown(socket, 2);
	if (funcionClose == -1)
		perror("Error al cerrar el socket");
	//exit(EXIT_SUCCESS);
}

int sonIguales(char* cadena1, char* cadena2) {
	return strcmp(cadena1, cadena2) == 0;
}
