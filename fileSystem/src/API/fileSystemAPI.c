#include "fileSystemAPI.h"
#include "../funcionesFileSystem.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char* leerArchivo(char *pathArchivo) {
	int i;
	char *contenido;
	t_list *bloques;
	t_bloque *bloque;
	t_archivo_a_persistir *archivo;
	off_t offset = 0;

	// Verifico su existencia.
	archivo = abrirArchivo(pathArchivo);
	if (!archivo) {
		printf("El archivo '%s' no existe.\n", pathArchivo);
		return NULL;
	}

	// Carga los bloques del archivo con el contenido que solicita a los datanodes.
	bloques = archivo->bloques;
	for (i = 0; i < bloques->elements_count; i++) {
		bloque = list_get(bloques, i);
		bloque->contenido = malloc(UN_BLOQUE);
		limpiar(bloque->contenido, UN_MEGABYTE);
		if (traerBloqueNodo(bloque) != TRAJO_BLOQUE_OK) {
			fprintf(stderr, "[ERROR]: no se pudo traer el bloque n° '%d'.\n",
					bloque->numeroBloque);
			return NULL;
		}
	}

	// Si no hubo fallos procede a serializar el contenido en un solo espacio de memoria.
	contenido = malloc(sizeof(char) * archivo->tamanio);
	for (i = 0; i < bloques->elements_count; i++) {
		bloque = list_get(bloques, i);
		memcpy(contenido + offset, bloque->contenido, bloque->bytesOcupados);
		offset += bloque->bytesOcupados;
	}
	contenido[archivo->tamanio]='\0';
	// Habria que liberar la lista de bloques, el archivo...para que no hayan leaks en cada llamado.
	FILE* fp=fopen("/home/utnso/thePonchos/prueba-recuperada.txt","w");
	fwrite(contenido, sizeof(char),archivo->tamanio, fp);
	fclose(fp);
	return contenido;
}

int almacenarArchivo(char *path, char *nombreArchivo, int tipo, FILE *datos) {
	pthread_mutex_lock(&mutex);
	t_list *bloques, *nodosAux = copiarListaNodos(nodos);
	t_bloque *bloque;
	int i, respuesta, tamanio = 0;
	t_nodo *nodoCopia0, *nodoCopia1;

	// Verifica si existe el directorio donde se va a "guardar" el archivo.
	if (!existePathDirectorio(path)) {
		list_clean_and_destroy_elements(nodosAux, (void*) destruirNodo);
		return ERROR;
	}

	// "Corta" el archivo en bloques segun su tipo y devuelve una lista con esos bloques.
	bloques = obtenerBloques(datos, tipo);

	// Pide los 2 nodos con mayor cantidad de bloques libres para todos los bloques del archivo.
	for (i = 0; i < bloques->elements_count; i++) {
		ordenarListaNodos(nodosAux);
		bloque = list_get(bloques, i);

		nodoCopia0 = list_get(nodosAux, 0);
		nodoCopia1 = list_get(nodosAux, 1);

		bloque->numeroBloqueCopia0 = obtenerYReservarBloqueBitmap(nodoCopia0->bitmap,
				nodoCopia0->bloquesTotales);
		bloque->nodoCopia0 = nodoCopia0;

		bloque->numeroBloqueCopia1 = obtenerYReservarBloqueBitmap(nodoCopia1->bitmap,
				nodoCopia1->bloquesTotales);
		bloque->nodoCopia1 = nodoCopia1;

		nodoCopia0->bloquesLibres--;
		nodoCopia1->bloquesLibres--;

		if (bloque->numeroBloqueCopia0 == ESTA_LLENO
				|| bloque->numeroBloqueCopia1 == ESTA_LLENO) {
			fprintf(stderr,
					"[ERROR]: No se pudo guardar el archivo, no hay bloques disponibles en el nodo.\n");

			free(nodosAux);
			return ERROR;
		}
	}

	// Envia las solicitudes de almacenado (copia 0 y 1) a los nodos correspondientes.
	for (i = 0; i < bloques->elements_count; i++) {
		bloque = list_get(bloques, i);

		respuesta = guardarBloqueEnNodoCopia0(bloque);
		if (validarGuardado(respuesta, bloque, bloque->nodoCopia0)
				!= GUARDO_BLOQUE_OK) {
			return ERROR;
		}

		respuesta = guardarBloqueEnNodoCopia1(bloque);
		if (validarGuardado(respuesta, bloque, bloque->nodoCopia1)
				!= GUARDO_BLOQUE_OK) {
			return ERROR;
		}

		tamanio += bloque->bytesOcupados;
	}

	// Si la operacion se realizo correctamente actualizo mi lista de nodos en memoria.
	list_clean_and_destroy_elements(nodos, (void*) destruirNodo);
	list_add_all(nodos, nodosAux);
	list_destroy(nodosAux);

	// Actualizo los archivos de metadata.
	actualizarBitmaps();
	crearTablaDeArchivo(
			nuevoArchivo(path, nombreArchivo, tipo, tamanio, bloques));
	actualizarTablaDeNodos();
	pthread_mutex_unlock(&mutex);
	return EXITO;
}

int validarGuardado(int respuesta, t_bloque *bloque, t_nodo *nodo) {
	if (respuesta == ERROR_NO_SE_PUDO_GUARDAR_BLOQUE) {
		fprintf(stderr,
				"[ERROR]: no se pudo guardar el bloque n° '%d' en el nodo id '%d'.\n",
				bloque->numeroBloque, nodo->idNodo);
		return ERROR;
	} else if (respuesta == ERROR_AL_RECIBIR_RESPUESTA) {
		fprintf(stderr, "[ERROR]: no se pudo recibir la respuesta.\n");
		return ERROR;
	} else {
		return GUARDO_BLOQUE_OK;
	}
}

/* Funcion que uso para escribir sobre un stream, con la posibilidad de
 * tener una lista variable de argumentos (con formato).
 */
void escribirStreamConFormato(FILE *stream, char *format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stream, format, args); // Similar a fprintf pero con la va_list.
	va_end(args);
}

void liberarBLoque(t_bloque* bloque) {
	free(bloque->nodoCopia0);
	free(bloque->nodoCopia1);
	free(bloque);
}

int proximoRegistro(FILE *datos, char *registro) {
	int largo = 0;
	char caracter = fgetc(datos);
	// Recorro el stream de datos.
	while (caracter != '\n' && !feof(datos)) {
		registro[largo++] = caracter;

		// Los registros son de maximo 1 MB. Considero 1 caracter por el '\n'.
		if (largo > UN_MEGABYTE) {
			puts("[ERROR]: El registro a escribir es mayor a 1 MB.");
			exit(EXIT_FAILURE);
		}
		caracter = fgetc(datos);
	}
	if (caracter == '\n') {
		registro[largo++] = caracter;
	}
	return largo;
}

// Crea un nuevo bloque vacio con el numero de bloque pasado como argumento.
t_bloque* nuevoBloque(uint32_t numeroBloque) {
	t_bloque* bloque = malloc(sizeof(t_bloque));
	bloque->contenido = malloc(UN_BLOQUE);
	limpiar(bloque->contenido, UN_MEGABYTE);
	bloque->bytesOcupados = 0;
	bloque->numeroBloque = numeroBloque;
	return bloque;
}

t_list* obtenerBloques(FILE *datos, int tipo) {
	if (tipo == TEXTO) {
		return parsearArchivoDeTexto(datos);
	} else if (tipo == BINARIO) {
		return parsearArchivoBinario(datos);
	} else {
		perror("[Error]: Se recibio un tipo no valido.");
		return NULL; // Error
	}
}

t_list* parsearArchivoDeTexto(FILE *datos) {
	char *registro = malloc(UN_BLOQUE);
	size_t tamanioRegistro = 0, bytesDisponibles = UN_MEGABYTE;
	uint32_t numeroBloque = 0;
	t_list *bloques = list_create();
	t_bloque *bloque = nuevoBloque(numeroBloque);

	// Agrego el primer bloque a la lista (inicialmente vacio).
	list_add(bloques, bloque);

	while (TRUE) {
		tamanioRegistro = proximoRegistro(datos, registro);

		// Si llego al final del stream (EOF) salgo del bucle.
		if (feof(datos)) {
			memcpy(bloque->contenido + bloque->bytesOcupados, registro,
					tamanioRegistro);

			bloque->bytesOcupados += tamanioRegistro;
			bytesDisponibles -= tamanioRegistro;
			break;
		}

		/* Debo ver si hay lugar en el bloque para guardarlo.
		 * En caso afirmativo, escribe en el bloque y decrementa la cantidad de bytes disponibles. */
		if (tamanioRegistro <= bytesDisponibles) {
			memcpy(bloque->contenido + bloque->bytesOcupados, registro,
					tamanioRegistro);
			bloque->bytesOcupados += tamanioRegistro;
			bytesDisponibles -= tamanioRegistro;
		} else {
			/* Si no hay espacio suficiente en el bloque para que el registro no quede partido,
			 * agrego un nuevo bloque a la lista y escribo en el.
			 */
			numeroBloque++;
			bytesDisponibles = UN_MEGABYTE;
			bloque = nuevoBloque(numeroBloque);
			list_add(bloques, bloque);
			bloque->bytesOcupados = tamanioRegistro; // Lo que voy a escribir en el nuevo bloque
			memcpy(bloque->contenido, registro, tamanioRegistro);
			bytesDisponibles -= tamanioRegistro;
		}
	}

	// Libero recursos
	free(registro);

	return bloques;
}

t_list* parsearArchivoBinario(FILE *datos) {
	int largo = 0;
	char caracter;
	uint32_t numeroBloque = 0;
	t_list *bloques = list_create();
	t_bloque *bloque = nuevoBloque(numeroBloque);
	limpiar(bloque->contenido, UN_MEGABYTE);
	list_add(bloques, bloque);

	while (TRUE) {
		caracter = fgetc(datos);
		// Si llego al final del stream (EOF) salgo del bucle.
		if (feof(datos))
			break;

		if (largo < UN_MEGABYTE) {
			bloque->bytesOcupados++;
			bloque->contenido[largo++] = caracter;
		} else {
			numeroBloque++;
			bloque = nuevoBloque(numeroBloque);
			limpiar(bloque->contenido, UN_MEGABYTE);
			list_add(bloques, bloque);

			largo = 0; // Reinicio el indice
			ungetc(caracter, datos); // Rollback del caracter leido.
		}
	}

	return bloques;
}

void ordenarListaNodos(t_list *listaNodos) {
	list_sort(listaNodos, (void*) compararBloquesLibres);
}

// Si la condicion retorna 'false' intercambia el orden, sino deja ordenado como esta.
bool compararBloquesLibres(t_nodo *unNodo, t_nodo *otroNodo) {
	return otroNodo->bloquesLibres < unNodo->bloquesLibres;
}

bool compararPorIdDesc(t_nodo *unNodo, t_nodo *otroNodo) {
	return unNodo->idNodo < otroNodo->idNodo;
}

void destruirNodo(t_nodo *nodo) {
	free(nodo);
}

void destruirBloque(t_bloque *bloque) {
	free(bloque->contenido);
	free(bloque->nodoCopia0);
	free(bloque->nodoCopia1);
	free(bloque);
}

void limpiar(char* string, size_t largo) {
	memset(string, 0, largo);
}

void actualizarBitmaps() {
	int i;
	char *path;
	t_nodo *nodo;

	/* Actualizo bitmaps */
	for (i = 0; i < nodos->elements_count; i++) {
		nodo = list_get(nodos, i);

		// Actualizo el archivo del nodo que tiene la copia 0 del i-esimo bloque.
		path = string_new();
		string_append(&path, PATH_METADATA);
		string_append(&path, "/bitmaps/");
		string_append(&path, string_itoa(nodo->idNodo));
		string_append(&path, ".dat");
		FILE *archivo = fopen(path, "r+");

		if (!archivo) {
			fprintf(stderr, "[ERROR]: no se encontro el bitmap del nodo %d.\n",
					nodo->idNodo);
			exit(EXIT_FAILURE);
		}

		fputs(nodo->bitmap, archivo);

		// Libero recursos.
		free(path);
		fclose(archivo);
	}
}

t_archivo_a_persistir* nuevoArchivo(char *path, char *nombreArchivo, int tipo,
		int tamanio, t_list *bloques) {
	t_archivo_a_persistir *archivo = malloc(sizeof(t_archivo_a_persistir));
	archivo->indiceDirectorio = obtenerIndice(path);
	archivo->nombreArchivo = string_new();
	string_append(&archivo->nombreArchivo, nombreArchivo);
	archivo->tipo = tipo;
	archivo->tamanio = tamanio;
	archivo->bloques = list_create();
	list_add_all(archivo->bloques, bloques);
	return archivo;
}

void liberarArchivo(t_archivo_a_persistir *archivo) {
	list_destroy_and_destroy_elements(archivo->bloques, (void*) liberarBLoque);
	free(archivo->nombreArchivo);
	free(archivo);
}

void crearTablaDeArchivo(t_archivo_a_persistir *archivo) {
	int i;
	t_bloque *bloque;
	t_list *bloques = archivo->bloques;

	char *clave, *valor, *path = string_new();
	string_append(&path, PATH_METADATA);
	string_append(&path, "/archivos/");
	string_append(&path, string_itoa(archivo->indiceDirectorio));
	string_append(&path, "/");
	string_append(&path, archivo->nombreArchivo);

	FILE *filePointer = fopen(path, "w");
	if (!filePointer) {
		fprintf(stderr, "[ERROR]: no se pudo crear el archivo '%s'.\n", path);
		exit(EXIT_FAILURE);
	}

	// TAMANIO
	clave = "TAMANIO=";
	valor = string_itoa(archivo->tamanio);
	fputs(clave, filePointer);
	fputs(valor, filePointer);

	// TIPO
	clave = "\nTIPO=";
	valor = string_itoa(archivo->tipo);
	fputs(clave, filePointer);
	fputs(valor, filePointer);

	// BLOQUES
	for (i = 0; i < bloques->elements_count; i++) {
		bloque = list_get(archivo->bloques, i);

		// BLOQUExCOPIA0
		clave = string_new();
		string_append(&clave, "\nBLOQUE");
		string_append(&clave, string_itoa(bloque->numeroBloque));
		string_append(&clave, "COPIA0=");
		valor = string_new();
		string_append(&valor, "[");
		string_append(&valor, string_itoa(bloque->nodoCopia0->idNodo));
		string_append(&valor, ",");
		string_append(&valor, string_itoa(bloque->numeroBloqueCopia0));
		string_append(&valor, "]");
		fputs(clave, filePointer);
		fputs(valor, filePointer);
		free(clave);
		free(valor);

		// BLOQUExCOPIA1
		clave = string_new();
		string_append(&clave, "\nBLOQUE");
		string_append(&clave, string_itoa(bloque->numeroBloque));
		string_append(&clave, "COPIA1=");
		valor = string_new();
		string_append(&valor, "[");
		string_append(&valor, string_itoa(bloque->nodoCopia1->idNodo));
		string_append(&valor, ",");
		string_append(&valor, string_itoa(bloque->numeroBloqueCopia1));
		string_append(&valor, "]");
		fputs(clave, filePointer);
		fputs(valor, filePointer);
		free(clave);
		free(valor);

		// BLOQUExBYTES
		clave = string_new();
		string_append(&clave, "\nBLOQUE");
		string_append(&clave, string_itoa(bloque->numeroBloque));
		string_append(&clave, "BYTES=");
		valor = string_itoa(bloque->bytesOcupados);
		fputs(clave, filePointer);
		fputs(valor, filePointer);
		free(clave);
	}

	// Cierro recursos.
	fclose(filePointer);
}
