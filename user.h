#ifndef _USER_H     
#define _USER_H

typedef struct 
{
    char username[20];
    char password[30];
    char fifo[100];
    int logged;
}USER,*USERPTR;


#endif // 
