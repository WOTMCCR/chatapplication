#ifndef LOGEVENT_H
#define LOGEVENT_H

#include "clientinfo.h"
#include "user.h"
#include<semaphore.h>
#include<string.h>
#include <sys/ioctl.h>
#include "config.txt"

void logEvent(const char *username, const char *event, CLIENTINFO info);
void writeLog(const char *username, const char *logMessage);
void writePoolLog(const char *logMessage,pthread_t threadId);
#endif