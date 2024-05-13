#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "byte_stuff.h"

void byte_stuff(char **array) {
    int i = 0;
    int j = 0;
    int size = 0;

    // Calcula o novo tamanho do array
    while ((*array)[i] != '\0') {
        if ((*array)[i] == FLAG || (*array)[i] == ESCAPE) {
            size++;
        }
        size++;
        i++;
    }

    // Aloca memória para o novo array
    char *new_array = (char *)malloc(size);
    if (new_array == NULL) {
        printf("Erro ao alocar memória.\n");
        return;
    }

    // Preenche o novo array com os dados modificados
    i = 0;
    while ((*array)[i] != '\0') {
        if ((*array)[i] == FLAG || (*array)[i] == ESCAPE) {
            new_array[j] = ESCAPE;
            j++;
            new_array[j] = (*array)[i] ^ STUFF;
        } else {
            new_array[j] = (*array)[i];
        }
        i++;
        j++;
    }

    // Realoca memória para o array original
    *array = (char *)realloc(*array, size);
    if (*array == NULL) {
        printf("Erro ao realocar memória.\n");
        free(new_array);
        return;
    }

    // Copia os dados do novo array para o array original
    strcpy(*array, new_array);
    free(new_array);
}


void byte_destuff(char **array){
   
    int i = 0;
    int size = 0;
    while ((*array)[i] != '\0') {
        if ((*array)[i] == 0x5d && (*array)[i + 1] == 0x7c) {
            size++;
            i += 2; 
        } else if ((*array)[i] == 0x5d && (*array)[i + 1] == 0x7d) {
            size++;
            i += 2; 
        } else {
            size++;
            i++;
        }
    }

    char *new_array = (char *)malloc(size + 1);

    i = 0;
    int j = 0;
    while ((*array)[i] != '\0') {
        if ((*array)[i] == 0x5d && (*array)[i + 1] == 0x7c) {
            new_array[j] = 0x5c;
            j++;
            i += 2; 
        } else if ((*array)[i] == 0x5d && (*array)[i + 1] == 0x7d) {
            new_array[j] = 0x5d;
            j++;
            i += 2; 
        } else {
            new_array[j] = (*array)[i];
            j++;
            i++;
        }
    }
    new_array[j] = '\0'; 

    free(*array);
    *array = new_array;
    printf("Destuffed: %s\n", *array);
}