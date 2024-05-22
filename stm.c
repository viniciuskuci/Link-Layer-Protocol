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
                        
                        for (int i = 0; i < destuffed_size; i++)
                        {
                            printf("%x ", destuffed[i]);
                        }
                        if (check_bcc2(destuffed, destuffed_size))
                        {
                            destuffed[destuffed_size-1] = '\0';
                            memset(file_buffer, 0, 1000);
                            memcpy(file_buffer, destuffed, destuffed_size);
                            file_buffer[destuffed_size-1] = '\0';
                            sm->bytes_downloaded = destuffed_size-1;
                            SendResponse(sm, (sm->expected_I_flag == I0) ? RR1 : RR0, true);
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
                else if (!sm->packet_rejected && (byte == (ADDR_TRANSMITTER^sm->expected_RR_flag))) {sm->state = BCC_OK;printf(" C_RCV -> BCC_OK\n");}
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
                        printf("Recieved ACK. BCC_OK -> START\n");
                        return 1;
                    }
                }
                else sm->state = START;
                return 0;
        }
    }
}