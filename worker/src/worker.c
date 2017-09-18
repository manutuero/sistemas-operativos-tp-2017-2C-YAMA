/*
 ============================================================================
 Name        : worker.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "funcionesWorker.h"

int main(void) {

	struct sockaddr_in dir;

	dir.sin_family = AF_INET;
	dir.sin_port = htons(24000);
	dir.sin_addr.s_addr = INADDR_ANY;
	//memset(&(dir.sin_zero), '\0', 8);

	FILE* fich;
	int cliente;

	cliente = socket(AF_INET, SOCK_STREAM, 0);

	// Olvidémonos del error "Address already in use" [La dirección ya se está usando]
/*
	int yes=1;
		 if (setsockopt(ssocket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		 perror("setsockopt");
		 exit(1);
		 }
*/

	if(connect(cliente, (struct sockaddr *)&dir, sizeof(struct sockaddr)) != 0){
		perror("fallo el connect");
		return 1;
	}
/*
	struct sockaddr_in direccionCliente;
	int tamanioDir = sizeof(struct sockaddr_in);
	int cliente = accept(ssocket, (void*)&direccionCliente, &tamanioDir);
	printf("Entro una conexion por el puerto %d\n", cliente);

	send(cliente, "Hola mundo\n", 11, 0);
*/
	/**********************************************************/

	int bytesRecibidos = recibirArchivo(cliente);
		if(bytesRecibidos < 0){
				perror("Error");
				return 1;
			}


	while(1){
		char mensaje[100];
		scanf("%s", mensaje);

		send(cliente, mensaje, strlen(mensaje), 0);
		/*
		 bytesRecibidos = recibirArchivo(bytesRecibidos, cliente, len, fich);
		if(bytesRecibidos < 0){
			perror("Error");
			return 1;
		}
		*/
	}
	return EXIT_SUCCESS;
}
