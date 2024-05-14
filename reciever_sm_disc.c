#include "reciever_sm.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>


StateMachine NewStateMachine(){
    StateMachine sm;
    sm.state = START;
    sm.expected_frame = SUPERVISION;
    sm.expected_I_flag = I0;
    sm.last_byte = 0x00;
    sm.bcc2 = 0x00;
    sm.bytes_downloaded = 0;
    return sm;
}

StateMachine DiscStateMachine(){

    StateMachine sm;
    sm.state = START;
    sm.expected_frame = DISC_frame;
    sm. packet_rejected = false;
    return sm;
}

int SendResponse(int fd, StateMachine* sm, unsigned char control_flag, bool DEBUG){
    
    char response[5];
    response[0] = FLAG;
    response[1] = ADDR_TRANSMITTER;
    response[2] = control_flag;
    response[3] = ADDR_TRANSMITTER^response[2];
    response[4] = FLAG;
    int res = write(fd, response, 5);
    if (res == -1){
        perror("write");
        return -1;
    }
    if (DEBUG) printf("Sent %d bytes: %x %x %x %x %x\n", res, response[0], response[1], response[2], response[3], response[4]);
    return 0;
}

int UpdateState(unsigned char byte, StateMachine* sm, int fd, bool DEBUG){

    if (sm->expected_frame == SUPERVISION)
    {
        if (DEBUG) printf("SUPERVISION FRAME: %x ", byte);

        switch(sm->state)
        {
            case START:
                if (byte == FLAG)
                {
                    sm->state = FLAG_RCV;
                    if (DEBUG) printf("-> Transitioned from START to FLAG_RCV\n");
                    return 0;
                }
                else
                {
                    if (DEBUG) printf("X Garbage recieved (%x). Still on START state\n", byte);
                    return -1;
                }

            case FLAG_RCV:
                if (byte == ADDR_TRANSMITTER)
                {
                    sm->state = A_RCV;
                    if (DEBUG) printf("-> Transitioned from FLAG_RCV to A_RCV\n");
                    return 0;
                }
                else if (byte == FLAG)
                {
                    if (DEBUG) printf("Garbage recieved (%x). Still on FLAG_RCV state\n", byte);
                    return 0;
                }
                else
                {
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from FLAG_RCV to START\n", byte);
                    return -1;
                }

            case A_RCV:
                if (byte == SET)
                {
                    sm->state = C_RCV;
                    if (DEBUG) printf("-> Transitioned from A_RCV to C_RCV\n");
                    return 0;
                }
                else
                {
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from A_RCV to START\n", byte);
                    return -1;
                }
            
            case C_RCV:
                if (byte == (ADDR_TRANSMITTER^SET))
                {
                    sm->state = BCC_OK;
                    if (DEBUG) printf("-> BCC1 OK! (%x) Transitioned from C_RCV to BCC_OK\n", byte);
                    return 0;
                }
                else
                {
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from C_RCV to START\n", byte);
                    return -1;
                }
            
            case BCC_OK:
                if (byte == FLAG)
                {   
                    sm->expected_frame = INFORMATION;
                    sm->state = START;
                    if (DEBUG) printf("-> SET frame correctly recieved. Transitioned from BCC_OK to START\n");
                    SendResponse(fd, sm, UA, true); //incluir disc.
                    return 0;
                }
                else
                {
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from BCC_OK to START\n", byte);
                    return -1;
                }

            default:
                if (DEBUG) printf("X Garbage recieved (%x). Transitioned to START\n", byte);
                sm->state = START;
                return -1;
        }     
    }

    else if (sm->expected_frame == INFORMATION)
    {
        if (DEBUG) printf("INFORMATION FRAME: %x ", byte);

        switch (sm->state)
        {
            
            case START:
                if (byte == FLAG)
                {
                    sm->state = FLAG_RCV;
                    if (DEBUG) printf("-> Transitioned from START to FLAG_RCV\n");
                    return 0;
                }
                else
                {
                    if (DEBUG) printf("X Garbage recieved (%x). Still on START state\n", byte);
                    return -1;
                }
            
            case FLAG_RCV:
                if (byte == ADDR_TRANSMITTER)
                {
                    sm->state = A_RCV;
                    if (DEBUG) printf("-> Transitioned from FLAG_RCV to A_RCV\n");
                    return 0;
                }
                else if (byte == FLAG)
                {
                    if (DEBUG) printf("X Garbage recieved (%x). Still waiting from FLAG_RCV state\n", byte);
                    return 0;
                }
                else
                {
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from FLAG_RCV to START\n", byte);
                    return -1;
                }
            
            case A_RCV:
                if (byte == I0 && sm->expected_I_flag == I0)
                {
                    sm->state = C_RCV;
                    if (DEBUG) printf("-> Transitioned from A_RCV to C_RCV\n");
                    return 0;
                }
                else if (byte == I1 && sm->expected_I_flag == I1)
                {
                    sm->state = C_RCV;
                    if (DEBUG) printf("-> Transitioned from A_RCV to C_RCV\n");
                    return 0;
                }
                else
                {
                    sm->state = START;
                    if (DEBUG) printf("X Duplicated information frame control recieved (%x). Transitioned from A_RCV to START\n", byte);
                    return -1;
                }
            
            case C_RCV:
                if (byte == (ADDR_TRANSMITTER^sm->expected_I_flag))
                {
                    sm->state = BCC_OK;
                    if (DEBUG) printf("-> BCC1 OK! (%x) Transitioned from C_RCV to BCC1_OK\n", byte);
                    return 0;
                }
                else
                {
                    SendResponse(fd, sm, (sm->expected_I_flag == I0) ? REJ1 : REJ0, DEBUG);
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from C_RCV to START\n", byte);
                    return -1;
                }
            
            case BCC_OK:
                if (byte == FLAG)
                {   
                    sm->bytes_downloaded = 0;
                    sm->bcc2 = 0x00;
                    sm->last_byte = 0x00;
                    sm->state = DATA_TRANSFER;
                    if (DEBUG) printf("-> Information header correctly recieved. Transitioned from BCC_OK to DATA_TRANSFER\n");
                    return 0;
                }
                else
                {
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from BCC_OK to START\n", byte);
                    return -1;
                }
            
            case DATA_TRANSFER:
                if (byte == FLAG)
                {   
                    SendResponse(fd, sm, (sm->expected_I_flag == I0) ? RR1 : RR0, DEBUG);
                    sm->expected_I_flag = (sm->expected_I_flag == I0) ? I1 : I0;
                    sm->state = START;
                    if (DEBUG) printf("-> Correctly recieved %d bytes. Transitioned from DATA_TRANSFER to START\n", sm->bytes_downloaded);
                    return 0;
                }
                else
                {
                    sm->bcc2 ^= sm->last_byte;
                    sm->last_byte = byte;
                    sm->bytes_downloaded++;
                    if (DEBUG) printf("-> Data (%c)\n", byte);
                    return 0;
                }
            
            default:
                sm->state = START;
                if (DEBUG) printf("X Garbage recieved (%x). Transitioned to START\n", byte);
                return -1;
        }
    }       

}

int Send_Termination(int fd, bool DEBUG){

    unsigned char disc[5];
    disc[0] = FLAG;
    disc[1] = ADDR_TRANSMITTER;
    disc[2] = DISC;
    disc[3] = ADDR_TRANSMITTER^DISC;
    disc[4] = FLAG;
    if (DEBUG) printf("DISC frame created %x %x %x %x %x\n", disc[0], disc[1], disc[2], disc[3], disc[4]);
    if (write(fd, disc, 5) == -1){
        perror("write");
        return -1;
    }
    return 0;
}

int ShortUpdateState(unsigned char byte, StateMachine* sm, bool DEBUG){

    switch (sm->state)
    {
    
        case START:
            if (byte == FLAG){
                sm->state = FLAG_RCV;
                if (DEBUG) printf("-> Transitioned from START to FLAG_RCV\n");
                return 0;
            }
            else{
                if (DEBUG) printf("X Garbage recieved (%x). Still on START state\n", byte);
                return -1;
            }
        break;

         case FLAG_RCV:
            if (byte == ADDR_TRANSMITTER){
                sm->state = A_RCV;
                if (DEBUG) printf("-> Transitioned from FLAG_RCV to A_RCV\n");
                return 0;
            }
            else if (byte == FLAG){
                if (DEBUG) printf("Garbage recieved (%x). Still on FLAG_RCV state\n", byte);
                return 0;
            }
            else{
                sm->state = START;
                if (DEBUG) printf("X Garbage recieved (%x). Transitioned from FLAG_RCV to START\n", byte);
                return -1;
            }

          case A_RCV:

            if(byte == FLAG){ // se receber uma flag volta para o rcv flag e nao para start 
                sm->state = FLAG;
                if (DEBUG) printf("Garbage recieved(%x). Transitioned from A_RCV to FLAG_RCV\n", byte);
                return 0;
            }
            if (sm->expected_frame == DISC_frame){
                if (byte == DISC){
                    sm->state = C_RCV;
                    if (DEBUG) printf("-> Transitioned from A_RCV to C_RCV\n");
                    return 0;
                }
                else{
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). UA frame expected. Transitioned from A_RCV to START\n", byte);
                    return -1;
                }
            }

             case C_RCV:
              if(byte == FLAG){ // se receber uma flag volta para o rcv flag e nao para start 
                sm->state = FLAG;
                if (DEBUG) printf("Garbage received (%x). Transitioned from C_RCV to FLAG_RCV\n", byte);
                return 0;
              }
              if (sm->expected_frame == DISC_frame){
                if (byte == (DISC^ADDR_TRANSMITTER)){
                    sm->state = BCC_OK;
                    if (DEBUG) printf("-> Transitioned from C_RCV to BCC_OK\n");
                    return 0;
                }
                else{
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from C_RCV to START\n", byte);
                    return -1;
                }
            }
           
      case BCC_OK:
            if (byte == FLAG)
            {   
                if (sm->packet_rejected)
                {
                    //retransmitir aqui
                    sm->packet_rejected = false;
                    sm->state = START;
                    if (DEBUG) printf("-> Retransmited packet. Transitioned from BCC_OK to START\n");
                    return 1;
                }
                else if (sm->expected_frame == UA_frame){
                    sm->expected_frame = RR_REJ_frame;
                    sm->state = START;
                    if (DEBUG) printf("-> UA recieved. Transitioned from BCC_OK to START\n");
                    return 1;
                }
                else
                {
                    sm->expected_REJ_flag = (sm->expected_REJ_flag == REJ0) ? REJ1 : REJ0;
                    sm->expected_RR_flag = (sm->expected_RR_flag == RR0) ? RR1 : RR0;
                    sm->state = START;
                    if (DEBUG) printf("-> ACK ok! Transitioned from BCC_OK to START\n");
                    return 1;
                }   
            }

            else
            {
                sm->state = START;
                if (DEBUG) printf("X Garbage recieved (%x). Transitioned from BCC_OK to START\n", byte);
                return -1;
            }
    }
}