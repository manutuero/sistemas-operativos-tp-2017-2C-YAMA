#include "funcionesFileSystem.h"

/* Inicializacion de variables globales */
int estadoFs = ESTABLE;

/* Inicializacion de estructuras administrativas */
t_directory directorios[100];
t_list *nodos;
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

	if (config_has_property(config, "CANTIDAD_NODOS_ESPERADOS")) {
		CANTIDAD_NODOS_ESPERADOS = config_get_int_value(config,
				"CANTIDAD_NODOS_ESPERADOS");
	} else {
		perror(
				"No existe la clave 'CANTIDAD_NODOS_ESPERADOS' en el archivo de configuracion.");
	}

	if (config_has_property(config, "PATH_METADATA")) {
		PATH_METADATA = config_get_string_value(config, "PATH_METADATA");
	} else {
		perror(
				"No existe la clave 'PATH_METADATA' en el archivo de configuracion.");
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
			//Cambiar bloque a ocupado
			bitmap[i] = 'O';
			return i;
			//Cortar For
			break;
		}
	}
	//Si no encontro nada devuelve -1 indicando que esta completo el bitmap;Hay que ver que hacemos ahi del otro lado
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

		//&& (errno!=EINTR))
		if (actividad < 0) {
			fprintf(stderr, "[ERROR]: Fallo la funcion select().");
		}

		// Si algo cambio en el socket master, es una conexion entrante.
		if (FD_ISSET(socketServidor, &readfds)) {
			if ((socketEntrante = accept(socketServidor,
					(struct sockaddr *) &address, (socklen_t*) &addrlen)) < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//printf("Nueva conexion, socket descriptor es: %d , ip es: %s, puerto: %d\n",
			//socketEntrante, inet_ntoa(address.sin_addr), PUERTO);

			// Agrego el nuevo socket al array
			for (i = 0; i < numeroClientes; i++) {
				//Busco una pos vacia en la lista de clientes para guardar el socket entrante
				if (socketsClientes[i] == 0) {
					socketsClientes[i] = socketEntrante;
					//printf("Se agrego el nuevo socket en la posicion: %d.\n",
					//	i);
					break;
				}
			}
		}
		//else  es un cambio en los sockets que estaba escuchando.
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
				if (recibirPorSocket(sd, buffer, header.tamanioPayload) <= 0) {
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
					nodo->bitmap = persistirBitmap(nodo->idNodo,
							nodo->bloquesTotales);
					nodo->ip = infoNodo.ip;

					agregarNodo(nodos, nodo);
					// Tenemos que ver si el hilo de yama entra por aca o ponemos a escuchar en otro hilo aparte
					// SON SOLO PARA DEBUG. COMENTAR Y AGREGAR AL LOGGER.
					// ACTUALIZAR TABLA DE ARCHIVOS Y PASAR A DISPONIBLES LOS
					// BLOQUES DE ESE NODO
				}

				free(buffer);
			}
		}
	}
}

void* serializarSetBloque(void *bloque, uint32_t numBloque) {
	uint32_t *numeroBloque = malloc(sizeof(uint32_t));
	*numeroBloque = numBloque;
	t_header *header = malloc(sizeof(t_header));
	//header->id=malloc(sizeof(uint32_t));
	header->id = 4; // solicitud escribir bloque
	header->tamanioPayload = UN_BLOQUE + sizeof(uint32_t);
	int desplazamiento = 0;
	void *paquete = malloc(sizeof(uint32_t) * 3 + UN_BLOQUE);

	int bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->id, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->tamanioPayload, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, numeroBloque, bytesACopiar);
	desplazamiento += bytesACopiar;
	//copia el bloque al paquete, el tamanio de payload es la suma del bloque y el numero de bloque
	memcpy(paquete + desplazamiento, bloque, UN_BLOQUE);
	free(numeroBloque);
	free(header);
	return paquete;
}

int guardarBloqueEnNodo(uint32_t numeroBloque, void *bloque, int socketNodo) {
	int rta;
	void *paquete = serializarSetBloque(bloque, numeroBloque);
	send(socketNodo, paquete, UN_BLOQUE + sizeof(uint32_t) * 3, 0);
	void *respuesta = malloc(sizeof(uint32_t));

	// No hubo error en el send.
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

int getNodo(int nodoNumero) {
	//Tiene que buscar en la lista de nodos conectados, por ahora trae el unico conectado
	return socketNodoConectado;
}

void serializarHeaderTraerBloque(uint32_t id, uint32_t numBloque, void* paquete) {
	t_header *header = malloc(sizeof(t_header));
	//header->id=malloc(sizeof(uint32_t));
	header->id = id;
	header->tamanioPayload = numBloque;
	int desplazamiento = 0;
	int bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->id, bytesACopiar);
	desplazamiento += bytesACopiar;

	bytesACopiar = sizeof(uint32_t);
	memcpy(paquete + desplazamiento, &header->tamanioPayload, bytesACopiar);
	desplazamiento += bytesACopiar;
	free(header);
}

int traerBloqueNodo(int nodo, uint32_t numBloque, void*bloque) {
	int rta = 0;
	int socketNodo = getNodo(nodo);
	void *paquete = malloc(sizeof(uint32_t) * 2);
	serializarHeaderTraerBloque(3, numBloque, paquete);
	int bytesEnviados = enviarPorSocket(socketNodo, paquete, 0);
	if (bytesEnviados <= 0) {
		printf("Error al enviar peticion al nodo \n");
		rta = 0;
	} else {

		if (recibirPorSocket(socketNodo, bloque, UN_BLOQUE) > 0) {
			rta = 1;
		} else
			rta = 0;
	}
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
	char **pathSeparado = string_split(path, "/");
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
				"mkdir: no se puede crear el directorio «%s»: No existe el directorio padre.\n",
				path);
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
				"mkdir: no se puede crear el directorio «%s»: No existe el directorio padre.\n",
				path);
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

	crearTablaDeDirectorios();
	crearDirectorioArchivos();
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
	nodos = list_create();
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

	// Para probar comando info y rename..(todavia no se recupera de un estado anterior completamente) luego borrar.
	// Hacer la funcion restaurarTablaDeArchivos();
	t_nodo *nodo1 = malloc(sizeof(t_nodo));
	nodo1->idNodo = 1;
	t_nodo *nodo2 = malloc(sizeof(t_nodo));
	nodo2->idNodo = 2;
	t_nodo *nodo3 = malloc(sizeof(t_nodo));
	nodo3->idNodo = 3;

	t_bloque *bloque0 = malloc(sizeof(t_bloque));
	bloque0->numeroBloque = 0;
	bloque0->nodoCopia0 = nodo1;
	bloque0->numeroBloqueCopia0 = 5;
	bloque0->nodoCopia1 = nodo2;
	bloque0->numeroBloqueCopia1 = 2;
	bloque0->bytesOcupados = 1048576;

	t_bloque *bloque1 = malloc(sizeof(t_bloque));
	bloque1->numeroBloque = 1;
	bloque1->nodoCopia0 = nodo1;
	bloque1->numeroBloqueCopia0 = 10;
	bloque1->nodoCopia1 = nodo3;
	bloque1->numeroBloqueCopia1 = 7;
	bloque1->bytesOcupados = 1048500;

	t_archivo_a_persistir *archPrueba = malloc(sizeof(t_archivo_a_persistir));
	archPrueba->tamanio = 3145592;
	archPrueba->indiceDirectorio = 0;
	archPrueba->nombreArchivo = "tux-con-poncho.jpg";
	archPrueba->bloques = list_create();
	list_add(archPrueba->bloques, bloque0);
	list_add(archPrueba->bloques, bloque1);
	list_add(archivos, archPrueba);

	//restaurarTablaDeNodos();
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

	mostrar(4);
	fclose(filePointer);
}

void restaurarTablaDeNodos() {
	int i, largo = 0;
	uint32_t tamanioLibreNodo;
	t_nodo *nodo;
	char *sNodos, **idNodos, *path, *keyNodoLibre;
	path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/nodos.bin");
	// Uso las commons para leer el archivo como un diccionario (key-value).
	t_config* diccionario = config_create(path);

	if (!diccionario) {
		perror("[Error]: No se encontro la tabla de nodos.");
		exit(EXIT_FAILURE);
	}

	nodos = list_create();
	// El tamaño se mide en bloques de 1 MiB
	sNodos = config_get_string_value(diccionario, "NODOS");
	if (!sNodos) {
		puts("[Error]: La tabla de nodos de un estado anterior esta vacia.");
		return;
	}
	printf("NODOS = %s \n", sNodos);

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

		list_add(nodos, nodo);
	}

	puts("Mostrando lista de nodos cargada en memoria...");
	for (i = 0; i < nodos->elements_count; i++) {
		nodo = list_get(nodos, i);
		printf("Id nodo: %d\n Bloques libres: %d\n", nodo->idNodo,
				nodo->bloquesLibres);
	}
}

void agregarNodo(t_list *lista, t_nodo *nodo) {
	if (!lista) {
		fprintf(stderr, "[WARNING]: La lista no esta inicializada.");
	} else {
		list_add(lista, nodo);
	}
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
	t_list *nodosAux = list_create(); // Uso una lista auxiliar para cambiar el orden en que persisto en el archivo.
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

	list_add_all(nodosAux, nodos);
	list_sort(nodosAux, (void*) compararPorIdDesc);

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
	nodo = list_get(nodosAux, 0);
	string_append(&valor, string_itoa(nodo->idNodo));
	for (i = 1; i < nodosAux->elements_count; i++) {
		nodo = list_get(nodosAux, i);
		string_append(&valor, ",");
		string_append(&valor, string_itoa(nodo->idNodo));
	}
	string_append(&valor, "]");
	fputs(clave, filePointer);
	fputs(valor, filePointer);
	free(valor);

	for (i = 0; i < nodosAux->elements_count; i++) {
		nodo = list_get(nodosAux, i);

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
	list_destroy(nodosAux);
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

t_directorio obtenerPathDirectorio(char *path) {
	return string_substring_until(path, lastIndexOf(path, '/'));
}

char* obtenerNombreArchivo(char *path) {
	return string_substring_from(path, lastIndexOf(path, '/') + 1);
}

void renombrarArchivo(char *pathOriginal, char *nombreFinal) {
	char *comando, *nombreInicial;
	t_archivo_a_persistir *archivo;
	t_directorio directorio;
	int indice;

	directorio = obtenerPathDirectorio(pathOriginal);
	indice = obtenerIndice(directorio);
	if (indice == DIR_NO_EXISTE) {
		printf("El directorio '%s' no existe.\n", directorio);
		return;
	}

	nombreInicial = obtenerNombreArchivo(pathOriginal);
	archivo = buscarArchivoPorIndiceYNombre(indice, nombreInicial);
	if (!archivo) {
		printf("El archivo %s no existe.\n", pathOriginal);
		return;
	}

	// Actualizo el nombre del archivo en la lista.
	archivo->nombreArchivo = string_new();
	strcpy(archivo->nombreArchivo, nombreFinal);

	// Actualizo el nombre del archivo correspondiente al directorio.
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
	system(comando);
	free(comando);
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
	for(i = 0; i < 100; i++)
		fwrite(&directorios[i], sizeof(t_directory), 1, archivo);

	// Libero recursos.
	fclose(archivo);
	free(pathDirectorios);
}
