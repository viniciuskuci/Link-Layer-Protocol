#include "linklayer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "sender_sm_disc.h"

int llopen(linklayer connectionParameters){
    
    int fd;
    struct termios oldtio,newtio;
     fd = open(connectionParameters->serialPort, O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }


    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters->baudrate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    cflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");


    if(connectionParameters->role==0){
        // transmitter
        StateMachine sm = NewStateMachine();
    if (Set(fd, DEBUG) == -1){
        perror("Set");
        return -1;
    }
    }
    
    else if(connectionParameters->role==1){
        //receiver
    }
    else{
        printf("ERROR:Role not specified!\n");
        return -1;
    }

// envia o set e recebe o ua dependendo se e o sender ou o receiver
return 1;
}

llwrite(char* buf, int bufSize){

}
