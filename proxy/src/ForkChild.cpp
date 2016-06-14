#include "ForkChild.h"

#include<stdio.h>  
#include<stdlib.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/param.h>
//#include <sys/wait.h>  
#include <unistd.h>
#include <signal.h>  

////��־��
//volatile sig_atomic_t _running = 1;
//
//// signal handler  
//void sigterm_handler(int arg)
//{
//    _running = 0;
//}

void ForkChild()
{
    pid_t pid;

    signal(SIGINT, SIG_IGN);// �ն��ж�  
    signal(SIGHUP, SIG_IGN);// ���ӹҶ�  
    signal(SIGQUIT, SIG_IGN);// �ն��˳�  
    signal(SIGPIPE, SIG_IGN);// ���޶����̵Ĺܵ�д����  
    signal(SIGTTOU, SIG_IGN);// ��̨������д����  
    signal(SIGTTIN, SIG_IGN);// ��̨�����Զ�����  
    signal(SIGTERM, SIG_IGN);// ��ֹ  

    // [1] fork child process and exit father process  
    pid = fork();
    if (pid < 0)
    {
        perror("fork error!");
        exit(1);
    }
    else if (pid > 0)
    {
        exit(0); //�������˳�
    }

    // [2] create a new session  
    setsid();

    // [3] set current path  
    /*char szPath[1024];
    if (getcwd(szPath, sizeof(szPath)) == NULL)
    {
    perror("getcwd");
    exit(1);
    }
    else
    {
    chdir(szPath);
    printf("set current path succ [%s]\n", szPath);
    }*/

    // [4] umask 0  
    umask(0);

    // [5] close useless fd  
    int i;
    for (i = 0; i < NOFILE; ++i)
    {
        close(i);
    }

    // [6] set termianl signal  
    //signal(SIGTERM, sigterm_handler);


}

/*
gcc -Wall -g -o test test.c
ps ux | grep -v grep | grep test
tail -f outfile
kill -s SIGTERM PID
*/

