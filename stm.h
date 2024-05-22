#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "byte_stuff.h"
#include <string.h>


#define FLAG    0x5c
#define SET     0x07
#define UA      0x06
#define DISC    0x0a
#define I0      0x80
#define I1      0xc0
#define RR0     0x01
#define RR1     0x11
#define REJ0    0x05
#define REJ1    0x15
#define ADDR_TRANSMITTER 0x01
#define ADDR_RECEIVER    0x03
#define FALSE 0
#define TRUE 1
#define TRANSMITTER 0
#define RECEIVER 1


typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    SM_STOP
} State;

typedef enum {
    SUPERVISION,
    INFORMATION,
    UA_frame,
    RR_REJ_frame,
} ExpectedFrame;

typedef struct {
    State state;
    int role;
    int fd;
    //sender
    unsigned char next_I_flag;
    unsigned char expected_RR_flag;
    unsigned char expected_REJ_flag;
    bool packet_rejected;
    //reciever
    unsigned char expected_I_flag;
    unsigned char last_byte;
    unsigned char bcc2;
    int bytes_downloaded;

    ExpectedFrame expected_frame;
} StateMachine;

StateMachine NewStateMachine(int role, int fd);
int Set(int fd, bool DEBUG);
int SendResponse(StateMachine* sm, unsigned char control_flag, bool DEBUG);
int UpdateState(unsigned char byte, StateMachine* sm, unsigned char* file_buffer);
int Send_Termination(int fd, bool DEBUG);

/*
 *  @brief Updates the state of the disc state machine according to the recieved byte. 
    *  @param  byte: recieved byte
    *  @param  *sm: reference to the state machine
    *  @param  DEBUG: flag to print debug messages
    *  @return -2 if a wrong bcc is recieved.
    *          -1 if the recieved byte is garbage.
    *          0 if the state machine updated its state successfuly.
    *          1 if the state machine is ready to send UA.           
 */
int ShortUpdateState(unsigned char byte, StateMachine_s* sm, bool DEBUG);

/*
 *  @brief Ends the communication to reciever sending a UA frame. 
    *  @param  fd: file descriptor of the serial port
    *  @param  DEBUG: flag to print debug messages
    *  @return -1 if the state machine could not be started.
    *          0 if the state machine was started successfuly.
 */
int Ua(int fd, bool DEBUG);
