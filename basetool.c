#include "base.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>

int openFIFO(const char *fifoName, int flags)
{
    int res;
    if (access(fifoName, F_OK) == -1)
    {
        res = mkfifo(fifoName, 0666);
        if (res != 0)
        {
            printf("FIFO %s was not created\n", fifoName);
            exit(EXIT_FAILURE);
        }
        struct passwd *pwd;
        pwd = getpwnam("caichenru2020152021");
        if (pwd == NULL)
        {
            perror("getpwnam");
            exit(EXIT_FAILURE);
        }
        uid_t new_owner_uid = pwd->pw_uid;
        gid_t new_group_gid = pwd->pw_gid;

        res = chown(fifoName, new_owner_uid, new_group_gid);
        if (res != 0)
        {
            perror("Could not change FIFO owner and group");
            exit(EXIT_FAILURE);
        }
    }
    int fifo_fd = open(fifoName, flags);
    if (fifo_fd == -1)
    {
        perror("Could not open FIFO");
        exit(EXIT_FAILURE);
    }
    return fifo_fd;
}

void sendInfo(CLIENTINFO info) // info的fifo是信息要穿到哪条fifo
{
    int res, fd1;
    if (access(info.fifo, F_OK) == -1)
    {
        res = mkfifo(info.fifo, 0777);
        if (res != 0)
        {
            printf("FIFO %s was not created\n", info.fifo);
            exit(EXIT_FAILURE);
        }
    }
    // fd1 = open(info.fifo, O_WRONLY | O_NONBLOCK);
    fd1 = open(info.fifo, O_WRONLY);
    if (fd1 == -1)
    {
        printf("Could not open %s for write only access\n", info.fifo);
        close(fd1);
    }
    else
    {
        //         write(fd1, &info, sizeof(CLIENTINFO));
        // close(fd1);
        int bytesAvailable;
        while (1)
        {
            bytesAvailable = 0;
            if (ioctl(fd1, FIONREAD, &bytesAvailable) == 0 && bytesAvailable > 0)
                continue;
            write(fd1, &info, sizeof(CLIENTINFO));
            close(fd1);
            break;
        }
    }
}

void setInfo(CLIENTINFOPTR info, char *sender, char *receiver, char *message, char *fifo, Outcome type, int ACK)
{
    strcpy(info->sender, sender);
    strcpy(info->receiver, receiver);
    info->type = type;
    info->ACK = ACK;
    strcpy(info->message, message);
    strcpy(info->fifo, fifo);
}

void displayInfo(CLIENTINFO info)
{
    printf("%s %s %s %s \n", info.sender, info.receiver, info.fifo, info.message);
}


void sendFailed(CLIENTINFO info)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // 储存失败消息在 receiveer_failed.log
    char logFilePath[256];
    sprintf(logFilePath, "%s%s_failed.log", LOGFILES, info.receiver);

    char logMessage[256];
    memset(&logMessage, '\0', sizeof(logMessage));

    int logFile = open(logFilePath, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (logFile == -1)
    {
        // perror("Error opening log file");
        return;
    }
    char timeStr[50];
    if (timeinfo != NULL)
    {
        // 手动构建年份、月份等字段
        int year = timeinfo->tm_year + 1900; // tm_year 表示自 1900 年以来的年数
        int month = timeinfo->tm_mon + 1;    // tm_mon 表示月份（0-11）
        // 将年份、月份等字段放入格式化字符串
        sprintf(timeStr, "%04d-%02d-%02d-%02d:%02d:%02d", year, month, timeinfo->tm_mday,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        // 输出时间字符串
        // printf("Current time: %s\n", timeStr);
    }
    else
    {
        fprintf(stderr, "Error: localtime failed\n");
    }
    sprintf(logMessage, "%s %s %s %s %s\n", info.sender, info.receiver,timeStr, info.fifo, info.message);
    write(logFile, logMessage, strlen(logMessage));
}
void Resendinfo(const char *username)
{
    char FilePath[256];
    sprintf(FilePath, "%s%s_failed.log", LOGFILES, username);
    FILE *file = fopen(FilePath, "r");
    if (file == NULL)
    {
        return;
    }
    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        CLIENTINFO info;
        char *token = strtok(line, " ");
        char messageWithTime[50];
        strcpy(info.sender, token);
        token = strtok(NULL, " ");
        strcpy(info.receiver, token);
        token = strtok(NULL, " ");
        strcpy(messageWithTime,token);
        token = strtok(NULL, " ");
        strcpy(info.fifo, token);
        token = strtok(NULL, "\n");
        strcpy(info.message, token);

        strcat(messageWithTime,info.message);
        // printf("%s\n",line);
        setInfo(&info, info.sender, info.receiver, messageWithTime, info.fifo, SEND, 1);
        // printf("****************\n");
        // displayInfo(info);
        sem_wait(ChatMutex);
        sendInfo(info);
        // sem_post(ChatMutex);
    }
    fclose(file);
    if (remove(FilePath) != 0)
    {
        perror("Error deleting log file");
    }
}
sem_t *open_sem(const char *filepath)
{
    sem_t *sem;
    sem = sem_open(filepath, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    struct passwd *pwd;

    pwd = getpwnam("caichenru2020152021");
    if (pwd == NULL)
    {
        perror("getpwnam");
        exit(EXIT_FAILURE);
    }

    uid_t new_owner_uid = pwd->pw_uid;
    gid_t new_group_gid = pwd->pw_gid;

    char path[256];
    sprintf(path, "/dev/shm/sem.%s", filepath + 1);
    chown(path, new_owner_uid, new_group_gid);

    return sem;
}