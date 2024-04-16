/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "statemachine.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;


void ack(int fd){
    printf("Sending ack...\n");
    char ack_buf[5];
    ack_buf[0] = FLAG;
    ack_buf[1] = ADDR_TRANSMITTER;
    ack_buf[2] = UA;
    ack_buf[3] = ADDR_TRANSMITTER^UA;
    ack_buf[4] = FLAG;
    int res = write(fd, ack_buf, 5);
    printf("Sent %d bytes\n", res);
    return;
}

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) ||
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

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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

    StateMachine sm;
    sm.state = START;

    while (STOP==FALSE) {       /* loop for input */
        res = read(fd,buf,1);   /* returns after 5 chars have been input */  
        if (res == -1){
            perror("read");
            exit(-1);
        }
        switch(buf[0]){
            case FLAG:
                if (sm.state == START){
                    sm.state = FLAG_RCV;    //se a maquina estiver no estado START e receber uma flag, passa para o estado FLAG_RCV
                    break;
                }
                else if (sm.state == BCC_OK){
                    if (sm.set == TRUE){
                        ack(fd);     //se a maquina estiver no estado SM_STOP e o set for TRUE, envia a resposta e volta ao estado START
                        sm.set = FALSE;
                        sm.state = START;
                        break;
                    } 
                    sm.state = SM_STOP;     //se a maquina estiver no estado BCC_OK e receber uma flag, passa para o estado SM_STOP   
                    break;
                }
                else if (sm.state == SM_STOP){
                    sm.state = START;       //se a maquina estiver no estado SM_STOP e receber uma flag, passa para o estado START
                }
                break;
            case ADDR_TRANSMITTER:
                if (sm.state == FLAG_RCV){
                    sm.state = A_RCV;
                }
                break;
            case SET:
                if (sm.state == A_RCV){
                    sm.set = TRUE;
                    sm.state = C_RCV;
                }
                break;
            default:
                if (sm.state == C_RCV){
                    if (buf[0] == (ADDR_TRANSMITTER^SET)){
                        printf("BCC OK\n");
                        sm.state = BCC_OK;
                        break;
                    }
                    else{
                        sm.state = START;
                        break;
                    } 
                }
                else if (sm.state == SM_STOP){
                    printf("%x ", buf[0]);         //se a maquina estiver no estado SM_STOP, imprime o que recebeu
                    break;
                }                
                else{
                    sm.state = START;       //reset da maquina de estados. acontece sempre que recebe algo que nao espera
                    break;
                }
        }


        //if (buf[0]=='z') STOP=TRUE;
    }



    /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião
    */
    sleep(1);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
