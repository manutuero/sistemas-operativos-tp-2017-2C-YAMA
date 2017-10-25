#include "fileSystemAPI.h"
#include "../funcionesFileSystem.h"

// Poner un mutex
int almacenarArchivo(char *path, char *nombreArchivo, int tipo, FILE *datos) {
	t_list *bloques, *nodosAux = copiarListaNodos(nodos);
	t_bloque *bloque;
	int i;
	t_nodo *nodo1, *nodo2;

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
	}

	// Si la operacion se realizo correctamente actualizo (reapunto) mi lista de nodos.
	free(nodos);
	nodos = nodosAux;

	// 3)  Persistir /metadata/nodos.bin y crear /metadata/index/nombreArchivo correspondiente.
	actualizarBitmaps();

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
	free(bloque->contenido);
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
	free(nodo->bitmap);
	free(nodo);
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

		fwrite(nodo->bitmap, sizeof(char), strlen(nodo->bitmap)-1, archivo);

		// Libero recursos.
		free(path);
		free(nodo);
		fclose(archivo);
	}

	/* Creo tabla de archivo en el indice del directorio 'path' */
	/*	char *directorio;
	 directorio	= string_substring_from(strrchr(path, '/'), 1);
	 int indice = obtenerIndice(directorio);

	 printf("Directorio: %s\n", directorio);
	 printf("Indice directorio: %d\n", indice);*/

	// no olvidar liberar char* pathBitmap, path y directorio!
}
