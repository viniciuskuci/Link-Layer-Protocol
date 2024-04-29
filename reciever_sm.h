#define FLAG    0x5c
#define SET     0x07
#define UA      0x06
#define DISC    0x0a
#define I0      0x80
#define I1      0xC0
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
    SM_STOP
} State;

typedef struct {
    State state;
    int set;
    int ua;
} StateMachine;

/*
 * Function:  UpdateState 
 * --------------------
 *  @brief Updates the state of the state machine according to the recieved byte.
 *  
 *  @param  byte: recieved byte
 *  @param  *sm: reference to the state machine
 *
 *  @return -2 if a wrong bcc is recieved.
 *          -1 if the recieved byte is garbage.
 *          0 if the state machine updated its state successfuly.
 *          1 if the state machine is ready to send UA.
 *            
 */
int UpdateState(char byte, StateMachine* sm);

