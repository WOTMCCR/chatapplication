#ifndef _CLIENTINFO_H
#define _CLIENTINFO_H

typedef enum {
    FALESE,
    LOGIN,
    REGISTER,
    LOGOUT,
    SEND,
} Outcome;

typedef struct
{
    Outcome type;
    int ACK;
    char sender[20]; /*client's name*/
    char receiver[20]; /*client's name*/    
    char message[500]; /*client's massage*/
    char fifo[100]; /*client's FIFO name*/
}CLIENTINFO,*CLIENTINFOPTR;

#endif