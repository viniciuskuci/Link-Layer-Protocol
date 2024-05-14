#include "sender_sm.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>


StateMachine NewStateMachine(){
    StateMachine sm;
    sm.state = START;
    sm.expected_RR_flag = RR1;
    sm.expected_REJ_flag = REJ1;
    sm.packet_rejected = false;
    sm.expected_frame =  UA_frame; 
    return sm;
}

StateMachine DiscStateMachine(){

    StateMachine sm;
    sm.state = START;
    sm.expected_frame = DISC_frame;
    sm.expected_RR_flag = NULL;
    sm.expected_REJ_flag = NULL;
    sm. packet_rejected = false;
    return sm;
}

int Set(int fd, StateMachine *sm, bool DEBUG){ //sm nao é utilizado
    unsigned char set[5];
    set[0] = FLAG;
    set[1] = ADDR_TRANSMITTER;
    set[2] = SET;
    set[3] = ADDR_TRANSMITTER^SET;
    set[4] = FLAG;
    if (DEBUG) printf("SET frame created %x %x %x %x %x\n", set[0], set[1], set[2], set[3], set[4]);
    if (write(fd, set, 5) == -1){
        perror("write");
        return -1;
    }
    return 0;
}
int SendResponse(int fd, StateMachine* sm, unsigned char control_flag, bool DEBUG){ // sm nao é utilizado...
    
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
    if (DEBUG) printf("Sent %d bytes\n", res);
    return 0;
}

int UpdateState(unsigned char byte, StateMachine* sm, bool DEBUG){

    switch(sm->state){

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
            if (sm->expected_frame == UA_frame){
                if (byte == UA){
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
            if (byte == sm->expected_RR_flag){
                sm->state = C_RCV; //mudar flags depois de receber o frame
                if (DEBUG) printf("-> Transitioned from A_RCV to C_RCV\n");
                return 0;
            }
            else if (byte == sm->expected_REJ_flag){
                sm->packet_rejected = true;
                sm->state = C_RCV; //mudar flags depois de receber o frame retransmitir
                if (DEBUG) printf("-> Packet rejected. Transitioned from A_RCV to C_RCV\n");
                return -1;
            }
            else{
                sm->state = START;
                if (DEBUG) printf("X Garbage recieved (%x). Transitioned from A_RCV to START\n", byte);
                return -1;
            }
        
        case C_RCV:
            if (sm->expected_frame == UA_frame){
                if (byte == (UA^ADDR_TRANSMITTER)){
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
            else if (sm->packet_rejected && (byte == (ADDR_TRANSMITTER^sm->expected_REJ_flag)))
            {
                sm->state = BCC_OK;
                if (DEBUG) printf("-> Transitioned from C_RCV to BCC_OK\n");
                return 0;
            }
            else if (!sm->packet_rejected && (byte == (ADDR_TRANSMITTER^sm->expected_RR_flag)))
            {
                sm->state = BCC_OK;
                if (DEBUG) printf("-> Transitioned from C_RCV to BCC_OK\n");
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

int Ua(int fd, bool DEBUG){ 
    unsigned char ua[5];
    ua[0] = FLAG;
    ua[1] = ADDR_TRANSMITTER;
    ua[2] = UA;
    ua[3] = ADDR_TRANSMITTER^UA;
    ua[4] = FLAG;
    if (DEBUG) printf("SET frame created %x %x %x %x %x\n", ua[0], ua[1], ua[2], ua[3], ua[4]);
    if (write(fd, ua, 5) == -1){
        perror("write");
        return -1;
    }
    return 0;
}

