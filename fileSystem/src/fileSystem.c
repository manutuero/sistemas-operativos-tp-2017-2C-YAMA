#include "funcionesFileSystem.h"

int estadoFs = 0;

int main(void) {
	ARCHCONFIG = "fsConfig.cfg";
	cargarArchivoDeConfiguracion(ARCHCONFIG);

	// Espera que se conecten todos los datanodes para alcanzar un estado "estable", no dejar entrar a YAMA hasta que eso pase.
	// es  un hilo ya que una vez que queda estable, sigue escuchando conexiones
	esperarConexionesDatanodes();

	while(estadoFs == 0){ //Cambia a 1 cuando hay una copia de cada bloque disponible
		sleep(1000);
		printf("Esperando datanodes");
	}

	//iniciar hilo consola


	return EXIT_SUCCESS;
}
