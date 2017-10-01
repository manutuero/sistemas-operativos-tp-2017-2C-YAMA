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
		bytesRecibidos = recv(unSocket, buffer + total, tamanio, MSG_WAITALL);
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

void enviarArchivo(int fd, char* buffer, char* archivo) {
	int file = open(archivo, O_RDWR);
	struct stat mystat;
	if(file==-1){
			 perror("open");
			 exit(1);
	}
	if(fstat(file,&mystat) < 0){
			 perror("fstat");
			 close(file);
			 exit(1);
		}
	int tam = mystat.st_size;
	buffer = (char*) malloc(tam * sizeof(char) + 1);

	//read(file, buffer, tam);

	char* pmap = (char *) mmap (0, tam, PROT_READ, MAP_SHARED, file, 0);
	int i;
	for ( i = 0; i < tam; i++) {
	        buffer[i] = pmap[i];
	        putchar(buffer[i]);
	    }


	printf("Mandando el archivo... \n");
	buffer[tam]='\0'; //Cierro el buffer
	serializarYEnviarArchivo(fd,tam, buffer);
	printf("Se mando el archivo al worker \n");
	munmap(pmap,tam);
	close(file);
	free(buffer);
}

void* serializarRutaArchivo(t_header* header,t_rutaArchivo* ruta){
		header->id = 5;
		int desplazamiento = 0, tamanioMensaje = 0;

		header->tamanio = ruta->tamanio + sizeof(ruta->tamanio);

		void* buffer = malloc(2*sizeof(int)+ sizeof(ruta->tamanio)+ruta->tamanio);


		memcpy(buffer,&header->id,sizeof(header->id));
		desplazamiento = sizeof(header->id);
		memcpy(buffer+desplazamiento,&header->tamanio,sizeof(header->tamanio));
		desplazamiento += sizeof(header->tamanio);
		memcpy(buffer+desplazamiento,&ruta->tamanio,sizeof(ruta->tamanio));
		desplazamiento += sizeof(ruta->tamanio);
		memcpy(buffer+desplazamiento,ruta->ruta,ruta->tamanio);
		tamanioMensaje = desplazamiento + ruta->tamanio;

		return buffer;
}

void serializarYEnviarArchivo(int fd, int tamanio, char* contenido){
	t_header header;
	int desplazamiento = 0;
	void* archivoAMandar = serializarArchivo(tamanio,contenido,&header);
	int tamanioTotal = sizeof(t_header) + header.tamanio;

	void* buffer = malloc(tamanioTotal);

	memcpy(buffer + desplazamiento,&header.id, sizeof(header.id));
	desplazamiento+=sizeof(header.id);

	memcpy(buffer + desplazamiento,&header.tamanio, sizeof(header.tamanio));
	desplazamiento+=sizeof(header.tamanio);

	memcpy(buffer + desplazamiento,archivoAMandar, header.tamanio);
	enviarPorSocket(fd, buffer, tamanioTotal);
	free(archivoAMandar);
	free(buffer);
}

void *serializarArchivo(int tamanio, char* contenido, t_header* header){
	t_archivo *paqueteArchivo;
	paqueteArchivo = malloc(sizeof(int)+tamanio);

	paqueteArchivo->tamanio = tamanio;
	paqueteArchivo->contenido = contenido;
	header->id = 4; //TODO ver cual va a ser el header

	int desplazamiento = 0;

	int tamanioTotal = sizeof(paqueteArchivo->tamanio)+(paqueteArchivo->tamanio);

	void *buffer = malloc(tamanioTotal);


	header->tamanio=sizeof(paqueteArchivo->tamanio);
	memcpy(buffer + desplazamiento,&tamanio, sizeof(paqueteArchivo->tamanio));
	desplazamiento += sizeof(paqueteArchivo->tamanio);

	header->tamanio+=paqueteArchivo->tamanio;
	memcpy(buffer + desplazamiento,paqueteArchivo->contenido, paqueteArchivo->tamanio);

	//free(paqueteArchivo->contenido);
	free(paqueteArchivo);
	return buffer;
}
