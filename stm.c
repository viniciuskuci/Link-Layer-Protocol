#include "stm.h"

StateMachine NewStateMachine(int role, int fd){
    StateMachine sm;
    sm.state = START;
    sm.role = role;
    sm.fd = fd;
    if (role == TRANSMITTER){
        sm.next_I_flag = I0;
        sm.expected_RR_flag = RR1;
        sm.expected_REJ_flag = REJ1;
        sm.packet_rejected = false;
        sm.expected_frame = UA_frame;
    }
    else if (role == RECEIVER){
        sm.state = START;
        sm.expected_frame = SUPERVISION;
        sm.expected_I_flag = I0;
        sm.last_byte = 0x00;
        sm.bcc2 = 0x00;
        sm.bytes_downloaded = 0;
    }
    printf("New state machine created. Role: %s\n", (role == TRANSMITTER) ? "sender" : "receiver");
    return sm;
}

int Set(int fd, bool DEBUG){
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

int SendResponse(StateMachine* sm, unsigned char control_flag, bool DEBUG){
    
    char response[5];
    response[0] = FLAG;
    response[1] = ADDR_TRANSMITTER;
    response[2] = control_flag;
    response[3] = ADDR_TRANSMITTER^response[2];
    response[4] = FLAG;
    int res = write(sm->fd, response, 5);
    if (res == -1){
        perror("write");
        return -1;
    }
    if (DEBUG) printf("Sent %d bytes: %x %x %x %x %x\n", res, response[0], response[1], response[2], response[3], response[4]);
    return 0;
}


int UpdateState(unsigned char byte, StateMachine *sm, unsigned char *file_buffer)
{
    if (sm->role == RECEIVER)
    {
        if (sm->expected_frame == SUPERVISION)
        {
            switch (sm->state)
            {
                case START:
                    if (byte == FLAG)
                    {
                        sm->state = FLAG_RCV;
                        printf("START -> FLAG_RCV\n");
                    }
                    return 0;
                    
                case FLAG_RCV:

                    if (byte == ADDR_TRANSMITTER)
                    {
                        sm->state = A_RCV;
                        printf("FLAG_RCV -> A_RCV\n");
                    }
                    else if (byte != FLAG)
                    {
                        sm->state = START;
                        printf("FLAG_RCV -> START\n");
                    }
                    return 0;
                
                case A_RCV:
                    if (byte == FLAG)
                    {
                        sm->state = FLAG_RCV;
                        printf("A_RCV -> FLAG_RCV\n");
                    }
                    else if (byte == SET)
                    {
                        sm->state = C_RCV;
                        printf("A_RCV -> C_RCV\n");
                    }
                    else
                    {
                        sm->state = START;
                        printf("A_RCV -> START\n");
                    }
                    return 0;
                
                case C_RCV:

                    if (byte == FLAG)
                    {
                        sm->state = FLAG_RCV;
                        printf("C_RCV -> FLAG_RCV\n");
                    }
                    else if (byte == (ADDR_TRANSMITTER^SET))
                    {
                        sm->state = BCC_OK;
                        printf("C_RCV -> BCC_OK\n");
                    }
                    else
                    {
                        sm->state = START;
                        printf("C_RCV -> START\n");
                    }
                    return 0;
                
                case BCC_OK:
                    
                    if (byte == FLAG)
                    {   
                        SendResponse(sm, UA, true);
                        sm->expected_frame = INFORMATION;
                        sm->state = START;
                        printf("BCC_OK. READY FOR DATA -> START. EXPECTED FRAME INFORMATION\n");
                        return 1;
                    }
                    else
                    {
                        sm->state = START;
                        printf("BCC ERROR: BCC_OK -> START\n");
                        return 0;
                    }
                
                default:
                    sm->state = START;
                    printf("BBC_OK -> START\n");
                    return 0;
            }
        }

        else if (sm->expected_frame == INFORMATION)
        {
            switch (sm->state)
            {
                case START:
                    if (byte == FLAG)
                    {
                        sm->state = FLAG_RCV;
                        printf("START -> FLAG_RCV\n");
                    }
                    return 0;
                    
                case FLAG_RCV:

                    if (byte == ADDR_TRANSMITTER)
                    {
                        sm->state = A_RCV;
                        printf("FLAG_RCV -> A_RCV\n");
                    }
                    else if (byte != FLAG)
                    {
                        sm->state = START;
                        printf("FLAG_RCV -> START\n");
                    }
                    return 0;
                
                case A_RCV:
                    if (byte == FLAG)
                    {
                        sm->state = FLAG_RCV;
                        printf("A_RCV -> FLAG_RCV\n");
                    }
                    else if (byte == I0 && sm->expected_I_flag == I0)
                    {
                        sm->state = C_RCV;
                        printf("A_RCV -> C_RCV\n");
                    }
                    else if (byte == I1 && sm->expected_I_flag == I1)
                    {
                        sm->state = C_RCV;
                        printf("A_RCV -> C_RCV\n");
                    }
                    else
                    {
                        sm->state = START;
                        printf("A_RCV -> START\n");
                        //responder com reject
                    }
                    return 0;
                
                case C_RCV:

                    if (byte == FLAG)
                    {
                        sm->state = FLAG_RCV;
                        printf("C_RCV -> FLAG_RCV\n");
                    }
                    else if (byte == (ADDR_TRANSMITTER^sm->expected_I_flag))
                    {
                        sm->state = BCC_OK;
                        printf("C_RCV -> BCC_OK\n");
                    }
                    else
                    {
                        SendResponse(sm, (sm->expected_I_flag == I0) ? REJ0 : REJ1, true);
                        sm->state = START;
                        printf("C_RCV -> START\n");
                    }
                    return 0;
                
                case BCC_OK:
                    
                    if (byte == FLAG)
                    {   
                        sm->bytes_downloaded = 0;
                        sm->bcc2 = 0x00;
                        sm->last_byte = 0x00;
                        sm->state = SM_STOP;
                        printf("BCC_OK -> SM_STOP\n");
                    }
                    else
                    {
                        sm->state = START;
                        printf("EXPECTED FLAG. BCC_OK -> START\n");
                    }
                    return 0;

                case SM_STOP:
                    if (byte == FLAG)
                    {   
                        int destuffed_size;
                        unsigned char* destuffed = byte_destuff(file_buffer, strlen(file_buffer), &destuffed_size);
                        
                        if (check_bcc2(destuffed, destuffed_size))
                        {
                            destuffed[destuffed_size-1] = '\0';
                            printf("Aqui\n");
                            memset(file_buffer, 0, 1000);
                            printf("Aqui\n");
                            memcpy(file_buffer, destuffed, destuffed_size);
                            file_buffer[destuffed_size-1] = '\0';
                            sm->bytes_downloaded = destuffed_size-1;
                            printf("Aqui\n");
                            SendResponse(sm, (sm->expected_I_flag == I0) ? RR1 : RR0, true);
                            printf("Aqui\n");
                            sm->expected_I_flag = (sm->expected_I_flag == I0) ? I1 : I0;
                            sm->state = START;
                            printf("SM_STOP -> START\n");
                            return 1;

                        }
                        else
                        {
                            //enviar reject
                            sm->state = START;
                            printf("REJECTED. SM_STOP -> START\n");
                        }
                    }
                    else
                    {
                        sm->last_byte = byte;
                        file_buffer[sm->bytes_downloaded] = byte;
                        sm->bytes_downloaded++;
                        return 0;
                    }
                    
                default:
                    sm->state = START;
                    return 0;
            }
        }
        
    }

    else if (sm->role == TRANSMITTER)
    {
        switch(sm->state)
        {
            case START:
                if (byte == FLAG)
                {
                    sm->state = FLAG_RCV;
                    printf("START -> FLAG_RCV\n");
                }
                return 0;
            
            case FLAG_RCV:
                if (byte == ADDR_TRANSMITTER)
                {
                    sm->state = A_RCV;
                    printf("FLAG_RCV -> A_RCV\n");
                }
                else if (byte != FLAG)
                {
                    sm->state = START;
                    printf("FLAG_RCV -> START\n");
                }
                return 0;
            
            case A_RCV:
                if (sm->expected_frame == UA_frame)
                {
                    if (byte == UA) 
                    {
                        sm->state = C_RCV;
                        printf("A_RCV -> C_RCV\n");
                    }
                    else 
                    {
                        sm->state = START;
                        printf("A_RCV -> START\n");
                    }
                }
                else if (byte == sm->expected_RR_flag)
                {
                    sm->state = C_RCV;
                    printf("A_RCV -> C_RCV\n");
                } 
                else if (byte == sm->expected_REJ_flag)
                {
                    sm->state = C_RCV;
                    sm->packet_rejected = true;
                    printf("A_RCV -> C_RCV\n");
                } 
                else sm->state = START;
                return 0;
            
            case C_RCV:
                if (byte == FLAG){sm->state = FLAG_RCV; printf("C_RCV -> FLAG_RCV\n");} 
                else if (sm->expected_frame == UA_frame)
                {
                    if (byte == (UA^ADDR_TRANSMITTER)) {sm->state = BCC_OK; printf("C_RCV -> BCC_OK\n");} 
                    else {sm->state = START;printf("C_RCV -> START\n");}
                }
                else if (sm->packet_rejected && (byte == (ADDR_TRANSMITTER^sm->expected_REJ_flag))) {sm->state = BCC_OK;printf("C_RCV -> BCC_OK\n");}
                else if (!sm->packet_rejected && (byte == (ADDR_TRANSMITTER^sm->expected_RR_flag))) {sm->state = BCC_OK;printf("C_RCV -> BCC_OK\n");}
                else {sm->state = START;printf("C_RCV -> START\n");}
                return 0;
            
            case BCC_OK:
                if (byte == FLAG)
                {
                    if (sm->packet_rejected)
                    {
                        sm->packet_rejected = false;
                        sm->state = START;
                        printf("BCC_OK -> START\n");
                        return -1;
                    }
                    else if (sm->expected_frame == UA_frame)
                    {
                        sm->expected_frame = RR_REJ_frame;
                        sm->state = START;
                        printf("BCC_OK -> START\n");
                        return 1;
                    }
                    else
                    {
                        sm->expected_REJ_flag = (sm->expected_REJ_flag == REJ0) ? REJ1 : REJ0;
                        sm->expected_RR_flag = (sm->expected_RR_flag == RR0) ? RR1 : RR0;
                        sm->next_I_flag = (sm->next_I_flag == I0) ? I1 : I0;
                        sm->state = START;
                        printf("BCC_OK -> START\n");
                        return 1;
                    }
                }
                else sm->state = START;
                return 0;
        }
    }
    else if(sm->expected_frame==DISC_frame){//se ja tiver recebido todos os dados, passa para esperar pelo disc

        if (DEBUG) printf("DISC FRAME: %x ", byte);

            switch (sm->state){
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
                    sm->state = START;
                    sm->expected_frame = UA_frame;
                    if (DEBUG) printf("-> DISC_frame complete! Sending DISC resposnse...\n");
                    SendResponse(fd,sm,DISC,true); // envia disc de resposta
                    if (DEBUG) printf("-> DISC resposnse sent! Transitioned from BCC_OK to START\n");
                    return 1;   
                    }

                else{
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from BCC_OK to START\n", byte);
                    return -1;
                    }
        }
    }

    else if(sm->expected_frame==UA_frame){//recebe o UA final e termina o programa 

        if (DEBUG) printf("UA FRAME: %x ", byte);

            switch (sm->state){
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

                case C_RCV:
                if(byte == FLAG){ // se receber uma flag volta para o rcv flag e nao para start 
                    sm->state = FLAG;
                    if (DEBUG) printf("Garbage received (%x). Transitioned from C_RCV to FLAG_RCV\n", byte);
                    return 0;
                    }
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
           
                case BCC_OK:
                if (byte == FLAG)
                {   
                    sm->state = START;
                    if (DEBUG) printf("-> UA_frame received! Terminating connection...\n");
                    STOP=TRUE; // para terminar o ciclo no main?
                    return 1;   
                    }

                else{
                    sm->state = START;
                    if (DEBUG) printf("X Garbage recieved (%x). Transitioned from BCC_OK to START\n", byte);
                    return -1;
                    }
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
