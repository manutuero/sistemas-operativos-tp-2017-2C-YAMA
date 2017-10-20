#include "funcionesPlanificacion.h"

t_worker_Disponibles workers[30];
int disponibilidadBase=1;//hardcode de momento a retirar de yamaConfig.cfg
int algoritmo = 1;//idem anterior, 0 para clock y 1 para wclock
uint32_t job = 0;
int idMaster = 1;

void mostrarTablaDeEstados(int i) {
	printf("Tabla de estados:\n");
	printf("job\tmaster\tnodo\tbloque\tetapa\testado\ttemporal\n");
	for (i = 0; i < list_size(listaTablaEstados); i++) {
		t_tabla_estados registro = *(t_tabla_estados*) list_get(
				listaTablaEstados, i);
		printf("%d\t%d\t%d\t%d\t%d\t%d\t%s\n", registro.job, registro.master,
				registro.nodo, registro.bloque, registro.etapa, registro.estado,
				registro.archivoTemp);
	}
}

//esta funcion va a recibir por parametro una estructura que tenga el job, el idMaster y el socket de ese master.
void preplanificarJob(t_job* jobMaster){
	//t_list listaBloquesRecibidos, listaNodosInvolucrados, listaPlanificacionMaster;
	t_header header;
	t_bloqueRecv* bloqueRecibido;
	//t_planificacion* planificacion;
	int i,socketFS,socketMaster,cantNodosInvolucrados,*clock,*clockAux;
	void* buffer;


	/* Reservar memoria para los clocks */

	clock = malloc(sizeof(uint32_t));
	clockAux = malloc(sizeof(uint32_t));

	/* Inicializo el array de todos los workers, deshabilitados  */

	inicializarWorkers();

	/* Recibir de FS la composicion completa del archivo, en el header usamos
	 * tamaño de payload para saber cantidad de bloques a recibir*/

	//recibirHeader(socketFS,&header);

	/* carga local de bloques de prueba */
	t_bloqueRecv bloques[3] = {{0, 1, 0, 3, 4},
										{1, 2, 4, 3, 9},
										{2, 4, 7, 1, 3}};
	header.tamanioPayload = 3;


	crearListas();

	int nodo;
	for (i=0;i<header.tamanioPayload;i++)
	{
		//Cambio bloqueRecibido por buffer en el malloc
		buffer = malloc(sizeof(t_bloqueRecv));//Se liberara cuando se destruya la lista
		bloqueRecibido = malloc(sizeof(t_bloqueRecv));//Se liberara cuando se destruya la lista
		//recibirPorSocket(socketFS, buffer, sizeof(t_bloqueRecv));
		//bloqueRecibido=(t_bloqueRecv*)buffer;

		//prueba local (sin sockets)
		bloqueRecibido=&bloques[i];
		guardarEnBloqueRecibidos(bloqueRecibido);

	}
	t_bloqueRecv* bloque;
	for(i=0;i<list_size(listaBloquesRecibidos);i++){
			bloque = (t_bloqueRecv*)list_get(listaBloquesRecibidos,i);
			printf("bloque  %d  nodo0: %d    nodo1: %d\n",bloque->nroBloqueArch, bloque->idNodo0, bloque->idNodo1);
		}
	printf("nodos involucrados: \n");
		for(i=0;i<list_size(listaNodosInvolucrados);i++){
			nodo = *(int*)list_get(listaNodosInvolucrados,i);
			printf("nodo %d\n",*(int*)list_get(listaNodosInvolucrados,i));
		}

	/* Ordenar lista por mayor disponibilidad */
	list_sort(listaNodosInvolucrados,ordenarPorDisponibilidad);

	/* Generar vector que sera utilizado por el clock */
	cantNodosInvolucrados= list_size(listaNodosInvolucrados);
	int nodosInvolucrados[cantNodosInvolucrados];
	cargarVector(nodosInvolucrados,listaNodosInvolucrados);

	printf("nodos ordenados: \n");
			for(i=0;i<cantNodosInvolucrados;i++){
				printf("nodo %d\n",nodosInvolucrados[i]);
			}


	/* Como esta ordenado por mayor disponibilidad, clock y clockAux apuntan a 0 */

	*clock = 0;
	*clockAux = 0;

									/* Inicia la planificacion */

	/* pre-planificacion de transformaciones */
	for(i=0;i<list_size(listaBloquesRecibidos);i++)
	{
		bloqueRecibido = list_get(listaBloquesRecibidos,i);
		planificarTransformaciones(cantNodosInvolucrados,nodosInvolucrados,bloqueRecibido,clock,clockAux);
	}

	/* pre-planificacion de reducciones locales */
	planificacionReduccionLocal();

	/* pre-planificacion de reducion global */
	planificacionReduccionGlobal(cantNodosInvolucrados,nodosInvolucrados);

	printf("planifico. \n");
	mostrarTablaDeEstados(i);

	/* Actualizar workload aplicando estupiad ecuacion*/
	actualizarWorkload(cantNodosInvolucrados,nodosInvolucrados);

	/* Enviar toda la planificacion a master */

	/* liberar listas para la siguiente planificacion */
	destruir_listas();

	/* Liberar variables dinamicas */
	free(buffer);
	free(clock);
	free(clockAux);
}



/* 						all this will go in funcionesYama.c						*/


void crearListas(){
	listaBloquesRecibidos = list_create();
		listaNodosInvolucrados= list_create();
		listaPlanTransformaciones = list_create();
		listaPlanRedLocal = list_create();
		listaPlanRedGlobal = list_create();
}


void inicializarWorkers()
{
	int i;
	for (i=0;i<30;i++)
	{
		workers[i].habilitado = 1;
		workers[i].disponibilidad = 0;
		workers[i].idNodo = i;
		workers[i].puerto = 0;
		workers[i].ip = "127.0.0.1";
		workers[i].workLoad = 0;
		workers[i].cant_transformaciones = 0;
		workers[i].cant_red_globales = 0;
	}
	return;
}

void guardarEnBloqueRecibidos(t_bloqueRecv* recibido)
{
	nodosCargados(recibido->idNodo0,recibido->idNodo1);
	list_add(listaBloquesRecibidos,recibido);
}

void nodosCargados(int idNodo0,int idNodo1)
{
	int *nodoACargar1,*nodoACargar2;

	if ((!existeIdNodo(idNodo0)) && (workers[idNodo0].habilitado == 1))
	{
		nodoACargar1 = malloc(sizeof(int));
		*nodoACargar1 = idNodo0;
		list_add(listaNodosInvolucrados,nodoACargar1);
	}
	if ((!existeIdNodo(idNodo1)) && (workers[idNodo1].habilitado == 1))
	{
		nodoACargar2 = malloc(sizeof(int));
		*nodoACargar2 = idNodo1;
		list_add(listaNodosInvolucrados,nodoACargar2);
	}
}

int existeIdNodo(int val1){
	int i;
	int* val2;
	for (i=0;i<list_size(listaNodosInvolucrados);i++)
	{
		val2=(int*)list_get(listaNodosInvolucrados,i);
		if(val1 == *val2){
			return 1;
		}
	}
	return 0;
}

bool ordenarPorDisponibilidad(void* val1,void* val2){
	int nodo0,nodo1;
	nodo0 = *(int*)val1;
	nodo1 = *(int*)val2;


	if ((workers[nodo0].disponibilidad) >= (workers[nodo1].disponibilidad))
		return 1;
	else
		return 0;

}

void cargarVector(int* vectorNodos, t_list* listaNodos){
 int i=0,*idNodo;

 while (!list_is_empty(listaNodos))
 {
  idNodo = list_remove(listaNodos,0);
  vectorNodos[i] = *idNodo;
  i++;
 }

 list_destroy(listaNodos);
}
//ahi deberia funcionar

//verificar el funcionamiento del clock
void planificarTransformaciones(int cantNodos,int* nodosInvolucrados, t_bloqueRecv* bloqueRecibido,int* clock,int* clockAux){
	int asignado=0;

	while(asignado==0)
	{
		if (nodoContieneBloque(*bloqueRecibido,nodosInvolucrados,clock))
		{
			if(workers[nodosInvolucrados[*clock]].disponibilidad>0)
			{
				actualizarPlanificacion(*bloqueRecibido,nodosInvolucrados,clock);
				actualizarWorker(nodosInvolucrados,clock);
				actualizarTablaEstados(*bloqueRecibido,nodosInvolucrados,clock);
				desplazarClock(clock,cantNodos);
				*clockAux=*clock;
				asignado=1;
			}else
			{
				incrementarDisponibilidadWorkers(cantNodos,nodosInvolucrados);
			}
		}
		else if(nodoContieneBloque(*bloqueRecibido,nodosInvolucrados,clockAux))
			{
				if(workers[nodosInvolucrados[*clockAux]].disponibilidad>0)
				{
					actualizarPlanificacion(*bloqueRecibido,nodosInvolucrados,clockAux);
					actualizarWorker(nodosInvolucrados,clockAux);
					actualizarTablaEstados(*bloqueRecibido,nodosInvolucrados,clockAux);
					clockAux = clock;
					asignado=1;
				}else
				{
					incrementarDisponibilidadWorkers(cantNodos,nodosInvolucrados);
				}
			}else
			{
				*clockAux = *clockAux + 1;
				//falta ver cuando no esta habilitado en ningun lado
			}
	}
	return;
}

int nodoContieneBloque(t_bloqueRecv bloqueRecibido,int* nodosInvolucrados,int* clock)
{
	/* Verifico que el nodo en cuestion este habilitado */
	if (workers[(nodosInvolucrados[*clock])].habilitado==0)
		return 0;

	/* Verifico que el nodo en cuestion sea alguno de los que tiene el bloque*/
	if((bloqueRecibido.idNodo0)==nodosInvolucrados[*clock]||((bloqueRecibido.idNodo1)==nodosInvolucrados[*clock]))
		return 1;
	else
		return 0;
}

void actualizarPlanificacion(t_bloqueRecv bloqueRecibido,int* nodosInvolucrados,int* clock)
{
	char *nombreTMP;
	nombreTMP = string_new();
	t_transformacionMaster* bloquePlanificado;
	bloquePlanificado = malloc(sizeof(t_transformacionMaster));

	bloquePlanificado->idNodo=nodosInvolucrados[*clock];
	bloquePlanificado->bytesOcupados=bloqueRecibido.bytesOcupados;
	bloquePlanificado->nroBloqueNodo=cargarNroBloque(bloqueRecibido,nodosInvolucrados,clock);
	bloquePlanificado->puerto=workers[nodosInvolucrados[*clock]].puerto;

	bloquePlanificado->ip=malloc(strlen(workers[nodosInvolucrados[*clock]].ip));
	strcpy(bloquePlanificado->ip,workers[nodosInvolucrados[*clock]].ip);

	nombreTMP=string_duplicate(generarNombreArchivoTemporal(job,bloquePlanificado->idNodo,bloquePlanificado->nroBloqueNodo));
	bloquePlanificado->archivoTransformacion = malloc(strlen(nombreTMP));
	strcpy(bloquePlanificado->archivoTransformacion,nombreTMP);

	list_add(listaPlanTransformaciones,bloquePlanificado);

	return;
}

int cargarNroBloque(t_bloqueRecv bloque,int* nodosInvolucrados,int* clock)
{
	if (bloque.idNodo0==nodosInvolucrados[*clock])
		return bloque.nroBloqueNodo0;
	else
		return bloque.nroBloqueNodo1;
}

void desplazarClock(int* clock,int cantidad){

	if(*clock < cantidad)
		*clock=*clock+1;
	else
		*clock = 0;
	return;
}

void actualizarWorker(int* nodosInvolucrados,int* clock)
{
	int idNodo = nodosInvolucrados[*clock];
	workers[idNodo].cant_transformaciones++;
	workers[idNodo].workLoad++;
	workers[idNodo].disponibilidad--;
}

void actualizarTablaEstados(t_bloqueRecv bloqueRecibido,int* nodosInvolucrados,int* clock){
	t_tabla_estados* registroEstado;
	registroEstado = malloc(sizeof(t_tabla_estados));
	registroEstado->job = job;
	registroEstado->master = idMaster;
	registroEstado->nodo = nodosInvolucrados[*clock];
	registroEstado->bloque = cargarNroBloque(bloqueRecibido,nodosInvolucrados,clock);
	registroEstado->etapa = 1; //ver cual va a ser la macro de etapa
	registroEstado->estado = 1; //ver cual va a ser la macro de estado
	char* temp = string_new();
	temp = string_duplicate(generarNombreArchivoTemporal(job, registroEstado->nodo, registroEstado->bloque));

	registroEstado->archivoTemp = malloc(strlen(temp));
	strcpy(registroEstado->archivoTemp,temp);
	list_add(listaTablaEstados, registroEstado);
}

char* generarNombreArchivoTemporal(int job, int nodo, int bloque){
	char* nombre;
	char* jobString, *nodoString, *bloqueString;
	nombre = string_new();
	jobString = string_new();
	nodoString = string_new();
	bloqueString = string_new();

	jobString = string_itoa(job);
	nodoString = string_itoa(nodo);
	bloqueString = string_itoa(bloque);

	string_append(&nombre, "/tmp/");
	string_append_with_format(&nombre, "j%s", jobString);

	if(bloque == 9000){ //si es reduccion global
		string_append(&nombre, "rg");
	}else{
		string_append_with_format(&nombre, "n%s", nodoString);
		if(bloque == 8000){
		string_append(&nombre,"rl");
		}else{ //si es reduccion local
			string_append_with_format(&nombre, "b%s", bloqueString);
		}
	}
	return nombre;
}

void incrementarDisponibilidadWorkers(int cantNodos,int* nodosInvolucrados){

	int i;

	for (i=0;i<cantNodos;i++)
	{
		workers[nodosInvolucrados[i]].disponibilidad+=disponibilidadBase;
	}
	return;
}


/*								Reduccion Local								*/

void planificacionReduccionLocal()
{
	int cantTransformaciones = list_size(listaPlanTransformaciones);
	int nodoActual, i=0;
	t_transformacionMaster *transformacion;
	t_reduccionLocalMaster *redLocal;
	char *nombreTMP;

	transformacion = malloc(sizeof(t_transformacionMaster));
	redLocal = malloc(sizeof(t_reduccionLocalMaster));
	nombreTMP = string_new();
	transformacion = list_get(listaPlanTransformaciones,i);

	while(i<cantTransformaciones)
	{
		nodoActual = transformacion->idNodo;

		nombreTMP = string_duplicate(generarNombreArchivoTemporal(job, transformacion->idNodo, 8000));

		while ((i<cantTransformaciones)&&(nodoActual==transformacion->idNodo))
		{
			cargarInfoReduccionLocal(transformacion,redLocal,nombreTMP);
			list_add(listaPlanRedLocal,redLocal);
			i++;
			cargarReduccionLocalTablaEstado(nombreTMP,redLocal->idNodo);
			transformacion=list_get(listaPlanTransformaciones,i);
			redLocal = malloc(sizeof(t_reduccionLocalMaster));
		}

	}

	free(nombreTMP);

	return;
}

void cargarInfoReduccionLocal(t_transformacionMaster *transformacion, t_reduccionLocalMaster *reduccion, char *nombreTMP)
{
	reduccion->idNodo = transformacion->idNodo;
	reduccion->puerto = transformacion->puerto;

	reduccion->archivoRedLocal = malloc(strlen(nombreTMP));
	strcpy(reduccion->archivoRedLocal,nombreTMP);

	reduccion->ip = malloc(strlen(transformacion->ip));
	strcpy(reduccion->ip,transformacion->ip);

	reduccion->archivoTransformacion = malloc(strlen(transformacion->archivoTransformacion));
	strcpy(reduccion->archivoTransformacion,transformacion->archivoTransformacion);

	return;
}

void cargarReduccionLocalTablaEstado(char* nombreTMP,int idNodo)
{
	t_tabla_estados *registro;
	registro = malloc(sizeof(t_tabla_estados));
	registro->archivoTemp = malloc(strlen(nombreTMP));
	strcpy(registro->archivoTemp,nombreTMP);
	registro->estado=1;// 1 en progreso
	registro->etapa=2;// 2 para reduccion local
	registro->job = job;
	registro->master = idMaster;
	registro->nodo = idNodo;
	registro->bloque=8000;
	list_add(listaTablaEstados,registro);
	return;

}

/*  							Reduccion global 							*/
void planificacionReduccionGlobal(int cantNodos,int *nodosInvolucrados)
{
	int nodoReduccionGlobal,i;
	t_list *listaAux;
	listaAux = list_create();

	seleccionarTransformacionLocales(listaAux);

	t_reduccionGlobalMaster *infoReduccionGlobal;

	nodoReduccionGlobal = seleccionarNodoMenorCarga(nodosInvolucrados,cantNodos);
	workers[nodoReduccionGlobal].cant_red_globales++;

	for (i=0;i<list_size(listaAux);i++)
	{
		infoReduccionGlobal= malloc(sizeof(t_reduccionGlobalMaster));
		cargarInfoReduccionGlobal(i,nodoReduccionGlobal,infoReduccionGlobal,listaAux);
		if(infoReduccionGlobal->idNodo == nodoReduccionGlobal)
		{
			cargarReduccionGlobalTablaEstados(nodoReduccionGlobal,infoReduccionGlobal);
		}

		list_add(listaPlanRedLocal,infoReduccionGlobal);
	}

	return;
}

/* Genero una lista con solo las diferentes reducciones locales, elimino las repetidas */
void seleccionarTransformacionLocales(t_list *lista)
{
	int i;
	t_reduccionLocalMaster *registro;
	registro = malloc(sizeof(t_reduccionLocalMaster));

	for (i=0;i<list_size(listaPlanRedLocal);i++)
	{
		registro = list_get(listaPlanRedLocal,i);

		if (existeRedLocal(registro,lista))
		{
			list_add(lista,registro);
			registro = malloc(sizeof(t_reduccionLocalMaster));
		}
	}
}

/* verifico si la reduccion local ya se encuntra en la lista */
bool existeRedLocal(t_reduccionLocalMaster *reduccion, t_list* lista)
{
	int i=0;

	t_reduccionLocalMaster *elementoLista;

	elementoLista = malloc(sizeof(t_reduccionLocalMaster));

	if (list_size(lista)>0)
	{
		for (i=0; i<list_size(lista);i++)
		{
			elementoLista = list_get(lista,i);
			if(sonIguales(reduccion->archivoRedLocal,elementoLista->archivoRedLocal))
			{
				return 0;
			}
		}
	}
	else
	{
		return 1;
	}

	return 1;
}

int seleccionarNodoMenorCarga(int* nodosInvolucrados,int cantNodos)
{
	int i, nodoMenorCarga=nodosInvolucrados[0];

	for (i=0;i<cantNodos;i++)
	{
		if(nodoMenorCarga>nodosInvolucrados[i])
		{
			nodoMenorCarga = nodosInvolucrados[i];
		}
	}
	return nodoMenorCarga;
}

void cargarInfoReduccionGlobal(int posicion,int nodoReduccionGlobal,t_reduccionGlobalMaster* infoReduccionGlobal,t_list *lista)
{

	char *nombreTMP,*reduccionLocal;
	t_reduccionLocalMaster *registro;

	/* Obtener el nombre del archivo de reduccion local */
	registro = malloc(sizeof(t_reduccionLocalMaster));
	registro=list_get(lista,posicion);

	reduccionLocal = malloc(strlen(registro->archivoRedLocal));
	strcpy(reduccionLocal,registro->archivoRedLocal);

	/* Cargar el resto de los campos del registro y agregarlo a la lista */
	infoReduccionGlobal->idNodo = registro->idNodo;
	infoReduccionGlobal->puerto=workers[nodoReduccionGlobal].puerto;
	infoReduccionGlobal->ip=malloc(strlen(workers[nodoReduccionGlobal].ip));
	strcpy(infoReduccionGlobal->ip,workers[nodoReduccionGlobal].ip);
	nombreTMP=generarNombreArchivoTemporal(job,nodoReduccionGlobal,9000);

	/* Verifica si es el nodo encargado y completa acordemente */
	if (registro->idNodo == nodoReduccionGlobal)
	{
		infoReduccionGlobal->encargado = 1;
		infoReduccionGlobal->archivoRedGlobal=malloc(strlen(nombreTMP));
		strcpy(infoReduccionGlobal->archivoRedGlobal,nombreTMP);
	}
	else
	{
		//si no es el encargado no le interesa el nombre del archivoRedGlobal
		infoReduccionGlobal->archivoRedGlobal = malloc(sizeof(char));
		strcpy(infoReduccionGlobal->archivoRedGlobal,"");
		infoReduccionGlobal->encargado = 0;
	}
	/* Release the memory */
	free(nombreTMP);
	return;
}

void cargarReduccionGlobalTablaEstados(int nodoReduccion,t_reduccionGlobalMaster *infoRedGlobal)
{
	t_tabla_estados *registro;
	registro = malloc(sizeof(t_tabla_estados));

	registro->archivoTemp = malloc(strlen(infoRedGlobal->archivoRedGlobal));
	strcpy(registro->archivoTemp,infoRedGlobal->archivoRedGlobal);
	registro->bloque = 9000;
	registro->estado = 1;
	registro->etapa = 3;
	registro->job = job;
	registro->master = idMaster;
	registro->nodo = infoRedGlobal->idNodo;

	list_add(listaTablaEstados,registro);

	return;
}

void actualizarWorkload(int cantNodosInvolucrados,int* nodosInvolucrados)
{
	int i,prevWorkLoad,difCarga,maxWorkload=workers[0].workLoad;

	/* Buscar maxima carga entre todos los workers */
	for (i=1;i<30;i++)
	{
		if(maxWorkload<workers[i].workLoad)
			maxWorkload=workers[i].workLoad;
	}

	/* Se actualiza segun algoritmo de planificacion */
	if (algoritmo==1)
	{
		for (i=0;i<cantNodosInvolucrados;i++)
		{
			prevWorkLoad=workers[nodosInvolucrados[i]].workLoad;
			difCarga= maxWorkload-prevWorkLoad;
			workers[nodosInvolucrados[i]].disponibilidad=disponibilidadBase+difCarga;
		}
	}
	else
	{
		for (i=0;i<cantNodosInvolucrados;i++)
		{
			workers[i].disponibilidad = disponibilidadBase;
		}
	}

	return;
}

void destruir_listas()
{
	list_destroy_and_destroy_elements(listaPlanTransformaciones,free);
	list_destroy_and_destroy_elements(listaPlanRedLocal,free);
	list_destroy_and_destroy_elements(listaPlanRedGlobal,free);
	return;
}


