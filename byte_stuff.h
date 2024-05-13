#define FLAG 0x5c
#define ESCAPE 0x5d
#define STUFF 0x20


/*
*    @brief Byte stuffs an array of bytes.
*    @param array: array to be byte stuffed.
*/
void byte_stuff(char **array);


/*
*    @brief Byte destuffs an array of bytes.
*    @param array: array to be byte destuffed.
*/
void byte_destuff(char **array);