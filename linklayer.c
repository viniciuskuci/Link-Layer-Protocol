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
#include "byte_stuff.h"
#include "reciever_sm_disc.h"


int llopen(linklayer connectionParameters){
    
    int fd;
    char buf[255];
    int i, sum = 0, speed = 0;
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
    memset(buf,0,255);


    if(connectionParameters->role==0){ //transmitter
        StateMachine_s sm = NewStateMachine();
        if (Set(fd, DEBUG) == -1){
            perror("Set");
            return -1;
            }
        
        while(read(fd,buf,1)==1){

           if (UpdateState_s(buf[0], &sm, bool DEBUG)==1) break; // recebeu ua, sai do ciclo e retorna 1

        }
    }

    else if(connectionParameters->role==1){ //receiver

        StateMachine sm = NewStateMachine();
        while (STOP==FALSE) {       /* loop for input */
        res = read(fd,buf,1);   /* returns after 5 chars have been input */  
        if (res == -1){
            perror("read");
            exit(-1);
        }
        int sm_res;
        sm_res = UpdateState(buf[0], &sm, fd, DEBUG);
        if(sm_res == 1) {
            printf("Header OK, can send the ack\n");
        }
        
    }

    }
    else{
        printf("ERROR:Role not specified!\n");
        return -1;
    }

// envia o set e recebe o ua dependendo se e o sender ou o receiver
return 1;
}

llwrite(char* buf, int bufSize){

    StateMachine_s sm = NewStateMachine();
    sm->expected_frame = RR_REJ_frame;
    UpdateState_s(buf[0], &sm, bool DEBUG);
    return 1;
}

llread(char* packet){

    StateMachine sm = NewStateMachine();
    sm->expected_frame=INFORMATION;
    int sm_res;
    sm_res = UpdateState(buf[0], &sm, fd, DEBUG);
    if(sm_res != 1) return -1;
    return 1;

}

llclose(linkLayer connectionParameters, int showStatistics){
    
    if(connectionParameters->role==0){
        StateMachine_s smf = DiscStateMachine(); // cria uma state machine para receber disc do recetor


    if (Send_Termination(fd, DEBUG) == -1){ // envia o disc ao recetor
        perror("Disc"); 
        exit(-1);
    }
    sleep(1);

    while(read(fd,buf,1) == 1){
        printf("Reading\n");  // le a resposta do receiver e depois envia ua... , falta a state machine para receber ambos
        if(ShortUpdateState(buf[0], &smf,DEBUG)){
            if (Set(fd, &sm, DEBUG) == -1){
                perror("Ua"); 
                exit(-1);
                }
        }
    }
    sleep(1);
    }

    else if(connectionParameters->role==1){ //colocar disc receiver
         
        STOP=FALSE;
        StateMachine sm = NewStateMachine();
        sm->expected_frame=DISC_frame;
        while (STOP==FALSE) {       /* loop for input */
        res = read(fd,buf,1);   /* returns after 5 chars have been input */  
        if (res == -1){
            perror("read");
            exit(-1);
        }
        int sm_res;
        sm_res = UpdateState(buf[0], &sm, fd, DEBUG);

    }
    }

    else return -1;

    if(showStatistics==TRUE){
        printf("%d\n",connectionParameters->numTries);// estatistica?
    }

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 1;
}
