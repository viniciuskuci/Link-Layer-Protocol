/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    SM_STOP
} State;

typedef struct {
    State state;
} StateMachine;



void reply(int fd, unsigned char *buf){
    printf("Sending response...\n");
    int replySize = strlen(buf);
    int res = write(fd, buf, replySize);
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
        printf("%x ", buf[0]);
        printf("\n");   

        switch(buf[0]){
            case 0x5c:
                if (sm.state == START){
                    printf("Received %x and the machine is on state START\n", buf[0]);
                    sm.state = FLAG_RCV;
                    break;
                }
                else if (sm.state == BCC_OK){
                    printf("BCC OK\n");
                    sm.state = START;           /*provisório*/
                    break;
                }
                break;
            case 0x03:
                if (sm.state == FLAG_RCV){
                    printf("Received %x and the machine is on state FLAG_RCV\n",  buf[0]);
                    sm.state = A_RCV;
                }
                break;
            case 0x08:
                if (sm.state == A_RCV){
                    printf("Received %x and the machine is on state A_RCV\n", buf[0]);
                    sm.state = C_RCV;
                }
                break;
            default:
                if (sm.state == C_RCV){
                    if (buf[0] == (0x03^0x08)){
                        printf("Received the BCC %x and the machine is on state C_RCV\n", buf[0]);
                        sm.state = BCC_OK;
                        break;
                    }
                    else{
                        sm.state = START;
                        break;
                    } 
                }
                else{
                    sm.state = START;
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
