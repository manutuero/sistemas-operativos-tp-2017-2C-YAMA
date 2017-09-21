#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "funcionesConsola.h"
#define ESPACIO " "

void levantarConsola(void) {
	char *linea, resultado;
	char **argumentos;
	int cantidadElementos;

	while (1) {
		linea = (char*) readline("> ");
		if (!linea)
			break; // Si recibe NULL corta la ejecucion, evita un segmentation fault.

		add_history(linea);

		argumentos = cargarArgumentos(linea);
		cantidadElementos = cantidadArgumentos(argumentos);

		switch (cantidadElementos) {
		case 1:
			if (sonIguales(argumentos[0], "format")) {
				resultado = invocarFuncionFormat(argumentos);
				break;
			} else
				printf("el comando ingresado no es valido.\n");
			break;

		case 2:
			if (sonIguales(argumentos[0], "rm")) {
				resultado = invocarFuncionRm(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "cat")) {
				resultado = invocarFuncionCat(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "mkdir")) {
				resultado = invocarFuncionMkdir(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "MD5")) {
				resultado = invocarFuncionMd5(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "ls")) {
				resultado = invocarFuncionLs(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "info")) {
				resultado = invocarFuncionInfo(argumentos);
				break;
			} else {
				printf("El comando %s no es valido.\n", argumentos[0]);
			}

			break;

		case 3:
			if (sonIguales(argumentos[0], "rename")) {
				resultado = invocarFuncionRename(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "rm")) {
				resultado = invocarFuncionRmDirectory(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "mv")) {
				resultado = invocarFuncionMv(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "cpfrom")) {
				resultado = invocarFuncionCpfrom(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "cpto")) {
				resultado = invocarFuncionCpto(argumentos);
				break;
			} else {
				printf("El comando %s no es valido.\n", argumentos[0]);
				break;
			}

		case 4:
			if (sonIguales(argumentos[0], "cpblock")) {
				resultado = invocarFuncionCpblok(argumentos);
				break;
			} else {
				printf("El comando %s no es valido.\n", argumentos[0]);
				break;
			}

		case 5:
			if (sonIguales(argumentos[0], "rm")) {
				resultado = invocarFuncionRmBloque(argumentos);
				break;
			} else {
				printf("El comando %s no es valido.\n", argumentos[0]);
				break;
			}

		default: {
			printf("El comando ingresado no es valido.\n");
		}

		}

		if (argumentos[0] != NULL)
			if (strcmp(argumentos[0], "exit") == 0)
				break;
	}

	return;
}
