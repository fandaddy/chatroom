/*************************************************************************
	> File Name: client.c
	> Author: jiangxiaobai 
	> Mail: jiangxiaobai1989@gmail.com
	> Created Time: Wed 17 Aug 2016 08:36:15 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>

#define PORTNUM 15001
#define EPOLLSIZE 5000
#define BUFSIZ 0xFFFF

int make_socket_connect(int);
void addfd(int, int, bool);

int main(int ac, char *av[])
{
    int sd;
    int pipe_fd[2];
    static struct epoll_event events[2];

    sd = make_socket_connect(PORTNUM);
    if(sd == -1)
    {
        exit(EXIT_FAILURE);
    }
    if(pipe(pipe_fd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /* 创建epoll */
    int epfd = epoll_create(EPOLLSIZE);
    if(epfd == -1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    addfd(epfd, sd, true);
    addfd(epfd, pipe_fd[0], true);

    // 表示客户端是否正常工作
    bool isClientwork = true;

    // 聊天缓冲区
    char message[BUFSIZ];

    // Fork
    int pid = fork();
    if(pid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0)
    {
        close(pipe_fd[0]);
        printf("Plz input 'EXIT' to exit the chat room\n");
        while(isClientwork)
        {
            bzero(&message, BUFSIZ);
            fgets(message, BUFSIZ, stdin);
            // write(fileno(stdout), &message, BUFSIZ);
            if(strncasecmp(message, "EXIT", 4) == 0)
            {
                isClientwork = 0;
            }
            else
            {
                if(write(pipe_fd[1], message, strlen(message) - 1) < 0)
                {
                    //printf("有没有进来写过信息？\n");
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    else
    {
        close(pipe_fd[1]);
        while(isClientwork)
        {
            int epoll_events_count = epoll_wait(epfd, events, 2, -1);
            //处理就绪事件
            for(int i = 0; i < epoll_events_count; ++i)
            {
                bzero(&message, BUFSIZ);
                if(events[i].data.fd == sd)
                {
                    int ret = recv(sd, message, BUFSIZ, 0);
                    if(ret == 0)
                    {
                        printf("server closed connection: %d\n", sd);
                        close(sd);
                        isClientwork = 0;
                    }
                    else
                    {
                        //fprintf(stdout, "message", message);
                        printf("%s\n", message);
                    }
                }
                else
                {
                    int ret = read(events[i].data.fd, message, BUFSIZ);
                    if(ret == 0)
                    {
                        isClientwork = 0;
                    }
                    else
                    {
                        send(sd, message, BUFSIZ, 0);
                    }
                }
            }
        }
    }

    if(pid)
    {
        //关闭父进程和sock
        close(pipe_fd[0]);
        close(sd);
    }
    else
    {
        close(pipe_fd[1]);
    }
    return 0;
}

int setnonblocking(int sd)
{
    fcntl(sd, F_SETFL, fcntl(sd, F_GETFD, 0)| O_NONBLOCK);
    return 0;
}

void addfd(int epollfd, int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if(enable_et)
    {
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}
