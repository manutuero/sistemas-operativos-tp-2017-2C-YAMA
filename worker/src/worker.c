#include "funcionesWorker.h"

int main(void) {

	//pathTemporales = "/home/utnso/temp/";
	crearLogger();
	NODOARCHCONFIG = "nodoConfig.cfg";
	cargarArchivoConfiguracion(NODOARCHCONFIG);

	abrirDatabin();

	//struct sockaddr_in direccionDelServidorKernel;
	direccionWorker.sin_family = AF_INET;
	direccionWorker.sin_port = htons(PUERTO_WORKER);
	direccionWorker.sin_addr.s_addr = INADDR_ANY;
	//memset(&(direccionYama.sin_zero), '\0', 8);  // Se setea el resto del array de addr_in en 0

	int activado = 1;
	int pid;

	socketWorker = socket(AF_INET, SOCK_STREAM, 0);
	// Permite reutilizar el socket sin que se bloquee por 2 minutos
	if (setsockopt(socketWorker, SOL_SOCKET, SO_REUSEADDR, &activado,
			sizeof(activado)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// Se enlaza el socket al puerto
	if (bind(socketWorker, (struct sockaddr *) &direccionWorker,
			sizeof(struct sockaddr)) != 0) {
		perror("No se pudo conectar");
		exit(1);
	}
	// Se pone a escuchar el servidor kernel
	if (listen(socketWorker, 10) == -1) {
		perror("listen");
		exit(1);
	}

	fd_set readfds, auxRead;
	int tamanioDir = sizeof(direccionWorker);
	char* buffer;
	int bytesRecibidos, maxPuerto, i, nuevoSocket;
	FD_ZERO(&readfds);
	FD_ZERO(&auxRead);
	FD_SET(socketWorker, &auxRead);

	maxPuerto = socketWorker;

	printf("escuchando masters\n");
	while (1) {

		readfds = auxRead;
		if (select(maxPuerto + 1, &readfds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}

		for (i = 0; i <= maxPuerto; i++) {
			if (FD_ISSET(i, &readfds)) {
				if (i == socketWorker) {

					if ((nuevoSocket = accept(socketWorker,(void*) &direccionWorker, &tamanioDir)) <= 0)
						perror("accept");
					else {
						//Le envia el archivo apenas se conecta con un puerto
						printf("Entro una conexion por el puerto %d\n",
								nuevoSocket);
						FD_SET(nuevoSocket, &auxRead);

						void* buffer;
						t_infoTransformacion* infoTransformacion;
						t_infoReduccionLocal* infoReduccionLocal;
						t_infoReduccionGlobal* infoReduccionGlobal;
						t_infoGuardadoFinal* infoGuardadoFinal;
						char* nombreArchTempPedido;
						t_header header;
						recibirHeader(nuevoSocket, &header);
						buffer = malloc(header.tamanioPayload);
						recibirPorSocket(nuevoSocket, buffer,header.tamanioPayload);
						pid=fork();
						switch (header.id) {

						case TRANSFORMACION:
							infoTransformacion=deserializarInfoTransformacion(buffer);
							realizarTransformacion(infoTransformacion,nuevoSocket);
							break;
						case REDUCCION_LOCAL:
							infoReduccionLocal=deserializarInfoReduccionLocal(buffer);
							realizarReduccionLocal(infoReduccionLocal,nuevoSocket);
							break;
						case REDUCCION_GLOBAL:
							infoReduccionGlobal=deserializarInfoReduccionGlobal(buffer);
							realizarReduccionGlobal(infoReduccionGlobal,nuevoSocket);
							break;
						case ORDEN_GUARDADO_FINAL:
							infoGuardadoFinal=deserializarInfoGuardadoFinal(buffer);
							guardadoFinalEnFilesystem(infoGuardadoFinal);
							break;
						case SOLICITUD_WORKER:
							nombreArchTempPedido=deserializarSolicitudArchivo(buffer);
							responderSolicitudArchivoWorker(nombreArchTempPedido,nuevoSocket);
							break;

						}
						wait(NULL);
							//t_rutaArchivo* ruta;
							//ruta = malloc(sizeof(t_rutaArchivo));

							//recibirRutaDeArchivoAProcesar(nuevoSocket,&ruta);
							//recive la ruta del archivo
							if (nuevoSocket > maxPuerto)
								maxPuerto = nuevoSocket;
						}
					}
				else {
					buffer = malloc(1000);

					bytesRecibidos = recibirPorSocket(i, buffer, 1000);
					if (bytesRecibidos < 0) {
						perror("Error");
						free(buffer);
						//exit(1);
					}
					if (bytesRecibidos == 0) {
						//printf("Se desconecto del fileSystem el socket %d", i);
						FD_CLR(i, &readfds);
						shutdown(i, 2);
						free(buffer);
					} else {
						buffer[bytesRecibidos] = '\0';
						printf(
								"Socket: %d -- BytesRecibidos: %d -- Buffer recibido : %s\n",
								i, bytesRecibidos, buffer);

					}
				}
			}
		}
	}
}
