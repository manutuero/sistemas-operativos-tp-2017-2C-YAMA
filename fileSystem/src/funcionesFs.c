#include "funcionesFs.h"

void procesarComandoConsola(void *buffer,int sd){
 comando *comando=buffer;
 printf("Recibi el comando codigo= %d. Envio respuesta \n",comando->funcion);
 //free(comando);
 //int enviarPorSocket(int socket, const void * mensaje, int tamanio)
 ///char * respuesta = "Comando recibido y ejecutado";
 //enviarPorSocket(sd,respuesta,sizeof(respuesta));
}

void procesarMensaje(void * buffer, int sd ,header header){

	switch (header.id){
	case 1:
		procesarComandoConsola(buffer,sd);
		//printf("Recibi un comando de consola con id= %d",comando.funcion);
		break;
	default :
		perror("Recibi verdura y no entendi nada");
	break;
}



}
