#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include "user.h"
#include "clientinfo.h"
#include "base.h"
#include <sys/select.h>

#define BUFF_SZ 100
USER me;

const char *operationNames[] = {
    "FALSE",
    "LOGIN",
    "REGISTER",
    "LOGOUT",
    "SEND",
};

typedef struct
{
    char username[BUFF_SZ];
    // Add other fields as needed
} ReceiverList;

void Register();
int Login();
void Listenchat();
void sendmassage();
void R_Login();
void handler(int sig);

int main(int argc, char *argv[])
{
    int res;
    int fifo_fd, my_fifo;
    int fd;
    CLIENTINFO info;

    // handler some signal
    signal(SIGKILL, handler);
    signal(SIGTERM, handler);
    signal(SIGINT, handler);

    // 打开信号量
    RegisterMutex = sem_open(RegisterName, 0); // 打已经存在的信号量 0的意思是打开若为1则创建
    LoginMutex = sem_open(LoginName, 0);
    ChatMutex = sem_open(ChatName, 0);
    LogOutMutex = sem_open(LogOutName, 0);
    while (1)
    {

        R_Login();
        sprintf(me.fifo, "%sclient_fifo/%s",FIFOFILES, me.username);
        if (fork() == 0)
        {

            Listenchat();
            exit(0);
        }
        sleep(1);
        sendmassage();
    }
}

void handler(int sig)
{
    unlink(me.fifo);
    exit(1);
}
CLIENTINFO read_info(USERPTR me)
{
    int fd1 = open(me->fifo, O_RDONLY);
    fd_set read_fds;
    struct timeval timeout;
    int ret;
    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(fd1, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        ret = select(fd1 + 1, &read_fds, NULL, NULL, &timeout);
        if (ret == -1)
        {
            printf("Error in select\n");
            exit(EXIT_FAILURE);
        }
        else if (ret == 0)
        {
            printf("Timeout in select\n");
            continue;
        }
        CLIENTINFO info;
        read(fd1, &info, sizeof(CLIENTINFO));
        printf("%s\n", info.message);
        return info;
    }
}
void Register()
{
    int fifo_fd, fd1;
    CLIENTINFO info;

    if (access(me.fifo, F_OK) == -1)
    {
        int res = mkfifo(me.fifo, 0777);
        if (res != 0)
        {
            printf("FIFO %s was not created\n", me.fifo);
            exit(EXIT_FAILURE);
        }
    }
    // 打开写管道向公共管道Register处写数据
    if (access(Register_FIFO, F_OK) == -1)
    {
        printf("Could not open %s\n", Register_FIFO);
        exit(EXIT_FAILURE);
    }

    fifo_fd = open(Register_FIFO, O_WRONLY | O_NONBLOCK);
    if (fifo_fd == -1)
    {
        printf("Could not open %s for write access\n", Register_FIFO);
        unlink(me.fifo);
        exit(EXIT_FAILURE);
    }

    // 向公共管道写数据
    sem_wait(RegisterMutex);
    write(fifo_fd, &me, sizeof(USER));
    close(fifo_fd);
    sem_post(RegisterMutex);
    printf("Register end\n");
    read_info(&me);
}
int Login()
{
    int fifo_fd, fd1;
    int res;
    CLIENTINFO info;
    printf("me.fifo = %s\n", me.fifo);

    if (access(me.fifo, F_OK) == -1)
    {
        res = mkfifo(me.fifo, 0777);
        if (res != 0)
        {
            printf("FIFO %s was not created\n", info.fifo);
            exit(EXIT_FAILURE);
        }
    }

    // 打开写管道向公共管道处写数据
    if (access(Login_FIFO, F_OK) == -1)
    {
        printf("Could not open %s\n", Login_FIFO);
        exit(EXIT_FAILURE);
    }

    fifo_fd = open(Login_FIFO, O_WRONLY);
    if (fifo_fd == -1)
    {
        printf("Could not open %s for write access\n", Login_FIFO);
        exit(EXIT_FAILURE);
    }
    sem_wait(LoginMutex);
    write(fifo_fd, &me, sizeof(USER));
    close(fifo_fd);
    sem_post(LoginMutex);

    info = read_info(&me);
    if (info.ACK == 1)
    {
        me.logged = 1;
        printf("successfully\n");
        return 1;
    }
    else
    {
        return 0;
    }
}
void R_Login()
{
    char buffer[BUFF_SZ];
    sprintf(me.fifo, "%sclient_fifo/client%d_fifo",FIFOFILES, getpid());
    while (1)
    {
        printf("Please input your username and password!\n");
        printf("username: ");
        scanf("%s", me.username);
        printf("password: ");
        scanf("%s", me.password);
        me.logged = 0;
        printf("Please input Register or Login!\n");
        scanf("%s", buffer);

        if (strcmp(buffer, "Register") == 0)
        {
            Register();
            continue;
        }
        else if (strcmp(buffer, "Login") == 0)
        {
            // 登录
            if (Login())
            {
                break;
            }
            else
            {
                continue;
            }
        }
    }
    unlink(me.fifo);
    printf("Login successfully!\n");
}
void Listenchat()
{
    int fifo_fd;
    CLIENTINFO info;
    int res;

    fifo_fd = openFIFO(me.fifo, O_RDONLY);
    prctl(PR_SET_PDEATHSIG,SIGKILL);
    while (1)
    {
        memset(&info, '\0', sizeof(CLIENTINFO));
        res = read(fifo_fd, &info, sizeof(info));

        if (res != 0)
        {
            printf("%s -> : %s", info.sender, info.message);            
            // fflush(stdout);
        }
    }
}
// 用户数量也要共享 fix bug1
// me的修改不是共享内存，所以没有初始值 fix bug2
void sendmassage()
{
    int fifo_fd, fd1;
    char buffer[BUFF_SZ];
    char receiver[BUFF_SZ];
    strcpy(receiver, me.username);
    int res;
    CLIENTINFO info;

    ReceiverList *receiverList = (ReceiverList *)malloc(MAX_ONLINE_USERS * sizeof(ReceiverList));
    for (int i = 0; i < MAX_ONLINE_USERS; i++)
    {
        memset(&receiverList[i], '\0', sizeof(receiverList));
    }

    int receiverNum = 0;

    printf("username:\n");
    printf("me.username = %s\n", me.username);

    setInfo(&info, me.username, receiver, "Please type a Name first!\n", Chat_FIFO, SEND, 0);

    fgets(buffer, BUFF_SZ, stdin);
    printf("<- %s : ", receiver);
    while (1)
    {
        memset(buffer, '\0', BUFF_SZ);
        memset(&info, '\0', sizeof(CLIENTINFO));

        fgets(buffer, BUFF_SZ, stdin);
        // 清除换行符
        buffer[strlen(buffer) - 1] = '\0';
        if (buffer[strlen(buffer) - 1] == '\n')
        {
            buffer[strlen(buffer) - 1] = '\0'; // remove trailing newline
        }
        if (buffer[0] == '\0')
        {
            continue; // ignore empty input
        }
        // 如果最后一个字符是‘：’则说明是要切换聊天对象
        if (buffer[strlen(buffer) - 1] == ':')
        {
            // 切换聊天对象
            memset(receiver, '\0', BUFF_SZ);

            for (int i = 0; i < MAX_ONLINE_USERS; i++)
            {
                memset(&receiverList[i], '\0', sizeof(receiverList));
            }

            receiverNum = 0;
            printf("Talking to %s\n", buffer);
            buffer[strlen(buffer) - 1] = '\0';
            strcpy(receiver, buffer);
            printf("<- %s : ", receiver);
            continue;
        }
        else if (buffer[strlen(buffer) - 1] == '\\')
        {

            if (access(LOGOUT_FIFO, F_OK) == -1)
            {
                printf("Could not open %s\n", LOGOUT_FIFO);
                exit(EXIT_FAILURE);
            }
            fifo_fd = open(LOGOUT_FIFO, O_WRONLY);
            if (fifo_fd == -1)
            {
                printf("Could not open %s for write access\n", LOGOUT_FIFO);
                exit(EXIT_FAILURE);
            }

            sem_wait(LogOutMutex);
            write(fifo_fd, &me, sizeof(USER));
            close(fifo_fd);
            sem_post(LogOutMutex);
            printf("Logout successfully\n");
            unlink(me.fifo);
            exit(0);
        }
        else if (buffer[strlen(buffer) - 1] == '>')
        {
            buffer[strlen(buffer) - 1] = '\0';
            strcpy(receiverList[receiverNum++].username, buffer);
            printf("<-");
            for (int i = 0; i < receiverNum; i++)
            {
                printf("%s ", receiverList[i].username);
            }
            printf(":");
            continue;
        }
        else
        {
            memset(&info, '\0', sizeof(CLIENTINFO));
            if (receiverNum == 0)
            {
                setInfo(&info, me.username, receiver, buffer, Chat_FIFO, SEND, 1);
                sem_wait(ChatMutex);
                sendInfo(info);
                // sem_post(ChatMutex);
            }
            else
            {
                for (int i = 0; i < receiverNum; i++)
                {
                    CLIENTINFO temInfo;

                    setInfo(&temInfo, me.username, receiverList[i].username, buffer, Chat_FIFO, SEND, 1);
                    // displayInfo(temInfo);

                    sem_wait(ChatMutex);
                    sendInfo(temInfo);
                    // sleep(3);
                    // sem_post(ChatMutex);
                }
            }
        }
        // 暂停0.5秒
        sleep(1);
        if (receiverNum == 0)
            printf("<- %s : ", receiver);
        else
        {
            printf("<-");
            for (int i = 0; i < receiverNum; i++)
            {
                printf("%s ", receiverList[i].username);
            }
            printf(":");
        }
    }
}
