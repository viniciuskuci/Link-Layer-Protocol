#define FLAG    0x5c
#define SET     0x07
#define UA      0x06
#define DISC    0x0a
#define ADDR_TRANSMITTER 0x01
#define ADDR_RECEIVER    0x03


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

