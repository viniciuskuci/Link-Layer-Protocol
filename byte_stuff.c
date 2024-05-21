#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "byte_stuff.h"

int check_bcc2(const unsigned char* buf, int bufSize) {
    unsigned char bcc2 = calculate_bcc2(buf, bufSize-1);
    return bcc2 == buf[bufSize];
}

unsigned char calculate_bcc2(const unsigned char* buf, int bufSize) {
    unsigned char bcc2 = 0x00;
    for (int i = 0; i < bufSize; i++) {
        bcc2 ^= buf[i];
    }
    
    return bcc2;
}

unsigned char* byte_stuff(const unsigned char* buf, int bufSize, int* stuffedSize) {
    unsigned char* stuffedBuf = (unsigned char*)malloc(bufSize * 2);
    if (stuffedBuf == NULL) {
        *stuffedSize = -1;
        return NULL;
    }

    int j = 0;
    for (int i = 0; i < bufSize; i++) {
        if (buf[i] == FLAG || buf[i] == ESCAPE) {
            stuffedBuf[j++] = ESCAPE;
            stuffedBuf[j++] = buf[i] ^ 0x20; 
        } else {
            stuffedBuf[j++] = buf[i];
        }
    }

    *stuffedSize = j;
    return stuffedBuf;
}


unsigned char* byte_destuff(const unsigned char* buf, int bufSize, int* destuffedSize) {

    int i = 0;
    int size = 0;
    while (i < bufSize) {
        if (buf[i] == 0x5d && buf[i + 1] == 0x7c) {
            size++;
            i += 2; 
        } else if (buf[i] == 0x5d && buf[i + 1] == 0x7d) {
            size++;
            i += 2; 
        } else {
            size++;
            i++;
        }
    }

    unsigned char *destuffedBuf = (unsigned char *)malloc(size);

    i = 0;
    int j = 0;
    while (i<bufSize) {
        if (buf[i] == 0x5d && buf[i + 1] == 0x7c) {
            destuffedBuf[j] = 0x5c;
            j++;
            i += 2; 
        } else if (buf[i] == 0x5d && buf[i + 1] == 0x7d) {
            destuffedBuf[j] = 0x5d;
            j++;
            i += 2; 
        } else {
            destuffedBuf[j] = buf[i];
            j++;
            i++;
        }
    }

    *destuffedSize = size;
    return destuffedBuf;
}

unsigned char* framing(const unsigned char* buf, int bufSize, int* framedSize, unsigned char control_field) {
    // Calcula o BCC2 do buffer original
    unsigned char bcc2 = calculate_bcc2(buf, bufSize);
    printf("Buffer original: ");
    for (int i = 0; i < bufSize; i++) {
        printf("%02x ", buf[i]);
    }
    // Aloca memória para o novo buffer com espaço para o BCC2
    unsigned char* to_stuff = (unsigned char*)malloc(bufSize + 1);
    if (to_stuff == NULL) {
        *framedSize = -1;
        return NULL;
    }

    // Copia o buffer original para o novo buffer
    memcpy(to_stuff, buf, bufSize);

    // Adiciona o BCC2 ao final do buffer
    to_stuff[bufSize] = bcc2;

    // Byte stuff no buffer completo (com o BCC2)
    printf("Buffer antes do byte stuffing: ");
    for (int i = 0; i < bufSize; i++) {
        printf("%02x ", to_stuff[i]);
    }
    printf("\n");
    int stuffedSize;
    unsigned char* stuffedBuf = byte_stuff(to_stuff, bufSize + 1, &stuffedSize);
    if (stuffedBuf == NULL) {
        *framedSize = -1;
        free(to_stuff);
        return NULL;
    }
    printf("Buffer depois do byte stuffing: ");
    for (int i = 0; i < stuffedSize; i++) {
        printf("%02x ", stuffedBuf[i]);
    }
    // Aloca memória para o buffer do quadro
    *framedSize = stuffedSize + 6; // 7 bytes para FLAG, ADDR, CONTROL, BCC2 e FLAG
    unsigned char* framedBuf = (unsigned char*)malloc(*framedSize);
    if (framedBuf == NULL) {
        *framedSize = -1;
        free(stuffedBuf);
        return NULL;
    }

    // Adiciona os cabeçalhos ao início do quadro
    framedBuf[0] = FLAG; // Flag de início
    framedBuf[1] = ADDR_TRANSMITTER; // Endereço do transmissor
    framedBuf[2] = control_field; // Campo de controle
    framedBuf[3] = ADDR_TRANSMITTER ^ control_field; // BCC1
    framedBuf[4] = FLAG; // Flag de fim de cabeçalho

    // Copia o conteúdo do buffer "stuffed" para o quadro
    memcpy(framedBuf + 5, stuffedBuf, stuffedSize);

    // Adiciona a tail ao final do quadro
    framedBuf[*framedSize - 1] = FLAG; // Flag de fim

    // Libera a memória alocada para o buffer "stuffed"
    printf("Framed Frame: ");
    for (int i = 0; i < *framedSize; i++) {
        printf("%02x ", framedBuf[i]);
    }
    printf("\n");
    free(stuffedBuf);

    // Retorna o buffer do quadro
    return framedBuf;
}
