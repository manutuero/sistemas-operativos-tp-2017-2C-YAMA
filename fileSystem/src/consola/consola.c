#include "funcionesConsola.h"

void* levantarConsola() {
	char *linea;
	char **argumentos;
	int cantidadElementos;

	while (TRUE) {
		linea = (char*) readline("> ");
		if (!linea)
			break;

		add_history(linea);
		argumentos = cargarArgumentos(linea);
		cantidadElementos = cantidadArgumentos(argumentos);
		switch (cantidadElementos) {
		case 1:
			if (sonIguales(argumentos[0], "format")) {
				ejecutarFormat(argumentos);
				break;
			}
			if (sonIguales(argumentos[0], "exit")) {
				puts("Que tenga un buen dia");

				if (nodos->elements_count > 0) {
					list_clean_and_destroy_elements(nodos,
							(void*) destruirNodo);
				}

				if (nodosEsperados->elements_count > 0) {
					list_clean_and_destroy_elements(nodosEsperados,
							(void*) destruirNodo);
				}

				exit(EXIT_SUCCESS);
			}

			if (sonIguales(argumentos[0], "clear")) {
				char *comando = "clear";
				system(comando);
				break;
			} else
				printf("el comando ingresado no es valido.\n");
			break;

		case 2:
			if (sonIguales(argumentos[0], "rm")) {
				ejecutarRmArchivo(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "cat")) {
				ejecutarCat(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "mkdir")) {
				ejecutarMkdir(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "md5")) {
				ejecutarMd5(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "ls")) {
				ejecutarLs(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "info")) {
				ejecutarInfo(argumentos);
				break;
			} else {
				printf("El comando %s no es valido.\n", argumentos[0]);
			}

			break;

		case 3:
			if (sonIguales(argumentos[0], "rename")) {
				ejecutarRename(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "rm")) {
				ejecutarRmDirectorio(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "mv")) {
				ejecutarMv(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "cpfrom")) {
				ejecutarCpfrom(argumentos);
				break;
			}

			if (sonIguales(argumentos[0], "cpto")) {
				ejecutarCpto(argumentos);
				break;
			} else {
				printf("El comando '%s' no es valido.\n", argumentos[0]);
				break;
			}

		case 4:
			if (sonIguales(argumentos[0], "cpblock")) {
				ejecutarCpblock(argumentos);
				break;
			} else {
				printf("El comando '%s' no es valido.\n", argumentos[0]);
				break;
			}

		case 5:
			if (sonIguales(argumentos[0], "rm")) {
				ejecutarRmBloque(argumentos);
				break;
			} else {
				printf("El comando '%s' no es valido.\n", argumentos[0]);
				break;
			}

		default: {
			printf("El comando ingresado no es valido.\n");
		}

		}
	}
	return EXIT_SUCCESS;
}
