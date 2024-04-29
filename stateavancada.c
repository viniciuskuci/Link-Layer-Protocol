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
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
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
    newtio.c_cc[VMIN]     = 1;  /* blocking read until 5 chars received */



    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

   
    
int f,a;

while(STOP==FALSE)
{
    res=read(fd,buf,1);
    printf("%x\n",buf[0]);
    switch (buf[0])
{
case 0x5c: 
          if (f==0 && a==0) {
            f=1;
            printf("%x\n",buf[0]);
            }
// se ainda nao tiver sido lida a flag ou qualquer outro campo e a flag for identificada, ativar o campo f.
          else if (a==3) {
            printf("%x\n",buf[0]);
            STOP=TRUE;
            printf("ack received!\n");
            } //se for identificada a flag e tiverem sido verificados todos os outros campos, termina o ciclo.

          else { //se for registada uma flag no lugar errado da sequencia, recomeça
           f=1;
           a=0;
           }

break;

case 0x01:
           if(a==0) {
            a=1;
            printf("%x\n",buf[0]);
            }

           else { //erro na sequencia
                  a=0;
                  f=0;
                  }
break;

case 0x06:
         if(a==1) {
            a=2;
            printf("%x\n",buf[0]);
            } // sempre que se verifique o nr esperado da sequencia, a é incremnetado.
         else
        { 
          a=0;
          f=0;
        }
break;

case 0x07:
        if(a==2) {
            a=3;
            printf("%x\n",buf[0]);
            }
        else{
             a=0;
             f=0;
             }
break;

default:
        a=0;
        f=0;
        
break;
}

}

STOP=FALSE;
a=0;
f=0;
int b=0, d=0;

while(STOP==FALSE)
{
    res=read(fd,buf,1);
    printf("%x\n",buf[0]);

 switch (buf[0])
{
case 0x5c: 
 if (f==0 && a==0) {
    f=1;
    printf("%x\n",buf[0]);
    }
// se ainda nao tiver sido lida a flag ou qualquer outro campo e a flag for identificada, ativar o campo f.
 else if (a==3) {
    printf("%x\n",buf[0]);
    STOP=TRUE;
    printf("ack received!\n");
    } //se for identificada a flag e tiverem sido verificados todos os outros campos, termina o ciclo.

 else { //se for registada uma flag no lugar errado da sequencia, recomeça
     f=1;
     a=0;
     }
  break;
case 0x01:
           if(a==0) {
            a=1;
            printf("%x\n",buf[0]);
            }

           else { //erro na sequencia
                  a=0;
                  f=0;
                }
break;

case 0x06:
         if(a==1) {
            a=2;
            printf("%x\n",buf[0]);
            } // sempre que se verifique o nr esperado da sequencia, a é incremnetado.

         else {
            a=0;
            f=0;
            }
break;

case 0x07:
        if(a==2) {
            a=3;
            printf("%x\n",buf[0]);
            }

        else{
             a=0;
             f=0;
             }
break;

case d:
llopen();
while(llread()>0); // ciclo de leitura, falta acabar a parametrizacao da funcao
d=1;
break;

case bcc2:
     if(bcc2==TRUE) b=1;
     else{
        printf("Lost data, aasking to retransmmit...\n");
        d=0;
        //enviar o erro ao transmiter de alguma forma...
        b=0;
     }
break;

default:
        a=0;
        f=0;
        
break;
}

}

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 0;
}
