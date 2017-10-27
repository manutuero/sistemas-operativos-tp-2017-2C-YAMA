#ifndef TEST_SUITE_H_
#define TEST_SUITE_H_

#include "../funcionesFileSystem.h"
#include "../API/fileSystemAPI.h"
#include <CUnit/Basic.h>

int correrTests();

/* Test cases Directorios */
void test_existePathDirectorio_noExistePath(void);
void test_existePathDirectorio_existePath(void);

/* Test cases Nodos */
void test_ordenarListaNodos_ordenaCorrectamente(void);
void test_compararBloquesLibres_Nodo1MasLibreQueNodo2(void);
void test_copiarYLiberarListaNodos_copiaCorrectamente(void);
void test_liberarVariableNodo_noModificaUltimoElementoDeLaLista(void);

/* Test cases Commons Config */
void test_crearArchivoDiccionario_creaArchivoCorrectamente(void);

#endif
