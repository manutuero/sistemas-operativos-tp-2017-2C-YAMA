#include "fileSystemAPI.h"

int almacenarArchivo(char* path, char* nombreArchivo, int tipo, FILE *datos) {
	int resultado = 0;

	if (tipo == TEXTO) {
		resultado = parsearArchivoDeTexto(datos);
	} else if (tipo == BINARIO) {
		resultado = parsearArchivoBinario(datos);
	} else {
		puts("[ERROR]: El tipo de archivo no es valido.");
		exit(EXIT_FAILURE);
	}

	return resultado;
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

int parsearArchivoDeTexto(FILE *datos) {
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
		if (feof(datos))
			break;

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

	/*// Mostrar bloques
	printf("Cantidad de bloques: %d\n\n", bloques->elements_count);
	int x;
	for (x = 0; x < bloques->elements_count; x++) {
		bloque = list_get(bloques, x);
		printf("Numero bloque: %d\n", bloque->numeroBloque);
		printf("Bytes ocupados: %d\n", bloque->bytesOcupados);
		printf("%s", bloque->contenido);
	}*/

	// Libero recursos
	free(registro);
	list_destroy_and_destroy_elements(bloques, (void*) liberarBLoque); // NO HAY QUE DESTRUIR LA LISTA, ES SOLO PARA EJEMPLO
	return EXIT_SUCCESS;
}

int parsearArchivoBinario(FILE *datos) {
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

	/*// Mostrar bloques
	printf("Cantidad de bloques: %d\n", bloques->elements_count);
	int x;
	for (x = 0; x < bloques->elements_count; x++) {
		printf("Bloque n° %d\n", x);
		bloque = list_get(bloques, x);
		printf("Bytes ocupados: %d	contenido: %s", bloque->bytesOcupados,
				bloque->contenido);
	}*/

	// Libero recursos
	list_destroy_and_destroy_elements(bloques, (void*) liberarBLoque); // NO HAY QUE DESTRUIR LA LISTA, ES SOLO PARA EJEMPLO
	return EXIT_SUCCESS;
}

void limpiar(char* string, size_t largo) {
	memset(string, 0, largo);
}
