#include "funcionesFileSystem.h"

int main(void) {
	// Agregar un logger (cuando estemos por terminar).
	ARCHCONFIG = "fsConfig.cfg";
	cargarArchivoDeConfiguracionFS(ARCHCONFIG);

	if (hayEstadoAnterior()) {
		cargarEstructurasAdministrativas();
		puts("Hay estado anterior...esperar que se conecten los Datanodes que estaban."); // borrar luego :p
	} else {
		puts("No hay estado anterior, laburar desde 0."); // borrar luego :p
	}

	// Hacer funcion que vea si hay validarEstadoAnterior() cargando , que un campo sea socketDescriptor y otro campo nodoId y estaConectado

	// Espera que se conecten todos los datanodes para alcanzar un estado "estable", no dejar entrar a YAMA hasta que eso pase.
	// es  un hilo ya que una vez que queda estable, sigue escuchando conexiones
	//esperarConexionesDatanodes();
	pthread_t hiloConexiones;
	pthread_create(&hiloConexiones, NULL, esperarConexionesDatanodes, NULL);

	/* Iniciar hilo consola. Habria que poner un mutex o algo para evitar que entre mientras el fs no sea estable.
	 Tambien agregar hilo de conexion con Yama. */
	pthread_t hiloConsola;
	pthread_create(&hiloConsola, NULL, levantarConsola, NULL);

	//Ver aca como evitar que se termine el hilo principal mientras los otros estan corriendo.
	while (1) {
		sleep(1000);
	}

	return 0;
}
