/*
 * funcionesMaster.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */


#include "funcionesMaster.h"
//#include "../../fileSystem/src/utils/utils.h"

int chequearParametros(char *transformador,char *reductor,char *archivoAprocesar,char *direccionDeResultado){


	char * comienzo ="yamafs:/";
		if(string_starts_with(archivoAprocesar,comienzo)<=0){
			printf("Parametro archivo a procesar invalido.: %s \n",archivoAprocesar);
			return 0;
		}
		if(string_starts_with(direccionDeResultado,comienzo)<=0){
			printf("La direccion de guardado de resultado es invalida: %s \n",direccionDeResultado);
			return 0;
		}

		if(!file_exists(transformador)){
			printf("El programa transformador no se encuentra en : %s  \n",transformador);
			return 0;
		}
		if(!file_exists(reductor)){
			printf("El programa reductor no se encuentra en : %s  \n",reductor);
			return 0;
			}


	return 1;

}
//Chequea existencia de archivo en linux
int file_exists (char * fileName)
{
   struct stat buf;
   int i = stat ( fileName, &buf );
     /* File found*/
     if ( i == 0 )
     {
       return 1;
     }
     return 0;

}

void masterEscuchando(int* socketMaster) {

	struct sockaddr_in dir;
	//configurarAddr(&dir);
	dir.sin_family = AF_INET;
	dir.sin_port = htons(24000);
	dir.sin_addr.s_addr = INADDR_ANY;
	memset(&(dir.sin_zero), '\0', 8);

	*socketMaster = socket(AF_INET, SOCK_STREAM, 0);
	// Olvidémonos del error "Address already in use" [La dirección ya se está usando]
	int opt = 1;
	if (setsockopt(*socketMaster, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int))
			== -1) {
		perror("setsockopt");
		exit(1);
	}
	if (bind(*socketMaster, (struct sockaddr*) &dir, sizeof(struct sockaddr))
			!= 0) {
		perror("fallo el bind");
		exit(1);
	}
	listen(*socketMaster, 100);
}

//FUNCION CON EL SELECT PARA LOS WORKERS
//TODO se va a cambiar por hilos mas adelante
void iniciarMaster(char* transformador,char* reductor,char* archivoAprocesar,char* direccionDeResultado){
	//struct sockaddr_in dir;
	int socketMaster;
	int nuevoSocket , max_clients = 5 , i ;
	int maxPuerto;
	fd_set readfds, auxRead;
	int socketYama;



	//socketYama = conectarseAYama(6669,"127.0.0.1");

	//mandarRutaArchivoAYama(socketYama, archivoAprocesar);

	masterEscuchando(&socketMaster);

	//inicializar todos los socketCliente
	    for (i = 0; i < max_clients; i++)
	    {
	        //socketCliente[i] = 0;
	    }

	/********************************************************/

	struct sockaddr_in direccionCliente;
	int tamanioDir;

	char *buffer;
	int bytesRecibidos;

	FD_ZERO(&readfds);
	FD_ZERO(&auxRead);
	FD_SET(socketMaster, &auxRead);

	maxPuerto = socketMaster;
	tamanioDir = sizeof(struct sockaddr_in);

	//-----------------------

	//Entro al select de conexiones
	printf("Esperando conexiones\n");
	while(1){
		readfds = auxRead;
		if (select(maxPuerto+1, &readfds, NULL, NULL, NULL) == -1) {
					perror("select");
					exit(1);
				}

		for ( i = 0 ; i <= maxPuerto ; i++)
		{
		    if(FD_ISSET(i, &readfds)){
		      if(i == socketMaster){

				if((nuevoSocket = accept(socketMaster, (void*)&direccionCliente, &tamanioDir)) <= 0) perror("accept");
				else{
					//Le envia el archivo apenas se conecta con un puerto
					printf("Entro una conexion por el puerto %d\n", nuevoSocket);
					FD_SET(nuevoSocket,&auxRead);

					//TODO
					/*-------Envio archivo-------------------------
					 *Actualmente manda el primer parametro, en este caso el tranformador.
					 * Seguramente va a haber que cambiarlo
					 */
					enviarArchivo(nuevoSocket, buffer, transformador);
					if(nuevoSocket > maxPuerto)
						maxPuerto = nuevoSocket;
				}
		      }
		      else{

				//bytesRecibidos = recibirPorSocket(i,buffer,1000);
				if(bytesRecibidos < 0){
					perror("Error");
					exit(1);
				}
				if(bytesRecibidos == 0){
					//printf("Se desconecto del fileSystem el socket %d", i);
					 FD_CLR(i, &readfds);
					 shutdown(i, 2);
				}else{
					 buffer[bytesRecibidos] = '\0';
					 printf("Socket: %d -- BytesRecibidos: %d -- Buffer recibido : %s\n",i, bytesRecibidos , buffer);

				}
				free(buffer);
		      }
		    }
		}
	}

}


int conectarseAYama(int puerto,char* ip){
	struct sockaddr_in direccionYama;

	direccionYama.sin_family = AF_INET;
	direccionYama.sin_port = htons(puerto);
	direccionYama.sin_addr.s_addr = inet_addr(ip);
	//memset(&(direccionYama.sin_zero), '\0', 8);


	int yama;

	yama = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(yama, (struct sockaddr *)&direccionYama, sizeof(struct sockaddr)) != 0){
			perror("fallo la conexion a YAMA");
			exit(1);
		}


	printf("se conecto a YAMA\n");

	return yama;
}

void mandarRutaArchivoAYama(int socketYama, char* archivoAprocesar){
	t_rutaArchivo ruta;
	t_header header;

	ruta.tamanio = strlen(archivoAprocesar)+1;
	ruta.ruta = archivoAprocesar;
	//int tamanioMensaje = header.tamanio + sizeof(header);

	void* buffer;
	buffer = serializarRutaArchivo(&header,&ruta); //esta en utils ya que lo voy a usar para Yama-fs
	int tamanioMensaje = header.tamanio + sizeof(header);
	enviarPorSocket(socketYama,buffer,tamanioMensaje);
	free(buffer);
}
