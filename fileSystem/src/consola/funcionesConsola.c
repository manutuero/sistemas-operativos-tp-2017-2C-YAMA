#include "funcionesConsola.h"

void inicializarComando(t_comando *comando) {
	comando->funcion = 0;
	comando->bloque = 0;
	comando->idNodo = 0;
	comando->opcion = 0;
	comando->parametro1 = "";
	comando->parametro2 = "";
}

int validarParametro(char *parametro) {
	if ((string_starts_with(parametro, "/")))
		return 1;
	else
		return 0;
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

char* invocarFuncionFormat(char **argumentos) {
	printf("Funcion de format.\n");
	char *path = "/home/utnso/thePonchos/prueba.txt";
	FILE *datos = fopen(path, "r");
	if (!datos)
		  fprintf(stderr, "\nEl archivo '%s' no existe.", path);

	almacenarArchivo("/root", "asdadas", TEXTO, datos);

	fclose(datos);
	return "";
}

char* invocarFuncionRm(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	if (validarParametro(argumentos[1]) == 1) {
		comando.funcion = 2;
		comando.parametro1 = argumentos[1];

		printf("Funcion de remove archivo estandar.\n");
		printf("El archivo a remover es: %s.\n", argumentos[1]);
	} else {
		printf("El parametro ingresado '%s' no es valido.\n", argumentos[1]);
	}

	return "<default>";
}

char* invocarFuncionRmDirectory(char **argumentos) {
	//revisar que directorio esta vacio primero
	t_comando comando;
	inicializarComando(&comando);

	if ((strcmp(argumentos[1], "-d") == 0)
			&& ((validarParametro(argumentos[2])) == 1)) {
		comando.funcion = 3;
		comando.opcion = 1;
		comando.parametro1 = argumentos[2];

		printf("Funcion de remove directorio.\n");
		printf("El directorio a remover es: %s.\n", argumentos[2]);
	} else
		printf(
				"La opcion: %s no es valida o el parametro: %s no es correcto.\n",
				argumentos[1], argumentos[2]);

	return "<default>";
}

char* invocarFuncionRmBloque(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	//revisar que no es la ultima copia del bloque
	if ((strcmp(argumentos[1], "-b") == 0) && (esNumero(argumentos[3]))
			&& (esNumero(argumentos[4]))) {
		comando.funcion = 4;
		comando.opcion = 2;
		comando.bloque = (int) argumentos[3];
		comando.idNodo = (int) argumentos[4];

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
	t_comando comando;
	inicializarComando(&comando);

	if (validarParametro(argumentos[1]) == 1) {
		comando.funcion = 5;
		comando.parametro1 = argumentos[1];

		printf("Funcion de concatenar texto plano.\n");
		printf("el archivo es: %s.\n", argumentos[1]);
	} else
		printf("El parametro ingresado: %s no es valido.\n", argumentos[1]);
	return "<default>";
}

void ejecutarMkdir(char **argumentos) {
	char *path = argumentos[1];
	if (validarParametro(path) && strlen(path) > 1) {
		mkdirFs(path);
	} else
		printf("mkdir: no se puede crear el directorio «%s»: La ruta ingresada no es valida.\n", path);
}

char* invocarFuncionMd5(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	if (validarParametro(argumentos[1])) {
		comando.funcion = 7;
		comando.parametro1 = argumentos[1];

		printf("funcion de md5.\n");
		printf("El archivo es: %s.\n", argumentos[1]);
	} else
		printf("El parametro ingresado: %s no es valido.\n", argumentos[1]);
	return "<default>";
}

char* invocarFuncionLs(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	if (validarParametro(argumentos[1])) {
		comando.funcion = 8;
		comando.parametro1 = argumentos[1];

		printf("funcion ls\n");
		printf("El directorio a listar es: %s.\n", argumentos[1]);
	} else
		printf("El parametro ingresado: %s no es valido.\n", argumentos[1]);
	return "<default>";
}

char* invocarFuncionInfo(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	if (validarParametro(argumentos[1])) {
		comando.funcion = 9;
		comando.parametro1 = argumentos[1];

		printf("Funcion info.\n");
		printf("El archivo es: %s.\n", argumentos[1]);
	} else
		printf("El parametro ingresado: %s no es valido.\n", argumentos[1]);
	return "<default>";
}

char* invocarFuncionRename(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	if (validarParametro(argumentos[1]) && validarParametro(argumentos[2])) {
		comando.funcion = 10;
		comando.parametro1 = argumentos[1];
		comando.parametro2 = argumentos[2];

		printf("Funcion rename.\n");
		printf("El nombre original era: %s.\n", argumentos[1]);
		printf("El nuevo nombre es: %s.\n", argumentos[2]);
	} else
		printf("Los parametros ingresados: %s,%s no son validos\n",
				argumentos[1], argumentos[2]);
	return "<default>";
}

char* invocarFuncionMv(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	if (validarParametro(argumentos[1]) && validarParametro(argumentos[2])) {
		comando.funcion = 11;
		comando.parametro1 = argumentos[1];
		comando.parametro2 = argumentos[2];

		printf("funcion move\n");
		printf("La ruta original era: %s\n", argumentos[1]);
		printf("La nueva ruta es: %s\n", argumentos[2]);
	} else
		printf("Los parametros ingresados: %s,%s no son validos\n",
				argumentos[1], argumentos[2]);
	return "<default>";
}

char* invocarFuncionCpfrom(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	if (validarParametro(argumentos[1]) && validarParametro(argumentos[2])) {
		comando.funcion = 12;
		comando.parametro1 = argumentos[1];
		comando.parametro2 = argumentos[2];

		printf("funcion cpfrom\n");
		printf("Los argumentos son: %s y %s\n", argumentos[1], argumentos[2]);
	} else
		printf("Los parametros ingresados: %s,%s no son validos\n",
				argumentos[1], argumentos[2]);
	return "<default>";
}

char* invocarFuncionCpto(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	if (validarParametro(argumentos[1]) && validarParametro(argumentos[2])) {
		comando.funcion = 13;
		comando.parametro1 = argumentos[1];
		comando.parametro2 = argumentos[2];

		printf("funcion cpto\n");
		printf("Los argumentos son: %s y %s\n", argumentos[1], argumentos[2]);
	} else
		printf("Los parametros ingresados: %s,%s no son validos\n",
				argumentos[1], argumentos[2]);

	return "<default>";
}

char* invocarFuncionCpblok(char **argumentos) {
	t_comando comando;
	inicializarComando(&comando);

	if ((esNumero(argumentos[2])) && (esNumero(argumentos[3]))
			&& (validarParametro(argumentos[1]))) {
		comando.funcion = 14;
		comando.parametro1 = argumentos[1];
		comando.bloque = (int) argumentos[2];
		comando.idNodo = (int) argumentos[3];

		printf("funcion cpblock\n");
		printf("El archivo al que pertenece el bloque: %s\n", argumentos[1]);
		printf("El numero de bloque es: %s\n", argumentos[2]);
		printf("El nodo en el que copiar es: %s\n", argumentos[3]);
	} else
		printf("Los parametros ingresados: %s,%s,%s no son validos\n",
				argumentos[1], argumentos[2], argumentos[3]);

	return "<default>";
}
