#include "funcionesConsola.h"

bool esValido(char *path) {
	return string_starts_with(path, "/root") ? true : false;
}

int esNumero(char* valor) {
	int i = 0;
	while (valor[i] != '\0') {
		if (!isdigit(valor[i])) {
			return 0;
		} else
			i++;
	}
	return 1;
}

int cantidadArgumentos(char** argumentos) {
	int cantidad = 0, i = 0;
	char *palabra;

	palabra = argumentos[i];

	while (palabra != NULL) {
		cantidad++;
		i++;
		palabra = argumentos[i];
	}

	return cantidad;
}

char** cargarArgumentos(char* linea) {
	string_trim(&linea);
	return (string_split(linea, ESPACIO));
}

void ejecutarFormat(char **argumentos) {
	persistirTablaDeNodos();

	// ---- Borrar luego
	char *path = "/home/utnso/thePonchos/tux-con-poncho.jpg";
	FILE *datos = fopen(path, "r");
	if (!datos)
		fprintf(stderr, "\nEl archivo '%s' no existe.", path);

	almacenarArchivo("/root", "tux-con-poncho.jpg", TEXTO, datos);
	fclose(datos);
	// ----
}

char* invocarFuncionRm(char **argumentos) {
	if (esValido(argumentos[1])) {
		printf("Funcion de remove archivo estandar.\n");
		printf("El archivo a remover es: %s.\n", argumentos[1]);
	} else {
		printf("El parametro ingresado '%s' no es valido.\n", argumentos[1]);
	}

	return "<default>";
}

char* invocarFuncionRmDirectory(char **argumentos) {
	//revisar que directorio esta vacio primero

	if ((strcmp(argumentos[1], "-d") == 0) && (esValido(argumentos[2]))) {

		printf("Funcion de remove directorio.\n");
		printf("El directorio a remover es: %s.\n", argumentos[2]);
	} else
		printf(
				"La opcion: %s no es valida o el parametro: %s no es correcto.\n",
				argumentos[1], argumentos[2]);

	return "<default>";
}

char* invocarFuncionRmBloque(char **argumentos) {
	//revisar que no es la ultima copia del bloque
	if ((strcmp(argumentos[1], "-b") == 0) && (esNumero(argumentos[3]))
			&& (esNumero(argumentos[4]))) {

		printf("Funcion de remover un bloque.\n");
		printf("Se removera del archivo: %s.\n", argumentos[2]);
		printf("El bloque: %s de la copia: %s.\n", argumentos[3],
				argumentos[4]);
	} else
		printf(
				"La opcion: %s no es valida o uno de los parametros: %s,%s no es correcto.\n",
				argumentos[1], argumentos[2], argumentos[3]);

	return "<default>";
}

char* invocarFuncionCat(char **argumentos) {
	if (esValido(argumentos[1])) {
		printf("Funcion de concatenar texto plano.\n");
		printf("el archivo es: %s.\n", argumentos[1]);
	} else
		printf("El parametro ingresado: %s no es valido.\n", argumentos[1]);
	return "<default>";
}

void ejecutarMkdir(char **argumentos) {
	char *path = argumentos[1];
	if (esValido(path) && strlen(path) > 1) {
		mkdirFs(path);
	} else
		printf(
				"mkdir: no se puede crear el directorio «%s»: La ruta ingresada no es valida.\n",
				path);
}

char* invocarFuncionMd5(char **argumentos) {

	if (esValido(argumentos[1])) {
		printf("funcion de md5.\n");
		printf("El archivo es: %s.\n", argumentos[1]);
	} else
		printf("El parametro ingresado: %s no es valido.\n", argumentos[1]);
	return "<default>";
}

void ejecutarLs(char **argumentos) {
	t_directorio directorio = argumentos[1];

	if (esValido(directorio)) {
		if (!existePathDirectorio(directorio)) {
			printf("El directorio '%s' no existe.\n", directorio);
			free(directorio);
			return;
		}

		int indice = obtenerIndice(directorio);
		char *comando = string_new();
		string_append(&comando, "ls ");
		string_append(&comando, PATH_METADATA);
		string_append(&comando, "/archivos/");
		string_append(&comando, string_itoa(indice));
		system(comando);
		free(comando);
	} else {
		printf(
				"El parametro ingresado '%s' no es valido. Debe ingresar una ruta.\n",
				directorio);
	}
	free(directorio);
}

void ejecutarInfo(char **argumentos) {
	int i;
	t_bloque *bloque;
	char *pathArchivo = argumentos[1];

	if (esValido(pathArchivo)) {
		t_archivo_a_persistir *archivo = obtenerArchivo(pathArchivo);
		if (!archivo) {
			printf("El archivo '%s' no existe.\n", pathArchivo);
			return;
		}

		// Header
		puts(
				" ---------------------------------------------------------------------------------------");
		printf("|				Info '%s'				|\n", archivo->nombreArchivo);
		puts(
				" ---------------------------------------------------------------------------------------\n");

		// Body
		printf("Tamaño: %d bytes.\n", archivo->tamanio);
		if (archivo->tipo == BINARIO) {
			printf("Tipo: binario.\n");
		} else {
			printf("Tipo: de texto.\n");
		}
		printf("Directorio padre: %d.\n", archivo->indiceDirectorio);
		printf("Cantidad de bloques: %d.\n", archivo->bloques->elements_count);
		printf(
				"\n ----------------------------------------------------------------------------------------------------");
		puts(
				"\n|     Bloque    |           Copia0              |             Copia1            |     Fin de bloque  |");
		printf(
				" ----------------------------------------------------------------------------------------------------");
		for (i = 0; i < archivo->bloques->elements_count; i++) {
			bloque = list_get(archivo->bloques, i);
			printf("\n|	%d	", bloque->numeroBloque);
			printf("|	Nodo %d - Bloque %d	", bloque->nodoCopia0->idNodo,
					bloque->numeroBloqueCopia0);
			printf("|	Nodo %d - Bloque %d	", bloque->nodoCopia1->idNodo,
					bloque->numeroBloqueCopia1);
			printf("|	%d      |\n", bloque->bytesOcupados);
			printf(
					" ---------------------------------------------------------------------------------------------------- ");
		}
		printf("\n");
	} else
		printf(
				"El parametro ingresado '%s' no es valido. Debe ser un ruta a archivo valida.\n",
				pathArchivo);
}

void ejecutarRename(char **argumentos) {
	char *pathOriginal, *nombreFinal;

	pathOriginal = argumentos[1];
	if (!esValido(pathOriginal)) {
		printf("La ruta '%s' no es valida.\n", pathOriginal);
		return;
	}

	nombreFinal = argumentos[2];
	if (string_starts_with(nombreFinal, "/")) {
		printf("El nombre '%s' no es valido.\n", nombreFinal);
		return;
	}

	// Determina si lo que se desea renombrar es un archivo o un directorio.
	if (existePathDirectorio(pathOriginal)) {
		renombrarDirectorio(pathOriginal, nombreFinal);
	} else {
		renombrarArchivo(pathOriginal, nombreFinal);
	}
}

char* invocarFuncionMv(char **argumentos) {
	if (esValido(argumentos[1]) && esValido(argumentos[2])) {
		printf("funcion move\n");
		printf("La ruta original era: %s\n", argumentos[1]);
		printf("La nueva ruta es: %s\n", argumentos[2]);
	} else
		printf("Los parametros ingresados: %s,%s no son validos\n",
				argumentos[1], argumentos[2]);
	return "<default>";
}

char* invocarFuncionCpfrom(char **argumentos) {
	if (esValido(argumentos[1]) && esValido(argumentos[2])) {
		printf("funcion cpfrom\n");
		printf("Los argumentos son: %s y %s\n", argumentos[1], argumentos[2]);
	} else
		printf("Los parametros ingresados: %s,%s no son validos\n",
				argumentos[1], argumentos[2]);
	return "<default>";
}

char* invocarFuncionCpto(char **argumentos) {
	if (esValido(argumentos[1]) && esValido(argumentos[2])) {
		printf("funcion cpto\n");
		printf("Los argumentos son: %s y %s\n", argumentos[1], argumentos[2]);
	} else
		printf("Los parametros ingresados: %s,%s no son validos\n",
				argumentos[1], argumentos[2]);

	return "<default>";
}

char* invocarFuncionCpblok(char **argumentos) {
	if ((esNumero(argumentos[2])) && (esNumero(argumentos[3]))
			&& (esValido(argumentos[1]))) {
		printf("funcion cpblock\n");
		printf("El archivo al que pertenece el bloque: %s\n", argumentos[1]);
		printf("El numero de bloque es: %s\n", argumentos[2]);
		printf("El nodo en el que copiar es: %s\n", argumentos[3]);
	} else
		printf("Los parametros ingresados: %s,%s,%s no son validos\n",
				argumentos[1], argumentos[2], argumentos[3]);

	return "<default>";
}
