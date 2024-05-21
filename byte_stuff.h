#define FLAG 0x5c
#define ESCAPE 0x5d
#define STUFF 0x20
#define ADDR_TRANSMITTER 0x01
#define I0      0x80

int check_bcc2(const unsigned char* buf, int bufSize);

unsigned char calculate_bcc2(const unsigned char* buf, int bufSize);

/*
*    @brief Byte stuffs an array of bytes.
*    @param array: array to be byte stuffed.
*/
unsigned char* byte_stuff(const unsigned char* buf, int bufSize, int* stuffedSize);


/*
*    @brief Byte destuffs an array of bytes.
*    @param array: array to be byte destuffed.
*/
unsigned char* byte_destuff(const unsigned char* buf, int bufSize, int* destuffedSize);

unsigned char* framing(const unsigned char* stuffedBuf, int stuffedSize, int* framedSize, unsigned char control_field);