#include "fileSystemAPI.h"
#include "../funcionesFileSystem.h"

int almacenarArchivo(char *path, char *nombreArchivo, int tipo, FILE *datos) {
	t_list *bloques;
	t_bloque *bloque;
	int idNodo;
	int numeroBloqueDatanode;
	int numBloqueDatanodeCopia;
	int i;
	t_nodo *nodo1;
	t_nodo *nodo2;

	// 1) Validar si existe el directorio en yamafs:, pedir index para crear el archivo en linux.
	if (!existePathDirectorio(path))
		return ERROR;

	// 2) A partir de la cantidad de bloques. Ver de la lista de nodos el que menos carga tenga (por bloque).
	bloques = obtenerBloques(datos, tipo);
	//ACA deberiamos hacer una copia de la lista nodos. Trabajar y cuando terminamos y esta todo OK la reemplazamos en la variable global.
	for(i = 0; i < bloques->elements_count; i++) {
		ordenarListaNodos();
		bloque = list_get(bloques, i);

		//Tomo los dos nodos donde voy a guardar. Como la lista de nodos esta ordenada. Se que los dos primeros son los que mas espacio libre tiene.
		//ATENCION, aca tambien deberiamos filtrar los nodos que NO tienen espacio.

		nodo1=list_get(nodos,0);
		nodo2=list_get(nodos,1);
		numeroBloqueDatanode=obtenerYReservarBloqueBitmap(nodo1->bitmap,nodo1->bloquesTotales);
	    numBloqueDatanodeCopia=obtenerYReservarBloqueBitmap(nodo2->bitmap,nodo2->bloquesTotales);
	    guardarBloqueEnNodo(nodo1->idNodo,numeroBloqueDatanode,bloque->contenido,nodo1->socketDescriptor);
	    guardarBloqueEnNodo(nodo2->idNodo,numBloqueDatanodeCopia,bloque->contenido,nodo2->socketDescriptor);
		//traer un numero de bloque del primer nodo de la lista. Aca se guarda el bloque original
		//traer un numero de bloque del segundo nodo de la lista. aca se guarda la copia del bloque original
	}
	// 3) Ver el bitmap para obtener el primer bloque disponible para ese nodo.
	// 4) Enviar solicitudes de setBloque con el n° de bloque que me dio bitmap de esos nodos.

	return EXITO; // Resultado de operacion de escritura.
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

void ordenarListaNodos() {
	list_sort(nodos, (void*)compararBloquesLibres);
}

bool compararBloquesLibres(t_nodo *unNodo, t_nodo *otroNodo) {
	// Si el primero es mayor que el segundo, su diferencia es positiva.
	return unNodo->bloquesLibres - otroNodo->bloquesLibres  >= 0 ? true : false;
}

void limpiar(char* string, size_t largo) {
	memset(string, 0, largo);
}

