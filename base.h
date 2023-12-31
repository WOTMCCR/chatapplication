#ifndef BASE_H
#define BASE_H

#include "clientinfo.h"
#include "user.h"
#include<semaphore.h>
#include<string.h>
#include <sys/ioctl.h>
#include "config.txt"

USERPTR userList;
// userNum也得共享
int *userNumPtr;

int shmid;
int shmid2;

sem_t *RegisterMutex;
sem_t *LoginMutex;
sem_t *ChatMutex;
sem_t *LogOutMutex;
sem_t *userListMutex;


int openFIFO(const char *fifoName, int flags);
void sendInfo(CLIENTINFO info);
void displayInfo(CLIENTINFO info);

void setInfo(CLIENTINFOPTR info, char *sender, char *receiver, char *message, char *fifo, Outcome type,int ACK);

sem_t * open_sem(const char * filepath);


void sendFailed(CLIENTINFO info);
void Resendinfo(const char *username);

#endif