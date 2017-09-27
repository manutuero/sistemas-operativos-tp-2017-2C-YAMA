#include "funcionesDataNode.h"

void cargarArchivoConfiguracion(char*nombreArchivo) {

	char cwd[1024]; // Variable donde voy a guardar el path absoluto
	char * pathArchConfig = string_from_format("%s/%s",
			getcwd(cwd, sizeof(cwd)), nombreArchivo); // String que va a tener el path absoluto para pasarle al config_create
	t_config *config = config_create(pathArchConfig);

	if (config_has_property(config, "PUERTO_FILESYSTEM")) {
		PUERTO_FILESYSTEM = config_get_int_value(config, "PUERTO_FILESYSTEM");
	}
	if (config_has_property(config, "IP_FILESYSTEM")) {
		IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
	}
	if (config_has_property(config, "ID_NODO")) {
		ID_NODO = config_get_string_value(config, "ID_NODO");
	}
	if (config_has_property(config, "PUERTO_WORKER")) {
		PUERTO_WORKER = config_get_int_value(config, "PUERTO_WORKER");
	}
	if (config_has_property(config, "RUTA_DATABIN")) {
		RUTA_DATABIN = config_get_string_value(config, "RUTA_DATABIN");
	}

	printf("\nIP Filesystem: %s\n", IP_FILESYSTEM);
	/*printf("\nPuerto Filesystem: %d\n", PUERTO_FILESYSTEM);
	printf("\nID Nodo %s\n", ID_NODO);
	printf("\nPuerto Worker %d\n", PUERTO_WORKER);
	printf("\nRuta Data.bin %s\n", RUTA_DATABIN);*/
	//log_info(vg_logger,"Archivo de configuracion cargado exitosamente");
}

int conectarSocket(int sockfd, const char * ipDestino, int puerto) {
	struct sockaddr_in datosServidor;

	datosServidor.sin_family = AF_INET;
	datosServidor.sin_port = htons(puerto);
	datosServidor.sin_addr.s_addr = inet_addr(ipDestino);
	memset(&(datosServidor.sin_zero), '\0', 8);

	int funcionConnect = connect(sockfd, (struct sockaddr *) &datosServidor,
			sizeof(struct sockaddr));
	if (funcionConnect != 0) {
		return 0;
	} else {
		return 1;
	}
}

int enviarPorSocket(int unSocket, const void * mensaje, int tamanio) {
	int bytes_enviados;
	int total = 0;
	tamanio = tamanio + sizeof(uint32_t) * 2;
	while (total < tamanio) {
		bytes_enviados = send(unSocket, mensaje + total, tamanio, 0);

		if (bytes_enviados == FAIL) {
			break;
		}
		total += bytes_enviados;
		tamanio -= bytes_enviados;
	}
	if (bytes_enviados == 0) {
		printf("Bytes enviados igual a cero \n");
	}
	//manejarError("[ERROR] Funcion send");

	return bytes_enviados;
}

int recibirPorSocket(int unSocket, void * buffer, int tamanio) {
	int total = 0;
	int bytesRecibidos;

	while (total < tamanio) {
		bytesRecibidos = recv(unSocket, buffer + total, tamanio, 0);
		if (bytesRecibidos == FAIL) {
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
}	// retorna -1 si fallo, 0 si se desconecto o los bytes recibidos

void cerrarSocket(int unSocket) {
	int funcionClose = shutdown(unSocket, 2);

	if (funcionClose == -1)
		printf("Error al cerrar socket");
}

void* serializarInfoNodo(t_infoNodo* infoNodo, t_header* header) {
	uint32_t bytesACopiar = 0, desplazamiento = 0, largoIp;

	void *payload = malloc(sizeof(uint32_t) * 5); // reservo 4 uint32_t para los primeros 4 campos del struct + 1 para el largo del string ip.
	/* Serializamos el payload */
	bytesACopiar = sizeof(uint32_t);
	memcpy(payload + desplazamiento, &infoNodo->sdNodo, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(payload + desplazamiento, &infoNodo->idNodo, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(payload + desplazamiento, &infoNodo->cantidadBloques, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(payload + desplazamiento, &infoNodo->puerto, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	largoIp = strlen(infoNodo->ip);
	memcpy(payload + desplazamiento, &largoIp, bytesACopiar); // le agrego el largo de la cadena ip como parte del mensaje
	desplazamiento += sizeof(uint32_t);

	payload = realloc(payload, desplazamiento + largoIp); // Hacemos apuntar al nuevo espacio de memoria redimensionado.
	memcpy(payload + desplazamiento, infoNodo->ip, largoIp);
	desplazamiento += largoIp;

	header->tamanio = desplazamiento; // Modificamos por referencia al argumento header.

	/* Serializamos y anteponemos el header */
	void *paquete = malloc(sizeof(uint32_t) * 2 + header->tamanio);
	desplazamiento = 0; // volvemos a empezar..

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->id, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->tamanio, bytesACopiar);
	desplazamiento += bytesACopiar;

	memcpy(paquete + desplazamiento, payload, header->tamanio);
	free(payload);
	return paquete;
}
int conectarAfilesystem(char *IP_FILESYSTEM, int PUERTO_FILESYSTEM) {
	void * paquete;
	int socketPrograma = socket(AF_INET, SOCK_STREAM, 0);
	t_header *header = malloc(sizeof(t_header));
	header->id = 1;
	if (socketPrograma <= 0) {
		printf(
				"\n\n[ERROR] No se ha podido obtener un número de socket. Reintentar iniciar el proceso. \n");
		return (ERROR);
	}
	//puts(IP_FILESYSTEM);
	if (conectarSocket(socketPrograma, IP_FILESYSTEM, PUERTO_FILESYSTEM) == FAIL) {
		printf(
				"\n\nNo se ha podido establecer la conexion con el FILESYSTEM. -> connect \n");
		//cerrarSocket(socketPrograma);
		//return (ERROR);
		//		exit(EXIT_SUCCESS);
		return 0;
	}

	//Armo struct, lo serializo y lo envio x socket. Del lado del fs lo recibe para agregarlo a la lista de nodos.
	t_infoNodo *infoNodo = malloc(sizeof(t_infoNodo));
	infoNodo->sdNodo = 0;
	infoNodo->idNodo = 9;
	infoNodo->cantidadBloques = 300;
	int largoIp = strlen(IP_FILESYSTEM);
	infoNodo->ip = malloc(largoIp + 1);
	strcpy(infoNodo->ip, IP_FILESYSTEM);

	//printf("El ip a enviar es: %s.\n", infoNodo->ip);

	infoNodo->puerto = PUERTO_FILESYSTEM;
	infoNodo->sdNodo = 0;
	paquete = serializarInfoNodo(infoNodo, header);
	if (enviarPorSocket(socketPrograma, paquete, header->tamanio)
			== (header->tamanio + sizeof(uint32_t) * 2)) {
		printf("Informacion del nodo enviada correctamente al FileSystem \n");
	} else {
		printf("Error en la cantidad de bytes enviados al fs");
	}
	return socketPrograma;
}
