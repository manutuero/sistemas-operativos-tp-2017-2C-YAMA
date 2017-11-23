#ifndef FUNCIONESWORKER_H_
#define FUNCIONESWORKER_H_

#include <utils.h>
#include <commons/collections/list.h>
#include <commons/txt.h>
//#include <linux/sort.h>



/* Variables globales */
t_log* workerLogger;
char* NODOARCHCONFIG;
char* IP_FILESYSTEM;
int PUERTO_FILESYSTEM;
char* ID_NODO;
int PUERTO_WORKER;
int PUERTO_DATANODE;
char* RUTA_DATABIN;
FILE *filePointer;
int fileDescriptor;
extern char* pathTemporales;
struct sockaddr_in direccionWorker;
int socketWorker;

#define UN_MEGABYTE 1048576
#define UN_BLOQUE sizeof(char)*UN_MEGABYTE

typedef char* t_regArch ;

typedef struct {
	char* ip;
	uint32_t puerto;
	char* nombre;
}__attribute__((packed)) t_infoArchivo;

typedef struct {
	uint32_t tamanio;
	char* nombre;
	char* contenido;
} __attribute__((packed)) t_archivoARGlobal;

typedef struct {
	uint32_t numBloque;
	uint32_t bytesOcupados;
	uint32_t largoNombreArchTemp;
	char* nombreArchTemp;
	uint32_t largoArchTransformador;
	char* archTransformador;
}__attribute__((packed)) t_infoTransformacion;

typedef struct{
		uint32_t etapa;
		uint32_t largoRutaArchivoReductorLocal;
		char* rutaArchivoReductorLocal;
		uint32_t largoArchivoReductor;
		char* archivoReductor;
		uint32_t cantidadTransformaciones;
		t_list* temporalesTranformacion;
	}t_infoReduccionLocal;

	typedef struct{
			uint32_t etapa;
			uint32_t largoRutaArchivoTemporal;
			char* rutaArchivoTemporal;
			uint32_t largoArchivoReductor;
			char* archivoReductor;
			uint32_t cantidadNodos;
			t_list* nodosAConectar;
		}t_infoReduccionGlobal;

//enums
#define TRANSFORMACION 10
#define TRANSFORMACION_OK 11
#define REDUCCION_LOCAL 14
#define REDUCCION_LOCAL_OK 15
#define REDUCCION_GLOBAL 18
#define REDUCCION_GLOBAL_OK 19
#define GUARDAR_FINAL 23
#define LARGO_MAX_LINEA 1048576
/* Firmas de funciones */
void crearLogger();
void cargarArchivoConfiguracion(char*);
int recibirArchivo(int cliente);
int recibirYDeserializar(int fd, t_archivo* miArchivo);
t_archivo* deserializarArchivo(void *buffer);
void recibirInfoOperacion(int);
t_infoTransformacion deserializarInfoTransformacion(void* buffer);
t_infoReduccionLocal deserializarInfoReduccionLocal(void* buffer);
t_infoReduccionGlobal deserializarInfoReduccionGlobal(void* buffer);
char* getBloque(int numero);
void abrirDatabin();
void cerrarDatabin();
void realizarTransformacion(t_infoTransformacion);
void realizarReduccionLocal(t_infoReduccionLocal);
void realizarReduccionGLobal(t_infoReduccionGlobal);
char* armarNombreConPathTemp(char*);
char *guardarArchScript(char*);

#endif
