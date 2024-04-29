#include "reciever_sm.h"
#include <stdio.h>


int UpdateState(char byte, StateMachine* sm){

    switch(byte){
            case FLAG:
                if (sm->state == START){
                    sm->state = FLAG_RCV;    //se a maquina estiver no estado START e receber uma flag, passa para o estado FLAG_RCV
                    return 0;
                }
                else if (sm->state == BCC_OK){
                    if (sm->set == TRUE){
                        sm->set = FALSE; //fim do cabecalho. retornar 1 indica que esta pronto para enviar o UA
                        sm->state = START;
                        return 1;
                        
                    } 
                    sm->state = SM_STOP;     //se a maquina estiver no estado BCC_OK e receber uma flag, passa para o estado SM_STOP   
                    return 0;
                }
                else if (sm->state == SM_STOP){
                    sm->state = START;       //se a maquina estiver no estado SM_STOP e receber uma flag, passa para o estado START
                }
                return 0;
            case ADDR_TRANSMITTER:
                if (sm->state == FLAG_RCV){
                    sm->state = A_RCV;
                }
                return 0;
            case SET:
                if (sm->state == A_RCV){
                    sm->set = TRUE;
                    sm->state = C_RCV;
                }
                return 0;
            default:
                if (sm->state == C_RCV){
                    if (byte == (ADDR_TRANSMITTER^SET)){
                        printf("BCC OK\n");
                        sm->state = BCC_OK;
                        return 0;
                    }
                    else{
                        sm->state = START;
                        return -2; //rejeitado pois o bcc estava incorreto
                    } 
                }
                else if (sm->state == SM_STOP){
                    printf("%x ", byte);         //se a maquina estiver no estado SM_STOP, imprime o que recebeu
                    return 0;
                }                
                else{
                    sm->state = START;       //reset da maquina de estados. acontece sempre que recebe algo que nao espera
                    return -1;
                }
        }

}