#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/param.h>
#include <pthread.h>
#include <pwd.h>
#include <sys/epoll.h>
#include <time.h>
#include "user.h"
#include "clientinfo.h"
#include "base.h"
#include "threadpool.h"
#include "logevent.h"

int userNum;

void cleanup();
void HandleRegister(void *arg);
void HandleLogin(void *arg);
void HandleChat(void *arg);
void HandleLogout(void *arg);
void addUser(USERPTR user, char *username, char *passward, char *fifo, int logged);
void createDetachedThread(void *(*start_routine)(void *), void *arg);
void boardcast(CLIENTINFO info);

int init_daemon(void)
{
    int pid;
    int i;
    /*忽略终端I/O信号，STOP信号*/
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if (pid > 0)
    {
        exit(0); /*结束父进程，使得子进程成为后台进程*/
    }
    else if (pid < 0)
    {
        return -1;
    }
    /*建立一个新的进程组,在这个新的进程组中,子进程成为这个进程组的首进程,以使该进程脱离所有终端*/
    setsid();
    /*再次新建一个子进程，退出父进程，保证该进程不是进程组长，同时让该进程无法再打开一个新的终端*/
    pid = fork();
    if (pid > 0)
    {
        exit(0);
    }
    else if (pid < 0)
    {
        return -1;
    }
    /*关闭所有从父进程继承的不再需要的文件描述符*/
    for (i = 0; i < NOFILE; close(i++))
        ;
    /*改变工作目录，使得进程不与任何文件系统联系*/
    chdir("/");
    /*将文件当时创建屏蔽字设置为0*/
    umask(0);
    /*忽略SIGCHLD信号*/
    signal(SIGCHLD, SIG_IGN);
    char FilePath[256];
    sprintf(FilePath, "%sserverlog/%s.log", LOGFILES, "server");
    freopen(FilePath, "w", stdout);
    return 0;
}

int main()
{
    init_daemon();

    int res;
    int fifo_fd, fd1;
    char massage[100];
    char receiver[100];
    char send[100];

    // signal(SIGINT, cleanup);
    // signal(SIGKILL, cleanup);
    // signal(SIGTERM, cleanup);
    // cleanup();

    RegisterMutex = open_sem(RegisterName);
    LoginMutex = open_sem(LoginName);
    ChatMutex = open_sem(ChatName);
    LogOutMutex = open_sem(LogOutName);
    userListMutex = open_sem(UserListNmae);

    int Register_fd = openFIFO(Register_FIFO, O_RDWR);
    int Login_fd = openFIFO(Login_FIFO, O_RDWR);
    int Chat_fd = openFIFO(Chat_FIFO, O_RDWR);
    int Logout_fd = openFIFO(LOGOUT_FIFO, O_RDWR);

    // atexit(cleanup);

    int epfd = epoll_create(1);
    struct epoll_event event;
    event.events = EPOLLIN;

    event.data.fd = Register_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, Register_fd, &event);
    event.data.fd = Login_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, Login_fd, &event);
    event.data.fd = Chat_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, Chat_fd, &event);
    event.data.fd = Logout_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, Logout_fd, &event);

    userList = (USERPTR)malloc(MAX_ONLINE_USERS * sizeof(USER));
    userNumPtr = (int *)malloc(sizeof(int));

    for (int i = 0; i < MAX_ONLINE_USERS; i++)
    {
        memset(&userList[i], '\0', sizeof(USER));
    }

    // addUser(&userList[0], "ccr", "123", "/var/log/chat-logs/client_fifo/ccr", 0);
    // addUser(&userList[1], "c", "1", "/var/log/chat-logs/client_fifo/c", 0);
    // addUser(&userList[2], "b", "2", "/var/log/chat-logs/client_fifo/b", 0);

    addUser(&userList[0], "ccr", "123", "/home/caichenru2020152021/finaltest/chatapplication/data/client_fifo/ccr", 0);
    addUser(&userList[1], "c", "1", "/home/caichenru2020152021/finaltest/chatapplication/data/client_fifo/c", 0);
    addUser(&userList[2], "b", "2", "/home/caichenru2020152021/finaltest/chatapplication/data/client_fifo/b", 0);

    *userNumPtr = 3;
    userNum = 3;

    threadpool_t *pool = threadpool_create(POOLSIZE, MAX_QUEUE);

    struct epoll_event events[MAX_EVENTS];
    while (1)
    {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        // 输出活跃事件以及文件描述符

        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == Register_fd)
            {
                USER user;
                // sem_wait(RegisterMutex);
                ssize_t bytes_read = read(events[i].data.fd, &user, sizeof(USER));
                // sem_post(RegisterMutex);
                if (bytes_read > 0)
                    // createDetachedThread(HandleRegister, &user);
                    threadpool_add(pool, HandleRegister, (void *)&user);
            }
            else if (events[i].data.fd == Login_fd)
            {
                USER user;
                ssize_t bytes_read = read(events[i].data.fd, &user, sizeof(USER));
                if (bytes_read > 0)
                    // createDetachedThread(HandleLogin, &user);
                    threadpool_add(pool, HandleLogin, (void *)&user);
            }
            else if (events[i].data.fd == Chat_fd)
            {
                CLIENTINFO info;
                // sem_wait(ChatMutex);
                ssize_t bytes_read = read(events[i].data.fd, &info, sizeof(CLIENTINFO));

                if (bytes_read > 0)
                    // createDetachedThread(HandleChat, &info);
                    threadpool_add(pool, HandleChat, (void *)&info);
            }
            else if (events[i].data.fd == Logout_fd)
            {
                // printf("logout\n");
                USER user;
                ssize_t bytes_read = read(events[i].data.fd, &user, sizeof(USER));
                if (bytes_read > 0)
                    // createDetachedThread(HandleLogout, &user);
                    threadpool_add(pool, HandleLogout, (void *)&user);
            }
        }
    }
}

void createDetachedThread(void *(*start_routine)(void *), void *arg)
{
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int res = pthread_create(&thread, &attr, start_routine, arg);
    if (res != 0)
    {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);
    }
    pthread_attr_destroy(&attr);
}
void addUser(USERPTR user, char *username, char *passward, char *fifo, int logged)
{
    sem_wait(userListMutex);
    strcpy(user->username, username);
    strcpy(user->password, passward);
    strcpy(user->fifo, fifo);
    user->logged = logged;
    sem_post(userListMutex);
}
void cleanup()
{
    unlink(Register_FIFO);
    unlink(Login_FIFO);
    unlink(Chat_FIFO);
    unlink(LOGOUT_FIFO);
    sem_unlink(RegisterName);
    sem_unlink(LoginName);
    sem_unlink(ChatName);
    sem_unlink(LogOutName);
    exit(1);
}
void HandleRegister(void *arg)
{
    int res;
    int fifo_fd;
    USER user;
    CLIENTINFO info;
    user = *(USERPTR)arg;
    int i;
    for (i = 0; i < userNum; i++)
        if (strcmp(user.username, userList[i].username) == 0)
            break;
    if (i < userNum)
    {
        memset(&info, '\0', sizeof(CLIENTINFO));
        setInfo(&info, "server", user.username, "Username already existed please type Register try again ! \n", user.fifo, REGISTER, 0);
    }
    else if (i < MAX_ONLINE_USERS)
    {

        char fifo[100];
        sprintf(fifo, "%sclient_fifo/%s", FIFOFILES, user.username);
        addUser(&userList[userNum], user.username, user.password, fifo, 0);
        userNum++;
        memset(&info, '\0', sizeof(CLIENTINFO));
        setInfo(&info, "server", user.username, "Register successfully! Use Login to login! \n", user.fifo, REGISTER, 1);
    }
    else
    {
        memset(&info, '\0', sizeof(CLIENTINFO));
        setInfo(&info, "server", user.username, "Register failed over MAX_ONLINE_USERS ! \n", user.fifo, REGISTER, 0);
    }
    sendInfo(info);
    logEvent(user.username, "Register", info);
    // return NULL;
}
void HandleLogin(void *arg)
{
    int res;
    int fifo_fd;
    USER user;
    CLIENTINFO info;
    user = *(USERPTR)arg;
    int i;
    for (i = 0; i < userNum; i++)
        if (strcmp(user.username, userList[i].username) == 0)
            break;
    if (i < userNum)
    {
        if (userList[i].logged == MAX_LOG_PERUSER)
        {
            setInfo(&info, "server", user.username, "Max loggers!", user.fifo, LOGIN, 0);
        }
        else if (strcmp(user.password, userList[i].password) == 0)
        {
            setInfo(&info, "server", user.username, "loged!", user.fifo, LOGIN, 1);
            userList[i].logged = 1;
            openFIFO(userList[i].fifo, O_RDWR);
        }
        else
        {
            setInfo(&info, "server", user.username, "Wrong password!", user.fifo, LOGIN, 0);
        }
    }
    else
    {
        setInfo(&info, "server", user.username, "User not exist!", user.fifo, LOGIN, 0);
    }

    sendInfo(info);
    logEvent(user.username, "Login", info);
    if (info.ACK == 1)
        boardcast(info);

    Resendinfo(user.username);

    // return NULL;
}
void HandleLogout(void *arg)
{
    int res;
    int fifo_fd;
    USER user;
    CLIENTINFO info;
    user = *(USERPTR)arg;
    int i;
    for (i = 0; i < userNum; i++)
        if (strcmp(user.username, userList[i].username) == 0)
            userList[i].logged = 0;
    setInfo(&info, user.username, user.username, "LogOUT", user.fifo, LOGOUT, 1);
    logEvent(user.username, "Logout", info);
    if (info.ACK == 1)
        boardcast(info);
    // return NULL;
}
void HandleChat(void *arg)
{
    int res;
    int fifo_fd;
    CLIENTINFO info;
    CLIENTINFO temp;
    USER user;

    char message[100];
    char receiver[100];
    char send[100];

    info = *(CLIENTINFOPTR)arg;
    temp = info;
    sem_post(ChatMutex);

    memset(&message, '\0', sizeof(message));
    char fifo[100];
    sprintf(message, "%s \n", info.message);
    memset(&receiver, '\0', sizeof(receiver));
    memset(&send, '\0', sizeof(send));
    strcpy(receiver, info.receiver);
    strcpy(send, info.sender);
    sprintf(fifo, "%sclient_fifo/%s", FIFOFILES, receiver); // fix bug3 sprintf(fifo, "data/client_fifo/%s \n", info.sender); 日了狗多了个\n
    memset(&info, '\0', sizeof(CLIENTINFO));
    int i;
    for (i = 0; i < userNum; i++)
        if (strcmp(receiver, userList[i].username) == 0)
            break;
    if (i < userNum)
    {
        // 用户存在
        if (userList[i].logged == 1)
            setInfo(&info, send, receiver, message, fifo, SEND, 1);
        else
        {
            memset(&fifo, '\0', sizeof(fifo));
            sprintf(fifo, "%sclient_fifo/%s", FIFOFILES, send);
            setInfo(&info, send, receiver, "User not online!\n", fifo, SEND, 0);
            sendFailed(temp);
        }
    }
    else // 不存在
    {
        memset(&fifo, '\0', sizeof(fifo));
        sprintf(fifo, "%sclient_fifo/%s", FIFOFILES, send);
        setInfo(&info, send, receiver, "User not exist!\n", fifo, SEND, 0);
    }

    sendInfo(info);

    logEvent(info.sender, "send", info);
    if (info.ACK)
        logEvent(info.receiver, "received", info);

    
    // return NULL;
}

void boardcast(CLIENTINFO info)
{

    char message[256];
    int i;
    memset(&message, '\0', sizeof(message));
    for (i = 0; i < userNum; i++)
    {
        if (userList[i].logged == 1)
        {
            // 存在用户
            char temp[256];
            memset(&temp, '\0', sizeof(temp));
            sprintf(temp, "User %s online\n", userList[i].username);
            strcat(message, temp);
        }
    }
    for (i = 0; i < userNum; i++)
    {
        if (userList[i].logged == 1)
        {
            // 存在用户
            CLIENTINFO SendInfo;

            char temp[256];
            memset(&temp, '\0', sizeof(temp));
            sprintf(temp, "User %s %s\n", info.receiver, info.message);

            strcat(temp, message);
            memset(&SendInfo, '\0', sizeof(CLIENTINFO));
            setInfo(&SendInfo, "server", userList[i].username, temp, userList[i].fifo, SEND, 1);
            sendInfo(SendInfo);
        }
    }
}