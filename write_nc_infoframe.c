/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "statemachine.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;
int flag=1, conta=1;

void atende()                   // atende alarme
{
    printf("alarme # %d\n", conta);
    flag=1;
    conta++;
}
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

    printf("New termios structure set\n");
    memset(buf,0,255);
    buf[0] = FLAG;
    buf[1] = ADDR_TRANSMITTER;
    buf[2] = SET;
    buf[3] = buf[1]^buf[2];
    buf[4] = FLAG;
    buf[5] = '\0';
    
    res = write(fd,buf,5);
    
    printf("%d bytes written\n", res);

    /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
    */

    sleep(1);
   
    
int f,a;
    while(STOP==FALSE)
{
res=read(fd,buf,1);

printf("%x\n",buf[0]);

 switch (buf[0])
{
case FLAG: 
 if (f==0 && a==0) {f=1;
printf("%x\n",buf[0]);}
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

case ADDR_TRANSMITTER:
           if(a==0) {a=1;
printf("%x\n",buf[0]);
}
           else { //erro na sequencia
                  a=0;
                  f=0;
}
break;

case UA:
         if(a==1) {a=2;
printf("%x\n",buf[0]);} // sempre que se verifique o nr esperado da sequencia, a é incremnetado.
         else
        { 
          a=0;
          f=0;
        }
break;

case 0x07:
        if(a==2) {a=3;
printf("%x\n",buf[0]);}
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
llopen();
STOP=FALSE;
a=0;
f=0;
buf[0] = FLAG;
buf[1] = ADDR_TRANSMITTER;
buf[2] = 0x80;
buf[3] = buf[1]^buf[2];
buf[4] = d12;
buf[5] = FLAG;
buf[6] = '\0';
write(fd,buf,2);
 while(STOP==FALSE)
{
res=read(fd,buf,1);

printf("%x\n",buf[0]);

 switch (buf[0])
{
case FLAG: 
 if (f==0 && a==0) {f=1;
printf("%x\n",buf[0]);}
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

case ADDR_TRANSMITTER:
           if(a==0) {
a=1;
printf("%x\n",buf[0]);
}
           else { //erro na sequencia
                  a=0;
                  f=0;
}
break;

case UA:
         if(a==1) {a=2;
printf("%x\n",buf[0]);} // sempre que se verifique o nr esperado da sequencia, a é incremnetado.
         else
        { 
          a=0;
          f=0;
        }
break;

case 0x07:
        if(a==2) {a=3;
printf("%x\n",buf[0]);}
        else{
             a=0;
             f=0;
}
case 0x11:
//envia a proxima i
//se nao ha mais is, passa para o campo bcc2
break;


default:
        a=0;
        f=0;
        (void) signal(SIGALRM, atende);  // instala rotina que atende interrupcao

    while (conta < 4) {
        if (flag) {
            alarm(3);  // activa alarme de 3s
            flag=0;
        }
    }
    printf("Vou terminar.\n");
break;
}
}
STOP=FALSE;
a=0;
f=0;
int rr=0;
buf[0] = FLAG;
buf[1] = ADDR_TRANSMITTER;
buf[2] = 0xc0;
buf[3] = buf[1]^buf[2];
buf[4] = d12;
buf[5] = FLAG;
buf[6] = '\0';
write(fd,buf,2);

while(STOP==FALSE)
{
res=read(fd,buf,1);

printf("%x\n",buf[0]);

 switch (buf[0])
{
case FLAG: 
 if (f==0 && a==0) {f=1;
printf("%x\n",buf[0]);}
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

case ADDR_TRANSMITTER:
           if(a==0) {
a=1;
printf("%x\n",buf[0]);
}
           else { //erro na sequencia
                  a=0;
                  f=0;
}
break;

case UA:
         if(a==1) {a=2;
printf("%x\n",buf[0]);} // sempre que se verifique o nr esperado da sequencia, a é incremnetado.
         else
        { 
          a=0;
          f=0;
        }
break;

case 0x07:
        if(a==2) {a=3;
printf("%x\n",buf[0]);}
        else{
             a=0;
             f=0;
}
case 0x01:
//envia a proxima i
if(a==3) {
rr=1;
STOP=TRUE;
}

break;


default:
        a=0;
        f=0;
    (void) signal(SIGALRM, atende);  // instala rotina que atende interrupcao
    while (conta < 4) {
        if (flag) {
            alarm(3);  // activa alarme de 3s
            flag=0;
        }
    }
    printf("Vou terminar.\n"); 
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
