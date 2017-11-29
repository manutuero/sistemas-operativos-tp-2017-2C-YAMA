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
	int socketMaster;
	pid_t pid;

	socketMaster = socket(AF_INET, SOCK_STREAM, 0);
	// Permite reutilizar el socket sin que se bloquee por 2 minutos
	if (setsockopt(socketMaster, SOL_SOCKET, SO_REUSEADDR, &activado,
			sizeof(activado)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// Se enlaza el socket al puerto
	if (bind(socketMaster, (struct sockaddr *) &direccionWorker,
			sizeof(struct sockaddr)) != 0) {
		perror("No se pudo conectar");
		exit(1);
	}
	// Se pone a escuchar el servidor kernel
	if (listen(socketMaster, 100) == -1) {
		perror("listen");
		exit(1);
	}

	int tamanioDir = sizeof(direccionWorker);
	//char* buffer;
	int bytesRecibidos, nuevoSocket;


	printf("escuchando masters\n");
	while (1) {

		if ((nuevoSocket = accept(socketMaster,
				(struct soccaddr*) &direccionWorker, &tamanioDir)) <= 0)
			perror("accept");
		else {
			printf("Entro una conexion por el puerto %d\n", nuevoSocket);

			void* buffer;
			t_infoTransformacion* infoTransformacion;
			t_infoReduccionLocal* infoReduccionLocal;
			t_infoReduccionGlobal* infoReduccionGlobal;
			t_infoGuardadoFinal* infoGuardadoFinal;
			char* nombreArchTempPedido;
			t_header header;
			recibirHeader(nuevoSocket, &header);
			buffer = malloc(header.tamanioPayload);
			bytesRecibidos=recibirPorSocket(nuevoSocket, buffer, header.tamanioPayload);
			if(bytesRecibidos<=0){
				printf("Error conexion con master entrante");
				continue;
			}
			pid = fork();
			if (pid == 0) {
				switch (header.id) {

				case TRANSFORMACION:
					infoTransformacion = deserializarInfoTransformacion(buffer);
					realizarTransformacion(infoTransformacion, nuevoSocket);
					break;
				case REDUCCION_LOCAL:
					infoReduccionLocal = deserializarInfoReduccionLocal(buffer);
					realizarReduccionLocal(infoReduccionLocal, nuevoSocket);
					break;
				case REDUCCION_GLOBAL:
					infoReduccionGlobal = deserializarInfoReduccionGlobal(
							buffer);
					realizarReduccionGlobal(infoReduccionGlobal, nuevoSocket);
					break;
				case ORDEN_GUARDADO_FINAL:
					infoGuardadoFinal = deserializarInfoGuardadoFinal(buffer);
					guardadoFinalEnFilesystem(infoGuardadoFinal);
					break;
				case SOLICITUD_WORKER:
					nombreArchTempPedido = deserializarSolicitudArchivo(buffer);
					responderSolicitudArchivoWorker(nombreArchTempPedido,
							nuevoSocket);
					break;
				}
				close(nuevoSocket);
				kill(pid,SIGUSR1);
			}
			//else{
			//waitpid(pid,0,0);
			//}

		}

	}

}
