#ifndef FUNCIONESWORKER_H_
#define FUNCIONESWORKER_H_

#include <utils.h>
#include <commons/collections/list.h>
#include <commons/txt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <commons/txt.h>

//#include <linux/sort.h>

/* Variables globales */
t_log* workerLogger;
char* NODOARCHCONFIG;
char* IP_FILESYSTEM;
extern char* pathTemporales;
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

typedef char* t_regArch;

typedef struct {
	char* ip;
	uint32_t puerto;
	char* nombre;
}__attribute__((packed)) t_infoArchivo;

typedef struct {
	uint32_t tamanio;
	char* nombre;
	char* contenido;
}__attribute__((packed)) t_archivoARGlobal;

typedef struct {
	uint32_t numBloque;
	uint32_t bytesOcupados;
	uint32_t largoNombreArchTemp;
	char* nombreArchTemp;
	uint32_t largoArchTransformador;
	char* archTransformador;
}__attribute__((packed)) t_infoTransformacion;

typedef struct {
	uint32_t largoRutaArchReducidoLocal;
	char* rutaArchReducidoLocal;
	uint32_t largoArchivoReductor;
	char* archReductor;
	uint32_t cantidadTransformaciones;
	t_list* archTemporales;
}__attribute__((packed)) t_infoReduccionLocal;

typedef struct {
	uint32_t largoRutaArchivoTemporalFinal;
	char* rutaArchivoTemporalFinal;
	uint32_t largoArchivoReductor;
	char* archivoReductor;
	uint32_t cantidadNodos;
	t_list* nodosAConectar;
}__attribute__((packed)) t_infoReduccionGlobal;

typedef char* rutaTemporal;

typedef struct{
		uint32_t largoRutaTemporalTransformacion;
		char* rutaTemporalTransformacion;
	}__attribute__((packed)) t_temporalesTransformacionWorker;

typedef struct {
	uint32_t puerto;
	uint32_t largoIp;
	char* ip;
	uint32_t largoRutaArchivoReduccionLocal;
	char* rutaArchivoReduccionLocal;
}__attribute__((packed)) t_datosNodoAEncargado;

typedef struct {
	int largoRutaArchivoFinal;
	char* rutaArchivoFinal;
	int largoArchivo;
	char* archivoFinal;
}__attribute__((packed)) t_infoArchivoFinal;

typedef struct {
	int largoRutaTemporal;
	char* rutaTemporal;
	int largoRutaArchFinal;
	char* rutaArchFInal;
}__attribute__((packed)) t_infoGuardadoFinal;

//defines
#define TRANSFORMACION 10
#define TRANSFORMACION_OK 11
#define REDUCCION_LOCAL 14
#define REDUCCION_LOCAL_OK 15
#define REDUCCION_GLOBAL 18
#define REDUCCION_GLOBAL_OK 19
#define SOLICITUD_WORKER 25
#define RECIBIR_ARCH_TEMP 26
#define ORDEN_GUARDADO_FINAL 22
#define GUARDAR_FINAL 23
#define ERROR_REDUCCION_GLOBAL  106
#define ERROR_ARCHIVO_NO_ENCONTRADO 105
#define LARGO_MAX_LINEA 1048576
#define LARGO_MAX_LINEA 1048576

/* Firmas de funciones */
void crearLogger();
void cargarArchivoConfiguracion(char*);
int recibirArchivo(int cliente);
int recibirYDeserializar(int fd, t_archivo* miArchivo);
t_archivo* deserializarArchivo(void *buffer);
void recibirInfoOperacion(int);
t_infoTransformacion* deserializarInfoTransformacion(void* buffer);
t_infoReduccionLocal* deserializarInfoReduccionLocal(void* buffer);
t_infoReduccionGlobal* deserializarInfoReduccionGlobal(void* buffer);
char* getBloque(int numero);
void abrirDatabin();
void cerrarDatabin();
void realizarTransformacion(t_infoTransformacion*);
void realizarReduccionLocal(t_infoReduccionLocal*);
void realizarReduccionGlobal(t_infoReduccionGlobal*);
char* armarNombreConPathTemp(char*);
char *guardarArchScript(char*);

int solicitarArchivoAWorker(char*, int,char*);
void* serializarSolicitudArchivo(char*,int*);
char* deserializarSolicitudArchivo(void*);
int conectarseAWorker(int , char* );
void responderSolicitudArchivoWorker(char*,int);
int verificarExistenciaArchTemp(char*,char*);
char* obtenerContenidoArchivo(char*);
int devolverTamanioArchivo(char*);
t_infoGuardadoFinal* deserializarInfoGuardadoFinal(void*);
char* recibirArchivoTemp(int,int*);
char* deserializarRecepcionArchivoTemp(void*);
void aparearArchivos(FILE*,FILE*,FILE*);
void leerRegArchivo(FILE*,t_regArch,bool*);
void copiarContenidoDeArchivo(FILE*,FILE*);

void* serializarInfoGuardadoFinal(int,char*,t_infoGuardadoFinal*,int*);
void guardadoFinalEnFilesystem(t_infoGuardadoFinal*);

#endif
