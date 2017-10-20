/*
 * funcionesYAMA.c
 *
 *  Created on: 20/9/2017
 *      Author: utnso
 */

#include "funcionesYAMA.h"
#include "funcionesPlanificacion.h"


uint32_t ultimoMaster = 0;
char* temp = "/tmp/";

void conectarseAFS(){

//	struct sockaddr_in direccionFS;

	direccionFS.sin_family = AF_INET;
	direccionFS.sin_port = htons(6667);
	direccionFS.sin_addr.s_addr = inet_addr("127.0.0.1");
		//memset(&(direccionYama.sin_zero), '\0', 8);


		//int socketFS;

		socketFS = socket(AF_INET, SOCK_STREAM, 0);

		if(connect(socketFS, (struct sockaddr *)&direccionFS, sizeof(struct sockaddr)) != 0){
				perror("fallo la conexion al fileSystem");
				exit(1);
			}

		printf("conectado al filesystem con el socket %d\n",socketFS);

}

void cargarArchivoDeConfiguracion(){
	char* path = "YAMAconfig.cfg";
	char cwd[1024];
	char *pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)),
			path);
	t_config *config = config_create(pathArchConfig);

	if (!config) {
		perror("[ERROR]: No se pudo cargar el archivo de configuracion.");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "YAMA_IP")) {
			ip = config_get_string_value(config, "YAMA_IP");
		}

	if (config_has_property(config, "YAMA_PUERTO")) {
		PUERTO = config_get_int_value(config, "YAMA_PUERTO");
	}

	if (config_has_property(config, "JOB")) {
		job = config_get_int_value(config, "JOB");
	}

	printf("Puerto: %d\n", PUERTO);
	printf("IP YAMA: %s\n", ip);
	printf("Job actual: %d\n", job);
}


void yamaEscuchando(){

	//struct sockaddr_in direccionDelServidorKernel;
	direccionYama.sin_family = AF_INET;
	direccionYama.sin_port = htons(PUERTO);
	direccionYama.sin_addr.s_addr = INADDR_ANY;
	//memset(&(direccionYama.sin_zero), '\0', 8);  // Se setea el resto del array de addr_in en 0

	int activado = 1;

	socketYama = socket(AF_INET, SOCK_STREAM, 0);
	// Permite reutilizar el socket sin que se bloquee por 2 minutos
	if (setsockopt(socketYama, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// Se enlaza el socket al puerto
	if(bind(socketYama, (struct sockaddr *)&direccionYama, sizeof(struct sockaddr))!= 0){
		perror("No se pudo conectar");
		exit(1);
	}
	// Se pone a escuchar el servidor kernel
	if (listen(socketYama, 10) == -1) {
		perror("listen");
		exit(1);
	}
}

void escucharMasters() {

	fd_set readfds, auxRead;
	int tamanioDir = sizeof(direccionYama);
	char* buffer;
	int bytesRecibidos, maxPuerto, i, nuevoSocket;
	FD_ZERO(&readfds);
	FD_ZERO(&auxRead);
	FD_SET(socketYama, &auxRead);

	maxPuerto = socketYama;

	printf("escuchando masters\n");
	while (1) {

		readfds = auxRead;
		if (select(maxPuerto + 1, &readfds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}

		for (i = 0; i <= maxPuerto; i++) {
			if (FD_ISSET(i, &readfds)) {
				if (i == socketYama) {

					if ((nuevoSocket = accept(socketYama,(void*) &direccionYama, &tamanioDir)) <= 0)
						perror("accept");
					else {
						//Le envia el archivo apenas se conecta con un puerto
						printf("Entro una conexion por el puerto %d\n",nuevoSocket);
						FD_SET(nuevoSocket, &auxRead);
						t_rutaArchivo* ruta;
						ruta = malloc(sizeof(t_rutaArchivo));

						recibirRutaDeArchivoAProcesar(nuevoSocket,&ruta);
						//recive la ruta del archivo
						if (nuevoSocket > maxPuerto)
							maxPuerto = nuevoSocket;
					}
				} else {
					buffer = malloc(1000);

					bytesRecibidos = recibirPorSocket(i, buffer, 1000);
					if (bytesRecibidos < 0) {
						perror("Error");
						free(buffer);
						exit(1);
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

void mandarRutaAFS(const t_header* header, void* buffer) {
	//se lo manda a FS
	char* bufferFS;
	int desplazamiento = 0;
	bufferFS = malloc(header->tamanioPayload + 8);
	memcpy(bufferFS, &header->id, sizeof(header->id));
	desplazamiento += sizeof(header->id);
	memcpy(bufferFS + desplazamiento, &header->tamanioPayload,
			sizeof(header->tamanioPayload));
	memcpy(bufferFS + desplazamiento, buffer, header->tamanioPayload);
	enviarPorSocket(socketFS, bufferFS, header->tamanioPayload + desplazamiento);
	free(bufferFS);
}

int recibirRutaDeArchivoAProcesar(int socketMaster, t_rutaArchivo** ruta ){
	t_header header;
	void* buffer;
	if((recibirHeader(socketMaster, &header))<=0){
		perror("Error al recibir header");
		return -1;
	}
	else{
		buffer = malloc(header.tamanioPayload);
		recibirPorSocket(socketMaster, buffer, header.tamanioPayload);

		//se lo manda a FS
		//mandarRutaAFS(&header, buffer);

		*ruta = (t_rutaArchivo*) deserializarRutaArchivo(buffer);
		printf("me llego la ruta %s\n",(*ruta)->ruta);

		t_job* jobMaster = malloc(sizeof(t_job));
		jobMaster->idMaster = ultimoMaster;
		jobMaster->job = job;
		jobMaster->socketMaster = socketMaster;

		pthread_t hiloPeticionMaster;
		printf("job: %d\n", jobMaster->job);
		pthread_create(&hiloPeticionMaster, NULL, (void*) preplanificarJob, jobMaster);

		ultimoMaster++;
		jobMaster++;

		return 0;
	}

}

t_rutaArchivo* deserializarRutaArchivo(void* buffer){
	t_rutaArchivo* rutaArchivo = malloc(sizeof(t_rutaArchivo));
	int desplazamiento = 0;

	memcpy(&rutaArchivo->tamanio, buffer, sizeof(rutaArchivo->tamanio));
	desplazamiento += sizeof(rutaArchivo->tamanio);

	rutaArchivo->ruta = malloc(rutaArchivo->tamanio);
	//rutaArchivo = realloc(rutaArchivo, sizeof(rutaArchivo->tamanio)+rutaArchivo->tamanio);
	memcpy(rutaArchivo->ruta, buffer+desplazamiento, rutaArchivo->tamanio);
	desplazamiento += rutaArchivo->tamanio;

return rutaArchivo;
}

void* obtenerBloquesDelArchivo(t_rutaArchivo* ruta){
		t_header header;

		void* buffer;
		buffer = serializarRutaArchivo(&header,ruta); //esta en utils ya que lo voy a usar para Yama-fs
		int tamanioMensaje = header.tamanioPayload + sizeof(header);
		enviarPorSocket(socketFS,buffer,tamanioMensaje);
		free(buffer);
		/*//Envio a FS y espero que me mande los bloques de ese archivo
		 t_header headerRespuesta;
		 recibirHeader(socketFS,&headerRespuesta);
		 t_bloques* bloques = recibirPaquete(socketFS,headerRespuesta);

		 return bloques;
		 */
		return buffer;
}

/* Crear la tabla de estados global */
void crearTablaDeEstados(){

	listaTablaEstados = list_create();

}



