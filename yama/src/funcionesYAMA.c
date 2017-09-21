/*
 * funcionesYAMA.c
 *
 *  Created on: 20/9/2017
 *      Author: utnso
 */

#include "funcionesYAMA.h"


void conectarseAFS(){

//	struct sockaddr_in direccionFS;

		direccionFS.sin_family = AF_INET;
		direccionFS.sin_port = htons(6667);
		direccionFS.sin_addr.s_addr = inet_addr("127.0.0.1");
		//memset(&(direccionYama.sin_zero), '\0', 8);


		int fdFS;

		fdFS = socket(AF_INET, SOCK_STREAM, 0);

		if(connect(fdFS, (struct sockaddr *)&direccionFS, sizeof(struct sockaddr)) != 0){
				perror("fallo la conexion al fileSystem");
				exit(1);
			}

		printf("conectado al filesystem\n con el socket %d",fdFS);
}


void cargarArchivoDeConfiguracion(){
	//nada
}

void yamaEscuchando(){

	//struct sockaddr_in direccionDelServidorKernel;
	direccionYama.sin_family = AF_INET;
	direccionYama.sin_port = htons(6669);
	direccionYama.sin_addr.s_addr = INADDR_ANY;
	//direccionDelKernel.sin_addr.s_addr = inet_addr(IPKERNEL);
	memset(&(direccionYama.sin_zero), '\0', 8);  // Se setea el resto del array de addr_in en 0

	int activado = 1;

	socketYama = socket(AF_INET, SOCK_STREAM, 0);
	// Permite reutilizar el socket sin que se bloquee por 2 minutos
	if (setsockopt(socketYama, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// Se enlaza el socket al puerto
	if(bind(socketYama, (struct sockaddr *)&direccionYama, sizeof(struct sockaddr))!= 0){
		perror("No se pudo conectar");
		exit(1);
	}
	// Se pone a escuchar el servidor kernel
	if (listen(socketYama, 10) == -1) {
		perror("listen");
		exit(1);
	}
}
