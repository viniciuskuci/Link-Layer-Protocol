/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "sender_sm.h"
#include <stdbool.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define DEBUG true

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

    if ( (argc < 3) ||
         ((strcmp("/dev/ttyS10", argv[1])!=0) &&
          (strcmp("/dev/ttyS11", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }


    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
    memset(buf,0,255);
   

    StateMachine sm = NewStateMachine();
    if (Set(fd, &sm, DEBUG) == -1){
        perror("Set"); //usar timer aqui?
        exit(-1);
    }
    sleep(1);

    //pegar o input do usuario:
    char* input = argv[2];
    printf("Input: %s\n", input);
    buf[0] = FLAG;
    buf[1] = ADDR_TRANSMITTER;
    buf[2] = I0;
    buf[3] = ADDR_TRANSMITTER^buf[2];
    buf[4] = FLAG;
    for (int i = 0; i < strlen(input); i++){
        buf[i+5] = input[i];
    }

    unsigned char bcc2;
    for (int i = 0; i < strlen(input); i++){
        bcc2 ^= input[i];
    }

    buf[strlen(input)+5] = bcc2;
    buf[strlen(input)+6] = FLAG;

    while(read(fd,buf,1) == 1){
        printf("Reading\n");
        if (UpdateState(buf[0], &sm, DEBUG) == 1){
            write(fd, buf, strlen(input)+7);
            break;
        }
    }


    
   
    
    

    sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 0;
}
