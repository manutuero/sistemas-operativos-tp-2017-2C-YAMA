#include "funcionesFileSystem.h"

/* Inicializacion de variables globales */
int estadoFs = NO_ESTABLE; // Por defecto siempre inicia no estable.
int estadoNodos = -1; //Depende de que estado venga el fs como se va a iniciar
int cantidad_nodos_esperados = 99;
sem_t semNodosRequeridos;
sem_t semEstadoEstable;

/* Inicializacion de estructuras administrativas */
t_directory directorios[100];
t_list *nodos, *nodosEsperados;
t_list *archivos;

/*********************** Implementacion de funciones ************************/
/* Implementacion de funciones para archivo de configuracion */
void cargarArchivoDeConfiguracionFS(char *path) {
	char cwd[1024];
	char *pathArchConfig = string_from_format("%s/%s", getcwd(cwd, sizeof(cwd)),
			path);
	t_config *config = config_create(pathArchConfig);

	if (!config) {
		perror("[ERROR]: No se pudo cargar el archivo de configuracion.");
		exit(EXIT_FAILURE);
	}

	if (config_has_property(config, "PUERTO")) {
		PUERTO = config_get_int_value(config, "PUERTO");
	} else {
		perror("No existe la clave 'PUERTO' en el archivo de configuracion.");
	}

	if (config_has_property(config, "PATH_METADATA")) {
		PATH_METADATA = config_get_string_value(config, "PATH_METADATA");
	} else {
		perror(
				"No existe la clave 'PATH_METADATA' en el archivo de configuracion.");
	}

	if (config_has_property(config, "PUERTO_YAMA")) {
		PUERTO_YAMA = config_get_string_value(config, "PUERTO_YAMA");
	} else {
		perror(
				"No existe la clave 'PUERTO_YAMA' en el archivo de configuracion.");
	}
}

/* Implementacion de funciones para bitmaps */
char* persistirBitmap(uint32_t idNodo, int tamanioDatabin) {
	int i;
	char *path;
	t_bitmap bitmap = malloc(sizeof(char) * tamanioDatabin + 1);
	limpiar(bitmap, tamanioDatabin);

	path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/bitmaps/");
	string_append(&path, string_itoa(idNodo));
	string_append(&path, ".dat");

	FILE *archivo = fopen(path, "w");

	for (i = 0; i < tamanioDatabin; i++) {
		bitmap[i] = 'L';
	}

	bitmap[i] = '\0';
	fputs(bitmap, archivo);

	fclose(archivo);
	return bitmap;
}

int obtenerYReservarBloqueBitmap(t_bitmap bitmap, int tamanioBitmap) {
	int i;
	for (i = 0; i < tamanioBitmap; i++) {
		if (bitmap[i] == 'L') {
			// Cambiar bloque a ocupado
			bitmap[i] = 'O';
			return i;
		}
	}
	// Si no encontro nada devuelve -1 indicando que esta completo el bitmap.
	return -1;
}

/* Implementacion de funciones para mensajes */
void* serializarInfoNodo(t_infoNodo *infoNodo, t_header *header) {
	uint32_t bytesACopiar = 0, desplazamiento = 0, largoIp;

	// reservo 4 uint32_t para los primeros 4 campos del struct + 1 para el largo del string ip.
	void *payload = malloc(sizeof(uint32_t) * 5);

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

	header->tamanioPayload = desplazamiento; // Modificamos por referencia al argumento header.

	/* Serializamos y anteponemos el header */
	void *paquete = malloc(sizeof(uint32_t) * 2 + header->tamanioPayload);
	desplazamiento = 0; // volvemos a empezar..

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->id, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->tamanioPayload, bytesACopiar);
	desplazamiento += bytesACopiar;

	memcpy(paquete + desplazamiento, payload, header->tamanioPayload);

	return paquete;
}

t_infoNodo deserializarInfoNodo(void *mensaje, int tamanioPayload) {
	t_infoNodo infoNodo;
	uint32_t desplazamiento = 0, bytesACopiar = 0, tamanioIp = 0;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoNodo.sdNodo, mensaje + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoNodo.idNodo, mensaje + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoNodo.cantidadBloques, mensaje + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(&infoNodo.puerto, mensaje + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t); // recibimos longitud IP
	memcpy(&tamanioIp, mensaje + desplazamiento, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = tamanioIp;
	infoNodo.ip = malloc(tamanioIp + 1);
	memcpy(infoNodo.ip, mensaje + desplazamiento, bytesACopiar);
	infoNodo.ip[tamanioIp] = '\0';
	desplazamiento += bytesACopiar;
	return infoNodo;
}

void* esperarConexionesDatanodes() {
	int socketServidor = nuevoSocket(), numeroClientes = 10,
			socketsClientes[10] = { }, opt = 1, addrlen, max_sd, i, sd,
			actividad, socketEntrante;
	fd_set readfds;
	t_header header;
	void *buffer = NULL;

	/* Esperar hasta 5 segundos. */
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	// Seteo el socketServidor para permitir multiples conexiones, aunque no sea necesario es una buena practica.
	if (setsockopt(socketServidor, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
			sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	// Defino el tipo de socket con sus parametros.
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PUERTO);

	// Bind al puerto indicado en la variable PUERTO. Cargada de la config.
	if (bind(socketServidor, (struct sockaddr *) &address, sizeof(address))
			< 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	/* Especifica un maximo de BACKLOG conexiones pendientes por parte del socketServidor
	 IMPORTANTE: listen() es una syscall BLOQUEANTE. socketServidor es el que escucha. */
	if (listen(socketServidor, BACKLOG) < 0) {
		perror("Maximo de tres conexiones pendientes");
		exit(EXIT_FAILURE);
	}

	// Acceptar conexiones entrantes
	addrlen = sizeof(address);
	while (1) {
		if (nodos->elements_count < 3) {

			// Limpio la lista de sockets
			FD_ZERO(&readfds);

			//Agrego el socket master a la lista, para que tambien revise si hay cambios
			FD_SET(socketServidor, &readfds);
			max_sd = socketServidor;

			//add child sockets to set
			for (i = 0; i < numeroClientes; i++) {
				// Socket descriptor
				sd = socketsClientes[i];

				// Si es un socket descriptor valido lo agrega a la lista "read" (monitoreados).
				if (sd > 0)
					FD_SET(sd, &readfds);

				// El numero de socket descriptor mas alto, es requerido por la funcion select().
				if (sd > max_sd)
					max_sd = sd;
			}

			// Espero que haya actividad en los sockets. Si el tiempo de espera es NULL, nunca termina.
			actividad = select(max_sd + 1, &readfds, NULL, NULL, &tv);
			if (actividad < 0) {
				fprintf(stderr, "[ERROR]: Fallo la funcion select().");
			}

			// Si algo cambio en el socket master, es una conexion entrante.
			if (FD_ISSET(socketServidor, &readfds)) {
				if ((socketEntrante = accept(socketServidor,
						(struct sockaddr *) &address, (socklen_t*) &addrlen))
						< 0) {
					perror("accept");
					exit(EXIT_FAILURE);
				}
				//Si la cantidad de nodos conectados es menor a la esperada, se agrega el socket, Sino se cierra
				if (nodos->elements_count < cantidad_nodos_esperados) {
					// Agrego el nuevo socket al array
					for (i = 0; i < numeroClientes; i++) {
						//Busco una pos vacia en la lista de clientes para guardar el socket entrante
						if (socketsClientes[i] == 0) {
							socketsClientes[i] = socketEntrante;
							break;
						}
					}
				} else { //Rechazo el socket ya que el sistema no permite mas conexiones.
					cerrarSocket(sd);
				}
			}

			// else  es un cambio en los sockets que estaba escuchando.
			for (i = 0; i < numeroClientes; i++) {
				sd = socketsClientes[i];

				if (FD_ISSET(sd, &readfds)) {
					//Chequea si fue para cerrarse y sino lee el mensaje;
					// desconexion
					if (recibirHeader(sd, &header) == 0) {
						//getpeername(sd, (struct sockaddr*) &address, (socklen_t*) &addrlen);
						//printf("Cliente desconectado sd: %d \n", sd);
						close(sd);
						socketsClientes[i] = 0;
						//SI SE DESCONECTO UN NODO BUSCARLO EN LA LISTA DE NODOS
						//CONECTADOS (LISTA DE STRUCTS) Y SACARLO.
						//ACTUALIZAR TABLA DE ARCHIVOS Y PASAR BLOQUES DE NODO
						//DESCONECTADO A NO DISPONIBLES
						break;
					}
					//Si entra aca recibi un header que va a tener la info de quien se conecto.
					else {
						//printf("Header : %d.\n", header.tamanioPayload);
					}

					buffer = malloc(header.tamanioPayload);

					//Lo que hariamos aca es buscar el nodo en la lista de nodos conectados y sacarlo. Ademas hay que actualizar la tabla de archivos.
					if (recibirPorSocket(sd, buffer, header.tamanioPayload)
							<= 0) {
						perror(
								"Error. El payload no se pudo recibir correctamente.");
					} else {
						t_infoNodo infoNodo = deserializarInfoNodo(buffer,
								header.tamanioPayload);

						t_nodo *nodo = malloc(sizeof(t_nodo));
						nodo->socketDescriptor = sd;
						nodo->idNodo = infoNodo.idNodo;
						nodo->bloquesTotales = infoNodo.cantidadBloques;
						nodo->bloquesLibres = nodo->bloquesTotales;
						nodo->puertoWorker = 0;
						getpeername(sd, (struct sockaddr*) &address,
								(socklen_t*) &addrlen);
						nodo->ip = inet_ntoa(address.sin_addr);

						if (estadoNodos == ACEPTANDO_NODOS_NUEVOS) {
							agregarNodo(nodos, nodo);
						}

						if (estadoNodos == ACEPTANDO_NODOS_YA_CONECTADOS) {
							if (nodos->elements_count
									== nodosEsperados->elements_count)
								cerrarSocket(sd);

							if (esNodoAnterior(nodosEsperados, nodo->idNodo)) {
								agregarNodo(nodos, nodo);
							} else {
								cerrarSocket(sd);
							}
						}

						puts("Nodos restaurados:\n");
						for (i = 0; i < nodos->elements_count; i++) {
							nodo = list_get(nodos, i);
							printf("Id nodo: %d\n Bloques libres: %d\n", nodo->idNodo,
									nodo->bloquesLibres);
						}

					}
					free(buffer);
				}
			}
		}
	}
}

void* serializarSetBloque(void *contenido, uint32_t nroBloqueDatabin) {
	void *paquete;
	int desplazamiento = 0, bytesACopiar = 0;

	// Preparo el header del paquete (id + tamanio payload = 8 bytes).
	t_header *header = malloc(sizeof(t_header));
	header->id = 4; // solicitud escribir bloque
	// header->tamanioPayload = nroBloqueDatabin + bloque->contenido
	header->tamanioPayload = sizeof(uint32_t) + UN_BLOQUE;

	paquete = malloc(sizeof(uint32_t) * 3 + UN_BLOQUE);

	// Serializo.
	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->id, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->tamanioPayload, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &nroBloqueDatabin, bytesACopiar);
	desplazamiento += bytesACopiar;

	memcpy(paquete + desplazamiento, contenido, UN_BLOQUE);

	// Libero recursos.
	free(header);
	return paquete;
}

int guardarBloqueEnNodo(t_bloque *bloque, int COPIA) {
	int bytesEnviados, bytesAEnviar, rta, socketNodo;
	void *paquete, *respuesta;

	bytesAEnviar = sizeof(uint32_t) + UN_BLOQUE;

	if (COPIA == 0) { //COPIA 0
		socketNodo = bloque->nodoCopia0->socketDescriptor;
		paquete = serializarSetBloque(bloque->contenido,
				bloque->numeroBloqueCopia0);
	} else { //COPIA 1
		socketNodo = bloque->nodoCopia1->socketDescriptor;
		paquete = serializarSetBloque(bloque->contenido,
				bloque->numeroBloqueCopia1);
	}
	// Envio el paquete.
	bytesEnviados = enviarPorSocket(socketNodo, paquete, bytesAEnviar);
	if (bytesEnviados < bytesAEnviar) {
		fprintf(stderr, "[ERROR]: no se pudo enviar todo el paquete.\n");
		return ERROR_NO_SE_PUDO_GUARDAR_BLOQUE;
	}

	// Si se envio bien espero la respuesta.
	respuesta = malloc(sizeof(uint32_t));
	if (recibirPorSocket(socketNodo, respuesta, sizeof(uint32_t)) > 0) {
		if (*(int*) respuesta == GUARDO_BLOQUE_OK) {
			rta = GUARDO_BLOQUE_OK;
		} else {
			printf("[Error]: no se guardo el bloque.\n");
			rta = ERROR_NO_SE_PUDO_GUARDAR_BLOQUE;
		}
	} else {
		rta = ERROR_AL_RECIBIR_RESPUESTA;
	}

	free(respuesta);
	free(paquete);
	return rta;
}

int obtenerSocketNodo(t_bloque *bloque, int *nroBloqueDatabin) {
	int i;
	t_nodo *nodo;

	for (i = 0; i < nodos->elements_count; i++) {
		nodo = list_get(nodos, i);

		if (nodo->idNodo == bloque->nodoCopia0->idNodo) {
			*nroBloqueDatabin = bloque->numeroBloqueCopia0;
			return nodo->socketDescriptor;
		}

		if (nodo->idNodo == bloque->nodoCopia1->idNodo) {
			*nroBloqueDatabin = bloque->numeroBloqueCopia1;
			return nodo->socketDescriptor;
		}
	}

	return NO_DISPONIBLE;
}

void serializarHeaderTraerBloque(uint32_t id, uint32_t numBloqueDataBin,
		void* paquete) {
	t_header *header = malloc(sizeof(t_header));
	//header->id=malloc(sizeof(uint32_t));
	header->id = id;
	header->tamanioPayload = numBloqueDataBin;
	int desplazamiento = 0;
	int bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->id, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->tamanioPayload, bytesACopiar);
	desplazamiento += bytesACopiar;
	free(header);
}

int traerBloqueNodo(t_bloque *bloque) {
	int nroBloqueDatabin, rta = 0;
	int socketNodo = obtenerSocketNodo(bloque, &nroBloqueDatabin);
	if (socketNodo == NO_DISPONIBLE) {
		fprintf(stderr,
				"[ERROR]: el bloque numero '%d' no esta disponible en el sistema.\n",
				nroBloqueDatabin);
		return NO_DISPONIBLE;
	}

	void *paquete = malloc(sizeof(uint32_t) * 2);
	serializarHeaderTraerBloque(3, nroBloqueDatabin, paquete);
	int bytesEnviados = enviarPorSocket(socketNodo, paquete, 0);
	if (bytesEnviados <= 0) {
		fprintf(stderr, "[ERROR]: fallo al enviar peticion al nodo.\n");
		rta = ERROR_AL_ENVIAR_PETICION;
	} else {
		if (recibirPorSocket(socketNodo, bloque->contenido, UN_BLOQUE) > 0) {
			rta = TRAJO_BLOQUE_OK;
		} else
			rta = ERROR_AL_TRAER_BLOQUE;
	}

	// Libero recursos.
	free(paquete);

	return rta;
}

void mostrar(int cantidad) {
	int i;
	cantidad > 100 ? cantidad = 100 : cantidad;
	printf("index			nombre						padre\n");
	for (i = 0; i < cantidad; i++) {
		printf("%d			%s						%d\n", directorios[i].index, directorios[i].nombre,
				directorios[i].padre);
	}
}

int existeDirectorio(char *directorio, int *padre) {
	int i;
	for (i = 0; i < 100; i++) {
		if ((sonIguales(directorio, directorios[i].nombre))
				&& (directorios[i].padre == *padre)) {
			*padre = (int) directorios[i].index;
			return 1;
		}
	}
	return 0;
}

int buscarPrimerLugarLibre() {
	int posicion = 0, i;
	for (i = 0; i < 100; i++) {
		if (strcmp(directorios[i].nombre, "") == 0) {
			posicion = i;
			break;
		}
	}

	return posicion;
}

int obtenerIndice(t_directorio path) {
	char *nombre = string_substring_from(strrchr(path, '/'), 1);
	int i;

	for (i = 0; i < 100; i++) {
		if (sonIguales(nombre, directorios[i].nombre))
			return directorios[i].index;
	}

	return DIR_NO_EXISTE;
}

bool existePathDirectorio(char *path) {
	int coincidencias = 0;
	int i, padre = -1, cantidadPartesPath;
	char **pathSeparado;

	if (sonIguales(path, "/root"))
		return true; // El directorio root siempre existe en el sistema.

	pathSeparado = string_split(path, "/");
	cantidadPartesPath = cantidadArgumentos(pathSeparado);

	if (!sonIguales(pathSeparado[0], "root")) {
		return false;
	}

	for (i = 0; pathSeparado[i]; i++) {
		if (existeDirectorio(pathSeparado[i], &padre))
			coincidencias++;
		else
			break;
	}

	if (cantidadPartesPath - coincidencias == 0) {
		return true;
	}
	return false;
}

void mkdirFs(char *path) {
	int coincidencias = 0;
	int padre = -1;
	int i;
	int cantidadPartesPath;
	int posicionLibre = -1;
	char ** pathSeparado;

	pathSeparado = string_split(path, "/");
	cantidadPartesPath = cantidadArgumentos(pathSeparado);

	if (!sonIguales(pathSeparado[0], "root")) {
		printf(
				"mkdir: no se puede crear el directorio «%s»: No existe el directorio '%s'.\n",
				path, obtenerPathDirectorio(path));
		return;
	}

	for (i = 0; pathSeparado[i]; i++) {
		if (existeDirectorio(pathSeparado[i], &padre))
			coincidencias++;
		else
			break;
	}

	switch (cantidadPartesPath - coincidencias) {
	case 0:
		printf(
				"mkdir: no se puede crear el directorio «%s»: El directorio ya existe.\n",
				path);
		break;

	case 1:
		posicionLibre = buscarPrimerLugarLibre();
		if (posicionLibre != -1) {
			crearDirectorioLogico(pathSeparado[cantidadPartesPath - 1], padre,
					posicionLibre);
			crearDirectorioFisico(posicionLibre);
		} else
			printf(
					"mkdir: no se puede crear el directorio «%s»: La tabla de directorios esta completa.\n",
					path);
		break;

	default:
		printf(
				"mkdir: no se puede crear el directorio «%s»: No existe el directorio '%s'.\n",
				path, obtenerPathDirectorio(path));
	}
}

void crearDirectorioLogico(char* nombre, int padre, int indice) {
	directorios[indice].index = indice;
	cargarNombre(nombre, indice);
	directorios[indice].padre = padre;

	char *path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/directorios.dat");

	FILE *filePointer = fopen(path, "r+");

	fwrite(directorios, 1, sizeof(t_directory) * 100, filePointer);

	fclose(filePointer);
}

void cargarNombre(char* name, int indice) {
	int i = 0;
	while (name[i] != '\0') {
		directorios[indice].nombre[i] = name[i];
		i++;
	}

	directorios[indice].nombre[i] = '\0';
}

void validarMetadata(char* path) {
	//faltaria sumar aunque sea 1 al malloc por '\0'?
	char *newPath = malloc(strlen(path) + strlen("/metadata"));

	if (newPath) {
		newPath[0] = '\0';
		strcat(newPath, path);
		strcat(newPath, "/metadata");
	} else {
		fprintf(stderr, "malloc fallido!.\n");
	}

	DIR* directoryPointer = opendir(newPath);

	if (directoryPointer) {
		// El directorio existe.
		closedir(directoryPointer);
	}
	// Si el directorio no existe lo crea
	else if (ENOENT == errno) {
		mkdir("metadata", 0777); // Le damos todos los permisos.
		closedir(directoryPointer);
	}

	free(newPath);
}

void crearDirectorioFisico(int indice) {
	char *sIndice = string_itoa(indice);
	char *path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/archivos/");
	string_append(&path, sIndice);

	DIR* directoryPointer = opendir(path);
	if (directoryPointer) {
		// Si el directorio existe, no hace nada.
		closedir(directoryPointer);
	}
	// Si el directorio no existe, lo crea.
	else if (ENOENT == errno) {
		mkdir(path, 0777); // Le damos todos los permisos, por ahora.
		closedir(directoryPointer);
	}

	free(path);
}

/* Asumimos que el directorio metadata fue creado por el sistema, por lo cual
 * si hay un estado anterior este directorio deberia existir.
 */
bool hayEstadoAnterior() {
	bool hayEstado;
	DIR *directorio = opendir(PATH_METADATA);

	if (!directorio) {
		hayEstado = false;
	} else {
		hayEstado = true;
	}

	closedir(directorio);
	return hayEstado;
}

/* Implementacion de funciones de inicializacion */
void crearDirectorioMetadata() {
	DIR *directorio = opendir(PATH_METADATA);

	// Si el directorio no existe, lo crea.
	if (ENOENT == errno) {
		char *comando = string_new();
		string_append(&comando, "mkdir -p ");
		string_append(&comando, PATH_METADATA);
		system(comando);
	}

	crearDirectorioArchivos();
	crearTablaDeDirectorios();
	crearTablaDeNodos();
	crearDirectorioBitmaps();

	closedir(directorio);
}

void crearTablaDeDirectorios() {
	char *path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/directorios.dat");
	FILE *filePointer = fopen(path, "wb");

	if (!filePointer) {
		fprintf(stderr,
				"[ERROR]: no se pudo crear la tabla de directorios en la ruta '%s'.\nSugerencia: verificar si existe el directorio.",
				path);
	}

	// Carga el array en memoria y escribe en el archivo.
	directorios[0].index = 0;
	strcpy(directorios[0].nombre, "root");
	directorios[0].padre = -1;

	fwrite(&directorios[0], sizeof(t_directory), 1, filePointer);
	free(path);

	// Crea el directorio /metadata/archivos/0 (root).
	path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/archivos/0");

	DIR *directorio = opendir(path);
	// Si el directorio no existe, lo crea.
	if (ENOENT == errno) {
		char *comando = string_new();
		string_append(&comando, "mkdir ");
		string_append(&comando, path);
		system(comando);
	}

	// Libero recursos.
	free(path);
	closedir(directorio);
	fclose(filePointer);
}

void crearDirectorioArchivos() {
	char *path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/archivos");

	DIR *directorio = opendir(path);
	// Si el directorio no existe, lo crea.
	if (ENOENT == errno) {
		char *comando = string_new();
		string_append(&comando, "mkdir ");
		string_append(&comando, path);
		system(comando);
	}

	closedir(directorio);
}

void crearTablaDeNodos() {
	char *path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/nodos.bin");

	FILE *filePointer = fopen(path, "wb");
	if (!filePointer) {
		fprintf(stderr, "[ERROR]: no se pudo crear el archivo '%s'.\n", path);
		exit(EXIT_FAILURE);
	}

	// Inicializo la lista de nodos, ahora a esperar que se conecten...
	fclose(filePointer);
}

void crearDirectorioBitmaps() {
	char *path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/bitmaps");

	DIR *directorio = opendir(path);
	// Si el directorio no existe, lo crea.
	if (ENOENT == errno) {
		char *comando = string_new();
		string_append(&comando, "mkdir ");
		string_append(&comando, path);
		system(comando);
	}

	closedir(directorio);
}

void restaurarEstructurasAdministrativas() {
	restaurarTablaDeDirectorios();
	restaurarTablaDeArchivos();
	restaurarTablaDeNodos();
}

void restaurarTablaDeDirectorios() {
	int i;
	char *path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/directorios.dat");

	FILE *filePointer = fopen(path, "r+b");

	if (!filePointer) {
		perror("[Error]: No se encontro la tabla de directorios.");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < 100; i++) {
		fread(&directorios[i], sizeof(t_directory), 1, filePointer);
	}

	fclose(filePointer);
}

void restaurarTablaDeNodos() {
	estadoNodos = ACEPTANDO_NODOS_YA_CONECTADOS; // Ya que se restaura la lista de nodos, solo se aceptan nodos ya conectados previamente

	int i, largo = 0;
	uint32_t tamanioLibreNodo;
	t_nodo *nodo;
	char *sNodos, **idNodos, *path, *keyNodoLibre;
	path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/nodos.bin");
	t_config* diccionario = config_create(path);

	// Validaciones
	if (!diccionario) {
		fprintf(stderr, "[Error]: no se encontro la tabla de nodos.\n");
		exit(EXIT_FAILURE);
	}

	nodos = list_create();
	nodosEsperados = list_create();

	sNodos = config_get_string_value(diccionario, "NODOS");
	if (!sNodos) {
		fprintf(stderr,
				"[Error]: la tabla de nodos de un estado anterior esta vacia.\n"); // Esta situacion nunca se deberia dar ...
		return;
	}

	largo = strlen(sNodos);
	sNodos = string_substring(sNodos, 1, largo - 2);
	idNodos = string_split(sNodos, ",");
	for (i = 0; idNodos[i]; i++) {
		nodo = malloc(sizeof(t_nodo));
		nodo->idNodo = atoi(idNodos[i]);

		keyNodoLibre = string_new();
		string_append(&keyNodoLibre, "Nodo");
		string_append(&keyNodoLibre, idNodos[i]);
		string_append(&keyNodoLibre, "Libre");
		tamanioLibreNodo = config_get_int_value(diccionario, keyNodoLibre);
		nodo->bloquesLibres = tamanioLibreNodo;

		list_add(nodosEsperados, nodo);
	}

	puts("Mostrando lista de nodos cargada en memoria...\n");
	printf("NODOS = %s \n", sNodos);
	for (i = 0; i < nodosEsperados->elements_count; i++) {
		nodo = list_get(nodosEsperados, i);
		printf("Id nodo: %d\n Bloques libres: %d\n", nodo->idNodo,
				nodo->bloquesLibres);
	}
}

// Ver si tiene sentido tener todas las referencias a archivos del FS en memoria...PREGUNTAR!!
void restaurarTablaDeArchivos() {
	/*int i;
	 char *path;
	 FILE *archivo;
	 t_list *nombres = list_create();

	 for (i = 0; i < CANTIDAD_DIRECTORIOS; i++) {
	 path = string_new();
	 string_append(&path, PATH_METADATA);
	 string_append(&path, "/");
	 string_append(&path, string_itoa(i));
	 archivo = fopen(path, "r");

	 if(!archivo) {
	 fprintf(stderr, "[Error]: no se encontro la metadata del archivo.\n");
	 exit(EXIT_FAILURE);
	 }


	 list_clean(nombres);
	 free(path);
	 }

	 list_destroy_and_destroy_elements(nombres, free);*/
}

void agregarNodo(t_list *lista, t_nodo *nodo) {
	if (!lista) {
		fprintf(stderr, "[WARNING]: La lista no esta inicializada.");
	} else {
		list_add(lista, nodo);
	}

	// Se conectaron al menos
	if (lista->elements_count >= 2)
		sem_post(&semNodosRequeridos);
}

t_list* copiarListaNodos(t_list *lista) {
	t_list *copia = list_create();
	t_nodo *aux;
	int i;
	for (i = 0; i < lista->elements_count; i++) {
		t_nodo *nodo = malloc(sizeof(t_nodo));
		aux = list_get(lista, i);
		nodo->idNodo = aux->idNodo;
		nodo->socketDescriptor = aux->socketDescriptor;
		nodo->bloquesTotales = aux->bloquesTotales;
		nodo->bloquesLibres = aux->bloquesLibres;
		nodo->bitmap = string_duplicate(aux->bitmap);
		nodo->ip = string_duplicate(aux->ip);
		nodo->puertoWorker = aux->puertoWorker;
		list_add(copia, nodo);
	}

	return copia;
}

int totalBloquesFileSystem() {
	t_nodo *nodo;
	int i, totalBloques = 0;

	for (i = 0; i < nodos->elements_count; i++) {
		nodo = list_get(nodos, i);
		totalBloques += nodo->bloquesTotales;
	}

	return totalBloques;
}

int bloquesLibresFileSystem() {
	t_nodo *nodo;
	int i, bloquesLibres = 0;

	for (i = 0; i < nodos->elements_count; i++) {
		nodo = list_get(nodos, i);
		bloquesLibres += nodo->bloquesLibres;
	}

	return bloquesLibres;
}

void persistirTablaDeNodos() {
	int i;
	t_nodo *nodo;
	char *clave, *valor, *path;

	path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/nodos.bin");
	FILE *filePointer = fopen(path, "r+b");

	if (!filePointer) {
		fprintf(stderr, "[ERROR]: no se encontro el archivo '%s'.\n", path);
		exit(EXIT_FAILURE);
	}

	// Ordeno la lista de nodos para que me quede en orden creciente (descendente).
	list_sort(nodos, (void*) compararPorIdDesc);

	// TAMANIO.
	clave = "TAMANIO=";
	valor = string_itoa(totalBloquesFileSystem());
	fputs(clave, filePointer);
	fputs(valor, filePointer);

	// LIBRE.
	clave = "\nLIBRE=";
	valor = string_itoa(bloquesLibresFileSystem());
	fputs(clave, filePointer);
	fputs(valor, filePointer);

	// NODOS
	clave = "\nNODOS=";
	valor = string_new();
	string_append(&valor, "[");
	nodo = list_get(nodos, 0);
	string_append(&valor, string_itoa(nodo->idNodo));
	for (i = 1; i < nodos->elements_count; i++) {
		nodo = list_get(nodos, i);
		string_append(&valor, ",");
		string_append(&valor, string_itoa(nodo->idNodo));
	}
	string_append(&valor, "]");
	fputs(clave, filePointer);
	fputs(valor, filePointer);
	free(valor);

	for (i = 0; i < nodos->elements_count; i++) {
		nodo = list_get(nodos, i);

		// NodoxTotal
		clave = string_new();
		string_append(&clave, "\nNodo");
		string_append(&clave, string_itoa(nodo->idNodo));
		string_append(&clave, "Total=");
		valor = string_itoa(nodo->bloquesTotales);
		fputs(clave, filePointer);
		fputs(valor, filePointer);
		free(clave);

		// NodoxLibre
		clave = string_new();
		string_append(&clave, "\nNodo");
		string_append(&clave, string_itoa(nodo->idNodo));
		string_append(&clave, "Libre=");
		valor = string_itoa(nodo->bloquesLibres);
		fputs(clave, filePointer);
		fputs(valor, filePointer);
		free(clave);
	}

	// Libero recursos.
	fclose(filePointer);
}

void actualizarTablaDeNodos() {
	char *path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/nodos.bin");
	FILE *filePointer = fopen(path, "wb");

	if (!filePointer) {
		fprintf(stderr, "[ERROR]: no se encontro el archivo '%s'.\n", path);
		exit(EXIT_FAILURE);
	}

	persistirTablaDeNodos();

	fclose(filePointer);
	free(path);
}

// Busca por ese criterio en la lista global de archivos en memoria.
t_archivo_a_persistir* buscarArchivoPorIndiceYNombre(int indice,
		char *nombreArchivo) {
	int i;
	t_archivo_a_persistir *archivo;
	for (i = 0; i < archivos->elements_count; i++) {
		archivo = list_get(archivos, i);
		if (archivo->indiceDirectorio == indice
				&& sonIguales(archivo->nombreArchivo, nombreArchivo))
			return archivo;
	}
	return NULL; // Devuelve NULL si no lo encuentra.
}

// Devuelve la ultima ocurrencia de un caracter dado en una cadena.
int lastIndexOf(char *cadena, char caracter) {
	int i, ultimaPosicion = -1;

	for (i = 0; i < string_length(cadena); i++) {
		if (cadena[i] == caracter)
			ultimaPosicion = i;
	}

	return ultimaPosicion;
}

// Obtiene el archivo segun su path, devuelve NULL si el archivo no existe en ese directorio.
t_archivo_a_persistir* obtenerArchivo(char *path) {
	t_archivo_a_persistir *archivo;
	char *directorio = string_substring_until(path, lastIndexOf(path, '/'));
	char *nombreArchivo = string_substring_from(path,
			lastIndexOf(path, '/') + 1);
	int indice = obtenerIndice(directorio);

	if (indice == DIR_NO_EXISTE)
		return NULL;

	archivo = buscarArchivoPorIndiceYNombre(indice, nombreArchivo);
	if (!archivo)
		return NULL;

	return archivo;
}

// Si no es un directorio sintacticamente valido devuelve NULL.
t_directorio obtenerPathDirectorio(char *path) {
	if (sonIguales(path, "/root")) {
		return path;
	} else if (string_starts_with(path, "/root/")) {
		return string_substring_until(path, lastIndexOf(path, '/'));
	} else {
		return NULL;
	}
}

char* obtenerNombreArchivo(char *path) {
	return string_substring_from(path, lastIndexOf(path, '/') + 1);
}

void renombrarArchivo(char *pathOriginal, char *nombreFinal) {
	char *comando, *nombreInicial, *path;
	t_directorio directorio;
	int indice;
	FILE *filePointer;

	// Valido que exista el directorio original.
	directorio = obtenerPathDirectorio(pathOriginal);
	indice = obtenerIndice(directorio);
	if (indice == DIR_NO_EXISTE) {
		printf("El directorio '%s' no existe.\n", directorio);
		return;
	}

	nombreInicial = obtenerNombreArchivo(pathOriginal);

	// Me genero el path donde esta el archivo con la metadata del archivo :p.
	path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/archivos/");
	string_append(&path, string_itoa(indice));
	string_append(&path, "/");
	string_append(&path, nombreInicial);

	filePointer = fopen(path, "r+");
	if (!filePointer) {
		printf("El archivo %s no existe.\n", pathOriginal);
		return;
	}

	// Actualizo el nombre del archivo usando el comando 'mv' de linux.
	comando = string_new();
	string_append(&comando, "mv ");
	string_append(&comando, PATH_METADATA);
	string_append(&comando, "/archivos/");
	string_append(&comando, string_itoa(indice));
	string_append(&comando, "/");
	string_append(&comando, nombreInicial);

	string_append(&comando, " ");
	string_append(&comando, PATH_METADATA);
	string_append(&comando, "/archivos/");
	string_append(&comando, string_itoa(indice));
	string_append(&comando, "/");
	string_append(&comando, nombreFinal);

	// Ejecuto.
	system(comando);

	// Libero recursos.
	free(comando);
	free(path);
	free(nombreInicial);
	fclose(filePointer);
}

void renombrarDirectorio(char *pathOriginal, char *nombreFinal) {
	char *pathDirectorios;
	int i, indice;
	FILE *archivo;

	indice = obtenerIndice(pathOriginal);
	if (indice == DIR_NO_EXISTE) {
		printf("El directorio '%s' no existe.\n", pathOriginal);
		return;
	}

	pathDirectorios = string_new();
	string_append(&pathDirectorios, PATH_METADATA);
	string_append(&pathDirectorios, "/directorios.dat");

	archivo = fopen(pathDirectorios, "r+b");
	if (!archivo) {
		fprintf(stderr,
				"[ERROR]: no se encontro el archivo 'directorios.dat'.\n");
		exit(EXIT_FAILURE);
	}

	// Actualizo la tabla de directorios en memoria.
	strcpy(directorios[indice].nombre, nombreFinal);

	// Actualizo la tabla de directorios en disco.
	for (i = 0; i < 100; i++)
		fwrite(&directorios[i], sizeof(t_directory), 1, archivo);

	// Libero recursos.
	fclose(archivo);
	free(pathDirectorios);
}

void moverArchivo(char *pathOriginal, t_directorio pathFinal) {
	int indiceOriginal, indiceFinal;
	char *nombreArchivo, *metadataOriginal, *metadataFinal, *comando;
	t_directorio directorio;

	// Validaciones.
	directorio = obtenerPathDirectorio(pathOriginal);
	if (sonIguales(directorio, pathFinal))
		return; // Si lo mueve al mismo lugar no hacer nada.

	// Si alguno de los dos paths no existen informar.
	indiceOriginal = obtenerIndice(directorio);
	if (indiceOriginal == DIR_NO_EXISTE) {
		printf("El directorio '%s' no existe.\n", pathFinal);
		return;
	}

	indiceFinal = obtenerIndice(pathFinal);
	if (!existePathDirectorio(pathFinal) || indiceFinal == DIR_NO_EXISTE) {
		printf("El directorio '%s' no existe.\n", pathFinal);
		return;
	}

	nombreArchivo = obtenerNombreArchivo(pathOriginal);

	// Busco el archivo en disco.
	metadataOriginal = string_new();
	string_append(&metadataOriginal, PATH_METADATA);
	string_append(&metadataOriginal, "/archivos/");
	string_append(&metadataOriginal, string_itoa(indiceOriginal));
	string_append(&metadataOriginal, "/");
	string_append(&metadataOriginal, nombreArchivo);

	// Si no lo encuentra informa.
	if (access(metadataOriginal, 0) != 0) {
		printf("El archivo '%s' no existe.\n", pathOriginal);
		return;
	}

	// Si lo encuentra prepara la ruta para el comando de linux.
	metadataFinal = string_new();
	string_append(&metadataFinal, PATH_METADATA);
	string_append(&metadataFinal, "/archivos/");
	string_append(&metadataFinal, string_itoa(indiceFinal));
	string_append(&metadataFinal, "/");
	string_append(&metadataFinal, nombreArchivo);

	comando = string_new();
	string_append(&comando, "mv ");
	string_append(&comando, metadataOriginal);
	string_append(&comando, " ");
	string_append(&comando, metadataFinal);

	// Y lo ejecuta.
	system(comando);

	// Por ultimo libero recursos.
	free(metadataOriginal);
	free(metadataFinal);
	free(nombreArchivo);
	free(comando);
}

void moverDirectorio(t_directorio pathOriginal, t_directorio pathFinal) {
	int i, indiceOriginal, indiceFinal;
	char *pathDirectorios;
	FILE *archivo;

	// Validaciones.
	indiceOriginal = obtenerIndice(pathOriginal);
	if (indiceOriginal == DIR_NO_EXISTE) {
		printf("El directorio '%s' no existe.\n", pathOriginal);
		return;
	}

	indiceFinal = obtenerIndice(pathFinal);
	if (indiceFinal == DIR_NO_EXISTE) {
		printf("El directorio '%s' no existe.\n", pathFinal);
		return;
	}

	pathDirectorios = string_new();
	string_append(&pathDirectorios, PATH_METADATA);
	string_append(&pathDirectorios, "/directorios.dat");

	archivo = fopen(pathDirectorios, "r+b");
	if (!archivo) {
		fprintf(stderr,
				"[ERROR]: no se encontro el archivo 'directorios.dat'.\n");
		exit(EXIT_FAILURE);
	}

	// Actualizo en memoria.
	directorios[indiceOriginal].padre = directorios[indiceFinal].index;

	// Actualizo la tabla de directorios en disco.
	for (i = 0; i < 100; i++)
		fwrite(&directorios[i], sizeof(t_directory), 1, archivo);

	// Libero recursos.
	fclose(archivo);
	free(pathDirectorios);
}

// Si el archivo a abrir no existe en ese path retorna NULL.
t_archivo_a_persistir* abrirArchivo(char *pathArchivo) {
	int i, indiceDirectorio, cantidadBloques, ID_NODO = 0, NRO_BLOQUE_DATABIN =
			1;
	char *path, *nombreArchivo, *clave, **valores;
	t_config *diccionario;
	t_bloque *bloque;
	t_nodo *nodoCopia0, *nodoCopia1;
	t_archivo_a_persistir *archivo;

	indiceDirectorio = obtenerIndice(obtenerPathDirectorio(pathArchivo));
	nombreArchivo = obtenerNombreArchivo(pathArchivo);

	path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/archivos/");
	string_append(&path, string_itoa(indiceDirectorio));
	string_append(&path, "/");
	string_append(&path, nombreArchivo);

	// Verifico existencia del archivo en ese path.
	diccionario = config_create(path);
	if (!diccionario)
		return NULL;

	// Pido memoria correspondiente.
	archivo = malloc(sizeof(t_archivo_a_persistir));

	// Nombre.
	archivo->nombreArchivo = string_duplicate(nombreArchivo);

	// Tamaño.
	archivo->tamanio = config_get_int_value(diccionario, "TAMANIO");

	// Tipo.
	archivo->tipo = config_get_int_value(diccionario, "TIPO");

	// Indice (es util luego para calcular el indice del padre).
	archivo->indiceDirectorio = indiceDirectorio;

	// Bloques.
	archivo->bloques = list_create();
	cantidadBloques = archivo->tamanio / UN_MEGABYTE + 1; // Division entera
	for (i = 0; i < cantidadBloques; i++) {
		// Reservo un bloque.
		bloque = malloc(sizeof(t_bloque));
		nodoCopia0 = malloc(sizeof(t_nodo));
		nodoCopia1 = malloc(sizeof(t_nodo));

		bloque->numeroBloque = i;

		bloque->nodoCopia0 = nodoCopia0;
		clave = string_new();
		string_append(&clave, "BLOQUE");
		string_append(&clave, string_itoa(i));
		string_append(&clave, "COPIA0");
		valores = config_get_array_value(diccionario, clave);
		bloque->nodoCopia0->idNodo = atoi(valores[ID_NODO]);
		bloque->numeroBloqueCopia0 = atoi(valores[NRO_BLOQUE_DATABIN]);

		free(clave);
		free(valores[ID_NODO]);
		free(valores[NRO_BLOQUE_DATABIN]);

		bloque->nodoCopia1 = nodoCopia1;
		clave = string_new();
		string_append(&clave, "BLOQUE");
		string_append(&clave, string_itoa(i));
		string_append(&clave, "COPIA1");
		valores = config_get_array_value(diccionario, clave);
		bloque->nodoCopia1->idNodo = atoi(valores[ID_NODO]);
		bloque->numeroBloqueCopia1 = atoi(valores[NRO_BLOQUE_DATABIN]);

		free(clave);
		free(valores[ID_NODO]);
		free(valores[NRO_BLOQUE_DATABIN]);

		clave = string_new();
		string_append(&clave, "BLOQUE");
		string_append(&clave, string_itoa(i));
		string_append(&clave, "BYTES");
		bloque->bytesOcupados = config_get_int_value(diccionario, clave);
		free(clave);

		list_add(archivo->bloques, bloque);
	}

	// Libero recursos. Liberar las listas desde la funcion que llama.
	free(path);
	config_destroy(diccionario);

	return archivo;
}

/* 0: binario (o shell script, ver si esta bien considerarlo asi).
 * 1: de texto.
 * -1: fallo en la busqueda.
 */
int obtenerTipo(char *pathArchivo) {
	/* El pipeline es unidireccional, si quiero que mi proceso
	 * padre le envie algo al hijo y que tambien reciva del hijo tengo que tener 2 pipes. */
	int tipo, status, pipe_padreAHijo[2], pipe_hijoAPadre[2];
	char *buffer, *comando;
	pid_t pid = 0;

	// Reservo un buffer para guardar el resultado del script.
	buffer = string_new();

	// Preparo el comando.
	comando = string_new();
	string_append(&comando, "/home/utnso/thePonchos/obtenerTipo.sh ");
	string_append(&comando, pathArchivo);

	// Creo los pipes, pidiendole a la funcion los fileDescriptors que seran los extremos del "tubo" que se habra construido.
	pipe(pipe_padreAHijo);
	pipe(pipe_hijoAPadre);

	// Creo un proceso hijo.
	pid = fork();
	/* Con esta sentencia ya cree un proceso hijo, devuelve 0 si la operacion salio bien,
	 *  no es el PID del proceso hijo sino un PID de referencia para el padre. */
	if (pid == 0) {
		// Aca estamos ejecutando lo que va a correr el proceso hijo.

		/* Lo que hace dup2() es duplicar un fd, creando una copia del mismo en donde yo le diga.
		 En este caso, estoy cambiando el fd de la entrada estandar del proceso hijo (STDIN_FILENO)
		 por el fd de uno de los extremos del pipe padre->hijo, precisamente la salida:
		 (pipe_padreAHijo[0], aca es donde el hijo debe leer lo que el padre le escribe).

		 Tambien estoy reemplazando el fd de la salida estadar del hijo (STDOUT_FILENO),
		 por el extremo de entrada del pipe hijo->padre:
		 (pipe_hijoAPadre[1], aca es donde el padre debe leer lo que el hijo le escribe). */
		dup2(pipe_padreAHijo[0], STDIN_FILENO);
		dup2(pipe_hijoAPadre[1], STDOUT_FILENO);
		/* Regla de oro para los pipes: 0 es lectura, 1 es escritura. */

		// Cierro los extremos del ambos pipes, ya que cuando ejecute el proceso hijo estos se duplicaran lo cual puede traer errores no deseados.
		close(pipe_padreAHijo[0]);
		close(pipe_padreAHijo[1]);
		close(pipe_hijoAPadre[0]);
		close(pipe_hijoAPadre[1]);

		system(comando);
		exit(1); // Fin del hijo.
	} else {
		// Vamos con el padre..
		close(pipe_padreAHijo[0]); // Lado de lectura de lo que el padre le pasa al hijo.
		close(pipe_hijoAPadre[1]); // Lado de escritura de lo que hijo le pasa al padre.

		/* Aca cierro los fds que no me interesan porque son los que esta usando el hijo,
		 * tambien los cierros por el tema del duplicado explicado mas arriba. Me quedan los otros 2 que son:
		 *
		 + pipe_padreAHijo[1] -> Lado de escritura de lo que el padre le pasa al hijo.
		 + pipe_hijoAPadre[0] -> Lado de lectura de lo que el hijo le pasa al padre. */

		/* Escribo en el proceso hijo. Cuando el este lo reciba, lo va a recibir como entrada estandar.
		 Le estoy mandando el path del archivo al script. */
		write(pipe_padreAHijo[1], pathArchivo, strlen(pathArchivo));

		// Como termine de escribir cierro esta parte del pipe.
		close(pipe_padreAHijo[1]);

		waitpid(pid, &status, 0);
		/* Esto es el "mata zombies", lo que hace waitpid es esperar a que el proceso hijo termine. Se logra el mismo resultado con wait().
		 * Espera el cambio de estado del proceso hijo (pid) y guarda el trace en ejecucion
		 * (lo guarda en status, segun lo que yo quiera consultar puedo, por ejemplo, saber si el hijo recibio una señal,
		 * cual fue su exitcode, etc... a nosotros no nos interesa nada de eso y no lo vamos a usar,
		 * entonces ponemos un 0 en el ultimo argumento, que son las opciones de trace, para indicar eso).

		 ----- Mientras tanto el padre se queda bloqueado -----.
		 Si no se hace esto, cuando el hijo termine la ejecucion por exit() no va a poder irse
		 ya que la entrada del proceso hijo en la tabla de procesos del sistema operativo
		 sigue estando para que el padre lea el codigo de salida.
		 El proceso hijo se encuentra en un estado de terminacion que se lo conoce como "zombie",
		 tecnicamente esta muerto porque hizo un exit pero sigue vivo en la tabla de procesos.

		 Esto se debe que la llamada de wait() o waitpid() permite al padre leer el exitcode o codigo de salida del hijo.
		 Si el padre nunca se entero que el hijo esta muerto, el SO no lo puede sacar (salvo a la fuerza).

		 ¿Que problemas trae un proceso zombie? No muchos en cuestion de memoria porque no usan recursos del sistema,
		 sin embargo tienen un PID asignado por el SO, del cual el sistema operativo tiene un numero finitos de estos.
		 Un zombie no causa muchos problemas. Varios zombies me limitan la cantidad de procesos que puedo ejecutar,
		 si estamos debuggeando un programa que cree zombies sin querer nos puede limitar el numero de procesos disponibles.
		 Por eso es importante matarlos. */

		/* Leo de un proceso hijo, ahora el resultado de mi script se encuentra en "buffer" y tiene un tamaño 1.
		 * Como termine de leer cierro el extremo del pipe. */
		read(pipe_hijoAPadre[0], buffer, 1);
		close(pipe_hijoAPadre[0]);
	}

	// Determino el resultado de la operacion.
	if (isdigit(buffer[0])) {
		tipo = buffer[0] - '0'; // Convierto a int restandole la base '0' (48 ascii)
	} else {
		tipo = -1;
	}

	// Libero recursos.
	free(comando);
	free(buffer);
	return tipo;
}

bool existeArchivoEnYamaFs(char *pathArchivo) {
	int indice;
	char *nombreArchivo, *pathMetadataArchivo;
	t_directorio directorio;

	// Verifico que exista el directorio en yamafs.
	directorio = obtenerPathDirectorio(pathArchivo);
	if (!directorio || !existePathDirectorio(directorio))
		return false;

	indice = obtenerIndice(directorio);
	nombreArchivo = obtenerNombreArchivo(pathArchivo);

	// Verifico que exista el archivo 'nombreArchivo' en el 'directorio'.
	pathMetadataArchivo = string_new();
	string_append(&pathMetadataArchivo, PATH_METADATA);
	string_append(&pathMetadataArchivo, "/archivos/");
	string_append(&pathMetadataArchivo, string_itoa(indice));
	string_append(&pathMetadataArchivo, "/");
	string_append(&pathMetadataArchivo, nombreArchivo);

	// Si lo encuentra retorna 'true' sino 'false'.
	if (access(pathMetadataArchivo, F_OK) != -1)
		return true;
	else
		return false;
}

//crea un socket. Lo conecta a yama y se queda esperando peticiones de informacion de archivos.
void *escucharPeticionesYama() {
	t_header *header;
	header = malloc(sizeof(t_header));

	struct addrinfo hints;
	struct addrinfo *serverInfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(NULL, PUERTO_YAMA, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE
	int socketYama;
	socketYama = socket(serverInfo->ai_family, serverInfo->ai_socktype,
			serverInfo->ai_protocol);
	bind(socketYama, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo); // Ya no lo vamos a necesitar
	listen(socketYama, BACKLOG); // IMPORTANTE: listen() es una syscall BLOQUEANTE.
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int socketCliente = accept(socketYama, (struct sockaddr *) &addr, &addrlen);
	/*
	 Cuando el cliente cierra la conexion, recv() devolvera 0.
	 */

	int status = 1;		// Estructura que manjea el status de los recieve.
	while (status != 0) {
		status = recibirHeader(socketYama, header);

		//status = recv(socketCliente, (void*) package, 8, 0);//8 ES EL TAMANIO DEL HEADER ENVIADOS DESDE YAMA
		if (status != 0) {
			void * peticionRecibida = malloc(header->tamanioPayload);
			char* pathArchivo;
			char* pathGuardadoFinal;

			status = recv(socketCliente, (void*) peticionRecibida,
					header->tamanioPayload, 0);	//Recibo el path del archivo que yama me pide informacion

			deserializarPeticionInfoArchivo(&peticionRecibida, &pathArchivo,
					&pathGuardadoFinal);
			//REVISAR EXISTENCIA DE PATH GUARDADO FINAL . Y EXISTENCIA DEL ARCHIVO. SI ALGUNO FALLA DEVUELVE ERROR.

			t_archivo_a_persistir* archivo = abrirArchivo(pathArchivo);
			if (archivo != NULL) {
				void * paqueteRespuesta = NULL;	//Para que no me tire el warning de que no esta inicializado. Se hace un malloc cuando se serializa
				t_header* headerRta;
				headerRta = malloc(sizeof(t_header));
				serializarInfoArchivo(archivo, paqueteRespuesta, headerRta);

			} else {
				//Enviar respuesta con error al yama. Solo con el header alcanza.

			}
			free(peticionRecibida);
			free(pathArchivo);
			free(pathGuardadoFinal);
		}
	}
	close(socketCliente);
	close(socketYama);
	return 0;
}

// Retorna 1 si el archivo es regular, 0 si no , y -1 si se produjo un error.
int esArchivoRegular(char *path) {
	struct stat st;

	if (stat(path, &st) < 0)
		return -1;

	return S_ISREG(st.st_mode);
}

void deserializarPeticionInfoArchivo(void *paquete, char ** rutaArchivo,
		char ** rutaGuardadoFinal) {
	int desplazamiento = 0;
	uint32_t * largoALeer = malloc(sizeof(uint32_t));

	memcpy(largoALeer, paquete, sizeof(uint32_t));	//largo de la ruta archivo
	desplazamiento += sizeof(uint32_t);

	*rutaArchivo = malloc(sizeof(char) * (*largoALeer));

	memcpy(*rutaArchivo, paquete + desplazamiento, *largoALeer); //ruta del archivo que se va a procesar en el sistema
	desplazamiento += *largoALeer;

	memcpy(largoALeer, paquete + desplazamiento, sizeof(uint32_t)); //largo de la ruta archivo
	desplazamiento += sizeof(uint32_t);

	*rutaGuardadoFinal = malloc(*largoALeer);

	memcpy(*rutaGuardadoFinal, paquete + desplazamiento, *largoALeer); //ruta donde se va a guardar el resultado del proceso

	free(largoALeer);

}

void serializarInfoArchivo(t_archivo_a_persistir *archivo,
		void* paqueteRespuesta, t_header *header) {
	paqueteRespuesta = malloc(
			((archivo->bloques->elements_count * 6) * sizeof(uint32_t))); //Falta agregar la cantidad de bloques.6  =   numBloque + idNodoCopia0+numBloqueCopia0+idNodoCopia1+numBloqueCopia1+tamanioBloque

	/*Estructura del paquete:
	 * header + tamanioPayload
	 * cantidad de bloques
	 * bloques:
	 * 			numBloque
	 idNodoCopia0
	 numBloqueCopia0
	 idNodoCopia1
	 numBloqueCopia1
	 tamanioBloque
	 *
	 *
	 *
	 * */
	int cantidadDebloques = archivo->bloques->elements_count;
	int desplazamiento = sizeof(int);
	memcpy(paqueteRespuesta, &cantidadDebloques, sizeof(int));
	int i;
	for (i = 0; i < archivo->bloques->elements_count; i++) {
		t_bloque *bloque = list_get(archivo->bloques, i);
		//ir agregando la informacion de cada bloque al paquete respuesta(payload).
		memcpy(paqueteRespuesta + desplazamiento, &bloque->numeroBloque,
				sizeof(uint32_t));	//numero de bloque
		desplazamiento += sizeof(uint32_t);
		memcpy(paqueteRespuesta + desplazamiento, &bloque->nodoCopia0->idNodo,
				sizeof(uint32_t));	//idNodoCopia0
		desplazamiento += sizeof(uint32_t);
		memcpy(paqueteRespuesta + desplazamiento, &bloque->numeroBloqueCopia0,
				sizeof(int)); //numero bloque copia 0
		desplazamiento += sizeof(int);

		memcpy(paqueteRespuesta + desplazamiento, &bloque->nodoCopia0->idNodo,
				sizeof(uint32_t)); //idNodoCopia1
		desplazamiento += sizeof(uint32_t);
		memcpy(paqueteRespuesta + desplazamiento, &bloque->numeroBloqueCopia0,
				sizeof(int)); //numero bloque copia 1
		desplazamiento += sizeof(int);

		memcpy(paqueteRespuesta + desplazamiento, &bloque->bytesOcupados,
				sizeof(size_t));
		desplazamiento += sizeof(size_t);
	}
}

// Devuelve un bloque del archivo que se encuentra en 'pathArchivo' o NULL si algo fallo.
t_bloque* obtenerBloque(char *pathArchivo, int numeroBloque) {
	int indice, ID_NODO = 0, NRO_BLOQUE_DATABIN = 1, resultado;
	char *pathMetadataArchivo, *nombreArchivo, *clave, **valores;
	t_directorio directorio;
	t_config *diccionario;
	t_nodo *nodoCopia0, *nodoCopia1;
	t_bloque *bloque;

	// Verifica que exista el archivo.
	if (!existeArchivoEnYamaFs(pathArchivo)) {
		printf("El archivo '%s' no existe.\n", pathArchivo);
		return NULL;
	}

	// Busca en la metadata del archivo, necesita el id del nodo en donde se encuentra el contenido del bloque que quiero replicar.
	nombreArchivo = obtenerNombreArchivo(pathArchivo);
	directorio = obtenerPathDirectorio(pathArchivo);
	indice = obtenerIndice(directorio);

	pathMetadataArchivo = string_new();
	string_append(&pathMetadataArchivo, PATH_METADATA);
	string_append(&pathMetadataArchivo, "/archivos/");
	string_append(&pathMetadataArchivo, string_itoa(indice));
	string_append(&pathMetadataArchivo, "/");
	string_append(&pathMetadataArchivo, nombreArchivo);

	diccionario = config_create(pathMetadataArchivo);
	if (!diccionario) {
		fprintf(stderr, "[ERROR]: no se pudo abrir la metadata del archivo.\n");
		return NULL;
	}

	// Reservo memoria para un bloque.
	bloque = malloc(sizeof(t_bloque));
	bloque->contenido = malloc(UN_BLOQUE);
	nodoCopia0 = malloc(sizeof(t_nodo));
	nodoCopia1 = malloc(sizeof(t_nodo));

	bloque->numeroBloque = numeroBloque;

	// Cargo el objeto bloque, para luego traer el contenido del bloque de alguna de las dos copias.
	bloque->nodoCopia0 = nodoCopia0;
	clave = string_new();
	string_append(&clave, "BLOQUE");
	string_append(&clave, string_itoa(numeroBloque));
	string_append(&clave, "COPIA0");

	// Verifico que el numeroBloque del archivo exista. Antes de que falle cuando trata de recuperarlo.
	if (!config_has_property(diccionario, clave)) {
		printf("No existe el numero de bloque '%d' en el archivo '%s'.\n",
				numeroBloque, pathArchivo);
		return NULL;
	}

	valores = config_get_array_value(diccionario, clave);
	bloque->nodoCopia0->idNodo = atoi(valores[ID_NODO]);
	bloque->numeroBloqueCopia0 = atoi(valores[NRO_BLOQUE_DATABIN]);

	free(clave);
	free(valores[ID_NODO]);
	free(valores[NRO_BLOQUE_DATABIN]);

	bloque->nodoCopia1 = nodoCopia1;
	clave = string_new();
	string_append(&clave, "BLOQUE");
	string_append(&clave, string_itoa(numeroBloque));
	string_append(&clave, "COPIA1");

	if (!config_has_property(diccionario, clave)) {
		printf("No existe el numero de bloque '%d' en el archivo '%s'.\n",
				numeroBloque, pathArchivo);
		return NULL;
	}

	valores = config_get_array_value(diccionario, clave);
	bloque->nodoCopia1->idNodo = atoi(valores[ID_NODO]);
	bloque->numeroBloqueCopia1 = atoi(valores[NRO_BLOQUE_DATABIN]);

	free(clave);
	free(valores[ID_NODO]);
	free(valores[NRO_BLOQUE_DATABIN]);

	// Si pasa las validaciones, nos queda traer el contenido del bloque de alguno de los dos nodos que este conectado.
	resultado = traerBloqueNodo(bloque);
	if (resultado == ERROR_AL_TRAER_BLOQUE) {
		fprintf(stderr,
				"[ERROR]: no se pudo traer el bloque n° '%d' del archivo '%s'.\n",
				numeroBloque, pathArchivo);
		return NULL;
	}

	// Libero recursos.
	free(valores);
	config_destroy(diccionario);
	return bloque;
}

int guardarBloque(t_bloque *bloque, int idNodo) {
	int respuesta;

	respuesta = guardarBloqueEnNodo(bloque, 0);
	// Si salio mal.
	if (respuesta == ERROR_AL_RECIBIR_RESPUESTA) {
		printf("Se produjo un error al recibir la respuesta.\n");
		return ERROR_NO_SE_PUDO_GUARDAR_BLOQUE;
	}

	// Si guardo ok. :)
	return GUARDO_BLOQUE_OK;
}

void persistirBitmaps() {
	int i;
	t_nodo *nodo;
	for (i = 0; i < nodos->elements_count; i++) {
		nodo = list_get(nodos, i);
		nodo->bitmap = persistirBitmap(nodo->idNodo, nodo->bloquesTotales);
	}
}

bool esNodoAnterior(t_list *nodosEsperados, int idNodo) {
	int i;
	t_nodo *nodo;

	for (i = 0; i < nodosEsperados->elements_count; i++) {
		nodo = list_get(nodosEsperados, i);
		if(nodo->idNodo == idNodo)
			return true;
	}
	return false;
}
