#include "suite.h"

int correrTests() {
	// Inicializa un registro de suites vacío.
	CU_initialize_registry();

	// Crea una nueva suite de tests, sin tomar funciones de inicializacion ni limpieza.
	CU_pSuite suiteFs = CU_add_suite("Suite fileSystem", NULL, NULL);

	// Agrego un caso de prueba a la suite.
	CU_add_test(suiteFs, "existePathDirectorio",
			test_existePathDirectorio_noExistePath);
	CU_add_test(suiteFs, "existePathDirectorio",
			test_existePathDirectorio_existePath);
	CU_add_test(suiteFs, "compararBloquesLibres",
			test_compararBloquesLibres_Nodo1MasLibreQueNodo2);
	CU_add_test(suiteFs, "ordenarListaNodos",
			test_ordenarListaNodos_ordenaCorrectamente);
	CU_add_test(suiteFs, "copiarListaNodos",
			test_copiarListaNodos_copiaCorrectamente);
	CU_add_test(suiteFs, "modificarListaCopiadaDeNodos",
			test_modificarListaCopiadaDeNodos_noModificaListaOriginal
);

	// Settea la librería de modo tal que muestre la mayor cantidad de información posible (con el flag CU_BRM_VERBOSE).
	CU_basic_set_mode(CU_BRM_VERBOSE);

	// Corre los tests.
	CU_basic_run_tests();

	/* Destruye todas las estructuras y los tests creados,
	 * libera la memoria utilizada por la librería (para evitar memory leaks).
	 * CUnit nos pide que lo llamemos y que sea lo último que hagamos. */
	CU_cleanup_registry();

	// Devuelve un código al finalizar la ejecución, que puede ser de error o de ejecución correcta.
	return CU_get_error();
}

/* Test cases */
void test_existePathDirectorio_noExistePath(void) {
	CU_ASSERT_FALSE(existePathDirectorio("/path/que/no/existe"));
}

void test_existePathDirectorio_existePath(void) {
	CU_ASSERT_TRUE(existePathDirectorio("/root"));
}

void test_compararBloquesLibres_Nodo1MasLibreQueNodo2(void) {
	t_list *listaNodos = list_create();

	t_nodo *nodo1 = malloc(sizeof(t_nodo));
	nodo1->idNodo = 1;
	nodo1->bloquesLibres = 5;
	agregarNodo(listaNodos, nodo1);

	t_nodo * nodo2 = malloc(sizeof(t_nodo));
	nodo2->idNodo = 2;
	nodo2->bloquesLibres = 10;
	agregarNodo(listaNodos, nodo2);

	// Devuelve true si el primero tiene mas bloques libres que el segundo.
	CU_ASSERT_EQUAL(compararBloquesLibres(nodo1, nodo2), false);

	free(nodo1);
	free(nodo2);
}

void test_ordenarListaNodos_ordenaCorrectamente(void) {
	t_list *listaNodos = list_create();

	// Nodo 1
	t_nodo *nodo = malloc(sizeof(t_nodo));
	nodo->idNodo = 1;
	nodo->bloquesLibres = 10;
	agregarNodo(listaNodos, nodo);

	// Nodo 2
	nodo = malloc(sizeof(t_nodo));
	nodo->idNodo = 2;
	nodo->bloquesLibres = 10;
	agregarNodo(listaNodos, nodo);

	// Nodo 3
	nodo = malloc(sizeof(t_nodo));
	nodo->idNodo = 3;
	nodo->bloquesLibres = 15;
	agregarNodo(listaNodos, nodo);

	ordenarListaNodos(listaNodos);

	nodo = malloc(sizeof(t_nodo));
	nodo = list_get(listaNodos, 0);

	CU_ASSERT_EQUAL(nodo->idNodo, 3);
}

void test_copiarListaNodos_copiaCorrectamente(void) {
	t_list *copia, *lista = list_create();
	t_nodo *nodoCopia, *nodo = malloc(sizeof(t_nodo));
	nodo->idNodo = 1;
	nodo->bloquesTotales = 50;
	nodo->bloquesLibres = 10;
	list_add(lista, nodo);

	nodo = malloc(sizeof(t_nodo));
	nodo->idNodo = 2;
	nodo->bloquesTotales = 30;
	nodo->bloquesLibres = 5;
	list_add(lista, nodo);

	copia = copiarListaNodos(lista);
	nodoCopia = list_get(copia, 1);
	CU_ASSERT_EQUAL(nodoCopia->idNodo, 2);
}

void test_modificarListaCopiadaDeNodos_noModificaListaOriginal(void) {
	t_list *copia, *lista = list_create();
	t_nodo *nodoCopia, *nodo = malloc(sizeof(t_nodo));
	nodo->idNodo = 1;
	nodo->bloquesTotales = 50;
	nodo->bloquesLibres = 10;
	list_add(lista, nodo);

	nodo = malloc(sizeof(t_nodo));
	nodo->idNodo = 2;
	nodo->bloquesTotales = 30;
	nodo->bloquesLibres = 5;
	list_add(lista, nodo);

	copia = copiarListaNodos(lista);
	nodoCopia = list_get(copia, 1);
	nodoCopia->idNodo = 0;
	CU_ASSERT_NOT_EQUAL(nodo->idNodo, 0);
	CU_ASSERT_EQUAL(nodo->idNodo, 2);
}

