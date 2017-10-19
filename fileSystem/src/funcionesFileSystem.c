#include "funcionesFileSystem.h"

/* Inicializacion de variables globales */
int estadoFs = ESTABLE;
char *pathBitmap = "/home/utnso/thePonchos/metadata/bitmaps";

t_directory directorios[100] = { { 0, "root", -1 } };
t_list *nodos;

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
void cargarArchivoBitmap(FILE *archivo, int tamanioDatabin) {
	int i;
	t_bitMap arrayBitmap[tamanioDatabin];
	for (i = 0; i < (tamanioDatabin); i++) {
		arrayBitmap[i].estadoBLoque = 'L';
	}
	for (i = 0; i < (tamanioDatabin); i++) {
		fwrite(&arrayBitmap[i], sizeof(char), 1, archivo);
	}
}

int verificarExistenciaArchBitmap(char *nombreArchBitmap, char *path) {
	DIR *directorio = opendir(path);

	if (!directorio) { // Verifica si el directorio a bitmaps existe.
		closedir(directorio);
		return 2;
	}

	closedir(directorio);

	if (access(nombreArchBitmap, F_OK) != -1) {
		return 1;
	}
	return 0;
}

// Esta funcion crea un archivo bitmap con el nombre id del nodo y
// tamanio de databin,verifica si existe de antes y si no lo crea.
void crearArchivoBitmapNodo(int idNodo, int tamanioDatabinNodo) {
	char*nombreArchBitmap = armarNombreArchBitmap(idNodo);
	FILE* arch;
	switch (verificarExistenciaArchBitmap(nombreArchBitmap, pathBitmap)) {
	case 0:
		arch = fopen(nombreArchBitmap, "w+");
		cargarArchivoBitmap(arch, tamanioDatabinNodo);
		fclose(arch);
		break;
	case 1:
		printf("Archivo bitmap ya existe");
		break;
	case 2:
		printf("La carpeta bitmaps no existe");  // validacion de carpeta bitmap
		break;
	}
}

//Accede al numero de bloque en el array y modifica su estado
void liberarBloqueBitmapNodo(int numBloque, int idNodo) {
	char* nombreArchBitmap = armarNombreArchBitmap(idNodo);
	FILE* arch;
	t_bitMap regActualizado;
	regActualizado.estadoBLoque = 'L';

	arch = fopen(nombreArchBitmap, "r+");
	fseek(arch, (numBloque - 1), SEEK_SET);
	fwrite(&regActualizado, sizeof(t_bitMap), 1, arch);
	fclose(arch);
}

void ocuparBloqueBitmapNodo(int numBloque, int idNodo) {
	char* nombreArchBitmap = armarNombreArchBitmap(idNodo);
	FILE* arch;
	t_bitMap regActualizado;
	regActualizado.estadoBLoque = 'O';

	arch = fopen(nombreArchBitmap, "r+");
	fseek(arch, (numBloque - 1), SEEK_SET);
	fwrite(&regActualizado, sizeof(t_bitMap), 1, arch);
	fclose(arch);
}

// Arma el path nombre del bitmap a partir del id del nodo
char* armarNombreArchBitmap(int idNodo) {
	char* nombreArchBitmap = string_new();
	char* idNodoString = string_itoa(idNodo);
	string_append(&nombreArchBitmap, pathBitmap);
	string_append(&nombreArchBitmap, idNodoString);
	string_append(&nombreArchBitmap, ".dat");

	return nombreArchBitmap;
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
	int socketServidor, numeroClientes = 10, socketsClientes[10] = { }, opt = 1,
			addrlen, max_sd, i, sd, actividad, socketEntrante;
	struct sockaddr_in address;
	fd_set readfds;
	t_header header;
	void *buffer = NULL;

	socketServidor = nuevoSocket();

	// Seteo el socketServidor para permitir multiples conexiones, aunque no sea necesario es una buena practica.
	if (setsockopt(socketServidor, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
			sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	//Defino el tipo de socket con sus parametros.
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PUERTO);

	// Bind al puerto indicado en la variable PUERTO. Cargada de la config O DEL ARCHIVO HEADER
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
		//Limpio la lista de sockets
		FD_ZERO(&readfds);

		//Agrego el socket master a la lista, para que tambien revise si hay cambios
		FD_SET(socketServidor, &readfds);
		max_sd = socketServidor;

		//add child sockets to set
		for (i = 0; i < numeroClientes; i++) {
			//socket descriptor
			sd = socketsClientes[i];

			//if valid socket descriptor then add to read list
			if (sd > 0)
				FD_SET(sd, &readfds);

			//highest file descriptor number, need it for the select function
			if (sd > max_sd)
				max_sd = sd;
		}

		//Espero que haya actividad en los sockets. Tiempo de espera null, nunca termina.
		actividad = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		//&& (errno!=EINTR))
		if (actividad < 0) {
			printf("select error");
		}

		//Si algo cambio en el socket master, es una conexion entrante
		if (FD_ISSET(socketServidor, &readfds)) {
			if ((socketEntrante = accept(socketServidor,
					(struct sockaddr *) &address, (socklen_t*) &addrlen)) < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//inform user of socket number - used in send and receive commands
			//printf(
			//	"Nueva conexion, socket descriptor es: %d , ip es: %s, puerto: %d\n",
			//socketEntrante, inet_ntoa(address.sin_addr), PUERTO);

			//Agrego el nuevo socket al array
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

					// Crear funcion que maneje la lista de struct t_infoNodo. "add" infoNodo por ejemplo.
					//Tenemos que ver si el hilo de yama entra por aca o ponemos a escuchar en otro hilo aparte
					//SON SOLO PARA DEBUG. COMENTAR Y AGREGAR AL LOGGER.
					/*printf("***************************************\n");
					 printf("sdNodo: %d\n", infoNodo.sdNodo);
					 printf("idNodo: %d\n", infoNodo.idNodo);
					 printf("cantidadBloques: %d\n", infoNodo.cantidadBloques);
					 printf("puerto: %d\n", infoNodo.puerto);
					 printf("ip: %s\n", infoNodo.ip);*/
					//ACTUALIZAR TABLA DE ARCHIVOS Y PASAR A DISPONIBLES LOS
					//BLOQUES DE ESE NODO
					socketNodoConectado = sd;
				}
				//printf("Paquete recibido. \n Mensaje: %s usuario: %s \n ",nuevoPacketeRecbido.message,nuevoPacketeRecbido.username);
				free(buffer);
			}
		}
	}

	//puts("Esperando conexiones datanode...");

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

int guardarBloqueEnNodo(int nodo, uint32_t numeroBloque, void *bloque) {
	int rta = 0;
	void *paquete = serializarSetBloque(bloque, numeroBloque);
	//int socketNodo = getNodo(nodo); //saca de la lista de conectados ese nodo.
	int socketNodo = socketNodoConectado; //Se usa para testear .
	//enviarPorSocket(socketNodo, bloque, UN_BLOQUE + sizeof(uint32_t));
	send(socketNodo, paquete, UN_BLOQUE + sizeof(uint32_t) * 3, 0);
	void *respuesta = malloc(sizeof(uint32_t));
	//int recibidos = recv(socketNodo, respuesta, sizeof(uint32_t), MSG_WAITALL);
	int recibidos = recibirPorSocket(socketNodo, respuesta, sizeof(uint32_t));
	if (recibidos > 0) {	//  no hubo error en el send
		if (*(int*) respuesta == 1) {//La respuesta fue 1, el bloque se guardo correctamente.
			printf("Bloque guardado correctamente");
			rta = 1;
		} else {
			printf("algo paso y no se guardo el bloque");
			rta = 0;
		}
	} else {
		rta = 0;
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

void mostrar(t_directory directorios[], int cantidad) {
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
		if ((sonIguales(directorio, directorios[i].nombre)) && (directorios[i].padre == *padre)) {
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

// Borrar ?
int getIndex(char *directorio) {
	int i;
	for (i = 0; i < 100; i++) {
		if (sonIguales(directorio, directorios[i].nombre))
			return directorios[i].index;
	}
	return DIR_NO_EXISTE;
}

bool esDirectorio(char *directorio) {
	return getIndex(directorio) == DIR_NO_EXISTE ? false : true;
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

	if(!sonIguales(pathSeparado[0], "root")) {
		printf("mkdir: no se puede crear el directorio «%s»: No existe el directorio padre.\n", path);
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
		printf("mkdir: no se puede crear el directorio «%s»: El directorio ya existe.\n", path);
		break;

	case 1:
		posicionLibre = buscarPrimerLugarLibre();
		if (posicionLibre != -1) {
			crearDirectorioLogico(pathSeparado[cantidadPartesPath - 1], padre,
					posicionLibre);
			crearDirectorioFisico(posicionLibre);
		} else
			printf("mkdir: no se puede crear el directorio «%s»: La tabla de directorios esta completa.\n", path);
		break;

	default:
		printf("mkdir: no se puede crear el directorio «%s»: No existe el directorio padre.\n", path);
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

	fwrite(directorios, 1, sizeof(t_directory)*100, filePointer);

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

/* Debo ver cuales tablas me conviene cargar en memoria...*/
void cargarEstructurasAdministrativas() {
	cargarTablaDeDirectorios();
	cargarTablaDeNodos();
}

void cargarTablaDeDirectorios() {
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

	mostrar(directorios, 4); // borrar luego de las pruebas..
	fclose(filePointer);
}

void cargarTablaDeNodos() {
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
