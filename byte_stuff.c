#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "byte_stuff.h"

int check_bcc2(const unsigned char* buf, int bufSize) {
    unsigned char bcc2 = calculate_bcc2(buf, bufSize-1);
    printf("BCC2 on buffer: %02x\n", buf[bufSize-1]);
    return bcc2 == buf[bufSize-1];
}

unsigned char calculate_bcc2(const unsigned char* buf, int bufSize) {
    unsigned char bcc2 = 0x00;
    for (int i = 0; i < bufSize; i++) {
        bcc2 ^= buf[i];
    }
    printf("Calculated BCC2: %02x\n", bcc2);
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
    unsigned char bcc2 = calculate_bcc2(buf, bufSize);
    unsigned char* to_stuff = (unsigned char*)malloc(bufSize + 1);
    if (to_stuff == NULL) {
        *framedSize = -1;
        return NULL;
    }

    memcpy(to_stuff, buf, bufSize);

    to_stuff[bufSize] = bcc2;

   
    int stuffedSize;
    unsigned char* stuffedBuf = byte_stuff(to_stuff, bufSize + 1, &stuffedSize);
    if (stuffedBuf == NULL) {
        *framedSize = -1;
        free(to_stuff);
        return NULL;
    }
    
    *framedSize = stuffedSize + 6; 
    unsigned char* framedBuf = (unsigned char*)malloc(*framedSize);
    if (framedBuf == NULL) {
        *framedSize = -1;
        free(stuffedBuf);
        return NULL;
    }

    framedBuf[0] = FLAG; 
    framedBuf[1] = ADDR_TRANSMITTER; 
    framedBuf[2] = control_field; 
    framedBuf[3] = ADDR_TRANSMITTER ^ control_field; 
    framedBuf[4] = FLAG; 

    memcpy(framedBuf + 5, stuffedBuf, stuffedSize);

    framedBuf[*framedSize - 1] = FLAG; 

    printf("Framed Frame: ");
    for (int i = 0; i < *framedSize; i++) {
        printf("%c", framedBuf[i]);
    }
    printf("\n");
    free(stuffedBuf);

    return framedBuf;
}
