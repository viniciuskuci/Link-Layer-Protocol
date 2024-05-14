#include <stdbool.h>

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


typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    SM_STOP,
    DATA_TRANSFER
} State;

typedef enum {
    SUPERVISION,
    UA_frame,
    INFORMATION,
    DISC_frame
} ExpectedFrame;


/*
*  @brief State machine structure.
    *  @param state: current state of the state machine.
    *  @param expected_frame: expected frame to be recieved.
    *  @param last_I_flag: last I flag recieved.
    *  @param last_byte: last byte recieved.
    *  @param bcc2: bcc of the data frame.
    *  @param bytes_downloaded: number of information bytes downloaded.
*/
typedef struct {
    State state;
    ExpectedFrame expected_frame;
    unsigned char expected_I_flag;
    unsigned char last_byte;
    unsigned char bcc2;
    int bytes_downloaded;
} StateMachine;


/*
*  @brief Creates a new state machine and initializes it.  
    *  @return A new state machine.  
*/
StateMachine NewStateMachine();


/*
 *  @brief Updates the state of the state machine according to the recieved byte. 
    *  @param  byte: recieved byte
    *  @param  *sm: reference to the state machine
    *  @param  DEBUG: flag to print debug messages
    *  @return -2 if a wrong bcc is recieved.
    *          -1 if the recieved byte is garbage.
    *          0 if the state machine updated its state successfuly.
    *          1 if the state machine is ready to send UA.           
 */
int UpdateState(unsigned char byte, StateMachine* sm, int fd, bool DEBUG);


/*
 *  @brief Sends an positive or negative ACK frame to the transmitter. 
    *  @param  fd: file descriptor of the serial port
    *  @param  *sm: reference to the state machine
    *  @param  control_flag: flag to send RR, REJ, UA or DISC
    *  @param  fd: file descriptor for sending the responses
    *  @param  DEBUG: flag to print debug messages
    *  @return 0 if the ACK was sent successfuly.
 */
int SendResponse(int fd, StateMachine *sm, unsigned char control_flag, bool DEBUG);

/*
 *  @brief Ends the communication to reciever sending a DISC frame. 
    *  @param  fd: file descriptor of the serial port
    *  @param  DEBUG: flag to print debug messages
    *  @return -1 if the request for termination was not sent.
    *          0 if the request was sent successfuly.
 */
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
int ShortUpdateState(unsigned char byte, StateMachine* sm, bool DEBUG);