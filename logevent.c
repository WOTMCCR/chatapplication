#include "logevent.h"
#include<time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include<fcntl.h>
#include <unistd.h>
#include<pthread.h>


void logEvent(const char *username, const char *event, CLIENTINFO info)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char message[256];
    if (info.ACK == 0)
    {
        if (info.type != SEND)
            sprintf(message, "\n%s", "false");
        else
            sprintf(message, "to %s %s%s", info.receiver, info.message, "false");
    }
    else
    {
        if (info.type != SEND)
            sprintf(message, "\n%s", "true");
        else
            sprintf(message, "to %s %s%s", info.receiver, info.message, "true");
    }

    if (strcmp(event, "received") == 0)
    {
        memset(message, '\0', sizeof(message));
        if (info.ACK == 1)
            sprintf(message, "%s %s%s", info.sender, info.message, "true");
        else
            sprintf(message, "%s %s%s", info.sender, info.message, "false");
    }
    char logMessage[256];
    char timeStr[50];
    if (timeinfo != NULL)
    {
        // 手动构建年份、月份等字段
        int year = timeinfo->tm_year + 1900; // tm_year 表示自 1900 年以来的年数
        int month = timeinfo->tm_mon + 1;    // tm_mon 表示月份（0-11）

        // 将年份、月份等字段放入格式化字符串
        sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d", year, month, timeinfo->tm_mday,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

        // 输出时间字符串
        // printf("Current time: %s\n", timeStr);
    }
    else
    {
        fprintf(stderr, "Error: localtime failed\n");
    }

    sprintf(logMessage, "%s %s %s %s\n", username, event, timeStr, message);

    writeLog(username, logMessage);
}
void writeLog(const char *username, const char *logMessage)
{
    char logFilePath[256];
    sprintf(logFilePath, "%s%s.log", LOGFILES, username);

    int logFile = open(logFilePath, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (logFile == -1)
    {
        perror("Error opening log file");
        return;
    }
    write(logFile, logMessage, strlen(logMessage));

    // Close the log file
    close(logFile);
}
void writePoolLog(const char *logMessage,pthread_t threadId)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char logFilePath[256];
    sprintf(logFilePath, "%s",POOLLOG);
    
    char timeStr[256];
    memset(timeStr, '\0', sizeof(timeStr));
        if (timeinfo != NULL)
    {
        int year = timeinfo->tm_year + 1900; // tm_year 表示自 1900 年以来的年数
        int month = timeinfo->tm_mon + 1;    // tm_mon 表示月份（0-11）
        sprintf(timeStr, "%04d-%02d-%02d-%02d:%02d:%02d", year, month, timeinfo->tm_mday,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    }
    else
    {
        fprintf(stderr, "Error: localtime failed\n");
    }
    char logStr[512];
    memset(logStr,'\0',sizeof(logStr));

    sprintf(logStr,"%s %lu %s",timeStr, (unsigned long)threadId,logMessage);

    int LogFile = open(logFilePath, O_WRONLY | O_CREAT | O_APPEND, 0600);
    write(LogFile, logStr, strlen(logStr));    
}