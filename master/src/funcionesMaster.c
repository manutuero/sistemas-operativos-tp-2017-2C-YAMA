/*
 * funcionesMaster.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */


#include "funcionesMaster.h"

void enviarArchivo(int fd, FILE* fich, char* buffer, long * tam, char* archivo) {

	fich = fopen(archivo, "r");
	//fich = fopen("/home/utnso/Escritorio/archivo", "rb");
	if (!fich) {
		printf("Error al abrir el archivo\n");
		exit(1);
	}
	buffer = (char*) malloc(*tam * sizeof(char) + 1);

	printf("Mandando el archivo... \n");
	while (!feof(fich)) { //leo el archivo y guardo el el contenido en el buffer
		fread(buffer, sizeof(char), *tam, fich);
	}
	buffer[*tam]='\0'; //Cierro el buffer
	serializarYEnviarArchivo(fd,*tam, buffer);


	printf("Se mando el archivo al worker \n");
	rewind(fich);
	fclose(fich);
	free(buffer);
}

//Calculo el tamanio del archivo que voy a mandar
long calcularTamanioArchivo(FILE* fich, char* archivo) {
	fich = fopen(archivo, "r");
	//fich = fopen("/home/utnso/Escritorio/archivo", "r");
	if(!fich){
		printf("No existe el archivo en la direccion indicada de apertura\n");
		exit(1);
	}
	fseek(fich, 0L, SEEK_END);
	int tam = ftell(fich); //Te dice los bytes desplazados desde el inicio del archivo
	rewind(fich);
	fclose(fich);

	printf("tengo un archivo de %d bytes para mandar\n", tam);
	return tam;
}

void serializarYEnviarArchivo(int fd, int tamanio, char* contenido){
	myHeader header;
	int desplazamiento = 0;
	void* archivoAMandar = serializarArchivo(tamanio,contenido,&header);
	int tamanioTotal = sizeof(myHeader) + header.tamanio;

	void* buffer = malloc(tamanioTotal);

	memcpy(buffer + desplazamiento,&header.id, sizeof(header.id));
	desplazamiento+=sizeof(header.id);

	memcpy(buffer + desplazamiento,&header.tamanio, sizeof(header.tamanio));
	desplazamiento+=sizeof(header.tamanio);

	memcpy(buffer + desplazamiento,archivoAMandar, header.tamanio);
	enviarPorSocket(fd, buffer, tamanioTotal);
	free(buffer);
}

void *serializarArchivo(int tamanio, char* contenido, myHeader* header){
	archivo *paqueteArchivo;

	paqueteArchivo->tamanio = tamanio;
	paqueteArchivo->contenido = contenido;


	header->id = 4; //TODO ver cual va a ser el header

	int desplazamiento = 0;

	int tamanioTotal = sizeof(paqueteArchivo->tamanio)+strlen(paqueteArchivo->contenido);

	void *buffer = malloc(tamanioTotal);


	header->tamanio=sizeof(paqueteArchivo->tamanio);
	memcpy(buffer + desplazamiento,&tamanio, sizeof(paqueteArchivo->tamanio));
	desplazamiento += sizeof(paqueteArchivo->tamanio);

	header->tamanio+=paqueteArchivo->tamanio;
	memcpy(buffer + desplazamiento,paqueteArchivo->contenido, paqueteArchivo->tamanio);

	return buffer;
}

#include "funcionesMaster.h"

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

//FUNCION CON EL SELECT PARA LOS WORKERS
//TODO se va a cambiar por hilos mas adelante
void iniciarMaster(char* transformador,char* reductor,char* archivoAprocesar,char* direccionDeResultado){
	struct sockaddr_in dir;
	int socketMaster;
	int opt = 1;
	int addrlen , nuevoSocket , socketCliente[30] , max_clients = 5 , i ;
	int maxPuerto;
	fd_set readfds, auxRead;

	//configurarAddr(&dir);
	dir.sin_family = AF_INET;
	dir.sin_port = htons(24000);
	dir.sin_addr.s_addr = INADDR_ANY;
	memset(&(dir.sin_zero), '\0', 8);



	//inicializar todos los socketCliente
	    for (i = 0; i < max_clients; i++)
	    {
	        socketCliente[i] = 0;
	    }

	socketMaster = socket(AF_INET, SOCK_STREAM, 0);

	// Olvidémonos del error "Address already in use" [La dirección ya se está usando]
	int yes=1;
		 if (setsockopt(socketMaster,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		 perror("setsockopt");
		 exit(1);
		 }


	if(bind(socketMaster, (struct sockaddr *)&dir, sizeof(struct sockaddr)) != 0){
		perror("fallo el bind");
		exit(1);
	}
	listen(socketMaster, 100);
	/********************************************************/

	struct sockaddr_in direccionCliente;
	int tamanioDir;

	char *buffer;
	int bytesRecibidos;
	FILE *fich;

	FD_ZERO(&readfds);
	FD_ZERO(&auxRead);
	FD_SET(socketMaster, &auxRead);

	maxPuerto = socketMaster;
	tamanioDir = sizeof(direccionCliente);

	//-----------------------
	//Calculo el tamanio del archivo que voy a mandar
	long tam = calcularTamanioArchivo(fich,transformador);

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
					enviarArchivo(nuevoSocket, fich, buffer, &tam, transformador);
					if(nuevoSocket > maxPuerto)
						maxPuerto = nuevoSocket;
				}
		      }
		      else{
				buffer = malloc(1000);

				recibirPorSocket(i,buffer,1000);
				if(bytesRecibidos < 0){
					perror("Error");
					free(buffer);
					exit(1);
				}
				if(bytesRecibidos == 0){
					//printf("Se desconecto del fileSystem el socket %d", i);
					 FD_CLR(i, &readfds);
					 shutdown(i, 2);
					 free(buffer);
				}else{
					 buffer[bytesRecibidos] = '\0';
					 printf("Socket: %d -- BytesRecibidos: %d -- Buffer recibido : %s\n",i, bytesRecibidos , buffer);

				}
		      }
		    }
		}
	}

}
