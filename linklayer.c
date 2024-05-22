#include "linklayer.h"
#include "byte_stuff.h"
#include "stm.h"
#include <stdio.h>

StateMachine sm;


int llopen(linkLayer connectionParameters) {
    struct termios oldtio, newtio;
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    if (tcgetattr(fd, &oldtio) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 1;

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    printf("New termios structure set\n");

    if (connectionParameters.role == TRANSMITTER) {
        sm = NewStateMachine(TRANSMITTER, fd);

        unsigned char reply;
        if (Set(fd, 0) == -1) {
            return -1;
        }
        printf("Set Message sent.\n");
        while (read(fd, &reply, 1) == 1) {
            int response = UpdateState(reply, &sm, NULL);
            if (response == 1) {
                return 1;
            } else {
                printf("Reply: %x\n", reply);
            }
        }
    } else {
        sm = NewStateMachine(RECEIVER, fd);

        unsigned char reply;
        while (read(fd, &reply, 1) == 1) {
            int response = UpdateState(reply, &sm, NULL);
            if (response == 1) {
                return 1;
            } else {
                printf("Reply: %x\n", reply);
            }
        }
    }

    return -1; // Caso nenhuma das condições seja satisfeita
}

int llwrite(unsigned char* buf, int bufSize){
    
    int frameSize;
    unsigned char *frame = framing(buf, strlen(buf), &frameSize, sm.next_I_flag);
    if (frame == NULL || frameSize < 0) {
        free(frame);
        return -1;
    }
    write(sm.fd, frame, frameSize);
    unsigned char reply;
    while (read(sm.fd, &reply, 1) == 1) {
        int response = UpdateState(reply, &sm, NULL);
        if (response == 1) {
            printf("Frame %s sent.\n", sm.next_I_flag == I0 ? "I0" : "I1");
            return 0; // Sucesso
        }
        else if(response==-1){
            printf("Frame %s rejected. Retransmitting...\n", sm.next_I_flag == I0 ? "I0" : "I1");
            write(sm.fd, frame, frameSize);
            sleep(1);
        }
    }

    free(frame);
    return -1; // Falha
}

int llread(unsigned char* packet){
    unsigned char *buffer[MAX_PAYLOAD_SIZE*2];
    unsigned char reply;
    while (read(sm.fd, &reply, 1) == 1) {
        int response = UpdateState(reply, &sm, buffer);
        if (response == 1) {
            memcpy(packet, buffer, MAX_PAYLOAD_SIZE);
            return sm.bytes_downloaded; // Sucesso
        }
    }

    return -1; // Falha
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
