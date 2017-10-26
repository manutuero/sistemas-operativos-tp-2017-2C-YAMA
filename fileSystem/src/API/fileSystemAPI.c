#include "fileSystemAPI.h"
#include "../funcionesFileSystem.h"

// Poner un mutex
int almacenarArchivo(char *path, char *nombreArchivo, int tipo, FILE *datos) {
	t_list *bloques, *nodosAux = copiarListaNodos(nodos);
	t_bloque *bloque;
	int i, indice, tamanio = 0;
	t_nodo *nodo1, *nodo2;
	char *directorio;
	t_archivo_a_persistir *archivo;

	// Verifica si existe el directorio donde se va a "guardar" el archivo.
	if (!existePathDirectorio(path)) {
		free(nodosAux);
		return ERROR;
	}

	// "Corta" el archivo en bloques segun su tipo y devuelve una lista con esos bloques.
	bloques = obtenerBloques(datos, tipo);

	// Pide los 2 nodos con mayor cantidad de bloques libres para todos los bloques del archivo.
	for (i = 0; i < bloques->elements_count; i++) {
		ordenarListaNodos(nodosAux);
		bloque = list_get(bloques, i);

		nodo1 = list_get(nodosAux, 0);
		nodo2 = list_get(nodosAux, 1);

		bloque->numeroBloqueCopia0 = obtenerYReservarBloqueBitmap(nodo1->bitmap,
				nodo1->bloquesTotales);
		bloque->nodoCopia0 = nodo1;

		bloque->numeroBloqueCopia1 = obtenerYReservarBloqueBitmap(nodo2->bitmap,
				nodo2->bloquesTotales);
		bloque->nodoCopia1 = nodo2;

		nodo1->bloquesLibres--;
		nodo2->bloquesLibres--;

		if (bloque->numeroBloqueCopia0 == ESTA_LLENO
				|| bloque->numeroBloqueCopia1 == ESTA_LLENO) {
			fprintf(stderr,
					"[ERROR]: No se pudo guardar el archivo, no hay bloques disponibles en el nodo.");

			free(nodosAux);
			return ERROR;
		}
	}

	// Envia las solicitudes de almacenado (copia 0 y 1) a los nodos correspondientes.
	for (i = 0; i < bloques->elements_count; i++) {
		bloque = list_get(bloques, i);

		guardarBloqueEnNodo(bloque->numeroBloqueCopia0, bloque->contenido,
				bloque->nodoCopia0->socketDescriptor);

		guardarBloqueEnNodo(bloque->numeroBloqueCopia1, bloque->contenido,
				bloque->nodoCopia1->socketDescriptor);

		tamanio += bloque->bytesOcupados;
	}

	// Si la operacion se realizo correctamente actualizo mi lista de nodos.
	list_clean_and_destroy_elements(nodos, (void*) destruirNodo);
	nodos = copiarListaNodos(nodosAux);
	list_destroy(nodosAux);

	// 3)  Persistir /metadata/nodos.bin y crear /metadata/index/nombreArchivo correspondiente.
	actualizarBitmaps();

	directorio = string_substring_from(strrchr(path, '/'), 1);
	indice = obtenerIndice(directorio);
	archivo = nuevoArchivo(indice, nombreArchivo, tipo, tamanio, bloques);
	crearTablaDeArchivo(archivo);

	return EXITO;
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

bool compararBloquesLibres(t_nodo *unNodo, t_nodo *otroNodo) {
	return otroNodo->bloquesLibres < unNodo->bloquesLibres;
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

		nodo->bitmap[strlen(nodo->bitmap)] = '\0';
		fputs(nodo->bitmap, archivo);

		// Libero recursos.
		free(path);
		free(nodo);
		fclose(archivo);
	}
}

t_archivo_a_persistir* nuevoArchivo(int indiceDirectorio, char *nombreArchivo,
		int tipo, int tamanio, t_list *bloques) {
	t_archivo_a_persistir *archivo = malloc(sizeof(t_archivo_a_persistir));
	archivo->indiceDirectorio = indiceDirectorio;
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

		// BLOQUE COPIA0
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

		// BLOQUE COPIA1
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
	}

	// Cierro recursos.
	fclose(filePointer);
	liberarArchivo(archivo);
}
