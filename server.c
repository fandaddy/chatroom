/*************************************************************************
	> File Name: server.c
	> Author: Jiangxiaobai
	> Mail: jiangxiaobai1989@gmail.com
	> Created Time: Wed 17 Aug 2016 03:43:21 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>

#define PORTNUM 15001
#define BACKLOG 5
#define EPOLL_SIZE 5000
#define MAXSIZE 1024
#define SERVER_WELCOME "Welcome to the char room! Ur chat ID is: CLIENT #%d"
#define SERVER_MESSAGE "ClientID %d say >> %s"
#define CAUTION "There is only one int in the chat room"
#define BUFSIZ 0xFFFF

int make_socket_bind(int);
void addfd(int, int, bool);
int sendbroadcastmsg(int);

struct stru_clients
{
    int clientfd;
    int flag;
};

struct stru_clients clients[MAXSIZE];
int count;

int main(int ac, char *av[])
{
    int sock_id;
    int epfd;
    static struct epoll_event events[EPOLL_SIZE];

    //int count = 0;
    sock_id = make_socket_bind(PORTNUM);
    if(sock_id == -1)
    {
        exit(EXIT_FAILURE);
    }

    if(listen(sock_id, BACKLOG) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    epfd = epoll_create(EPOLL_SIZE);
    if (epfd == -1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    addfd(epfd, sock_id, true);

    /* 主循环 */
    //printf("count: %d\n", count);
    while(1)
    {
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count == -1)
        {
            perror("epoll failure");
            break;
        }
        //printf("epoll_events_count = %d\n", epoll_events_count);
        //for(int i = 0; i < count; i++)
        //{
        //    printf("%d: clients[%d].clientfd: %d\n", i, i, clients[i].clientfd);
        //}
        for(int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            //printf("sockfd: %d\nsock_id:%d\n", sockfd, sock_id);
            if(sockfd == sock_id)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlen = sizeof(struct sockaddr_in);
                int clientfd = accept(sock_id, (struct sockaddr *)&client_address, &client_addrlen);
                printf("client connection from: %s : %d(IP : port), client fd = %d \n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), clientfd);
                addfd(epfd, clientfd, true);


                // 保存用户连接
                int j;
                for(j = 0; clients[j].flag != 0 && j < MAXSIZE; j++);
                //printf("count is %d\n", count);
                if(j <= MAXSIZE)
                {
                    clients[j].clientfd = clientfd;
                    clients[j].flag = 1;
                    count++;
                    printf("Add new clientfd = %d to epoll\n", clientfd);
                    //printf("debug: clienfd: %d\n", clients[j].clientfd);
                    printf("Now there are %d clients in the chat room\n", count);
                }
                else
                {
                    printf("Too many clients\n");
                }

                // 服务端发送欢迎信息
                char message[BUFSIZ];
                bzero(message, BUFSIZ);
                sprintf(message, SERVER_WELCOME, clientfd);
                int ret = send(clientfd, message, BUFSIZ, 0);
                if(ret == -1)
                {
                    perror("send");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                //for(i = 0; i < count; i++)
                //{
                //    printf("%d: clients[%d].clientfd: %d\n", i, i, clients[i].clientfd);
                //}
                int ret = sendbroadcastmsg(sockfd);
                if(ret < 0)
                {
                    perror("error");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    close(sock_id);
    close(epfd);
    return 0;

}

int sendbroadcastmsg(int clientfd)
{
    int i;
    char buf[BUFSIZ], message[BUFSIZ];
    bzero(buf, BUFSIZ);
    bzero(message, BUFSIZ);

    int len = recv(clientfd, buf, BUFSIZ, 0);
    //printf("len:%d\nclientfd:%d\n", len, clientfd);
    
    //for(i = 0; i < count; i++)
    //{
    //    printf("%d: clients[%d].clientfd: %d\n", i, i, clients[i].clientfd);   
    //}


    if(len == 0)
    {
        close(clientfd);
        for(i = 0; clients[i].clientfd != clientfd; i++);
        clients[i].flag = 0;
        count--;
        //printf("clientID = %d closed.\nnow there are %d clients in the chat room\n", clientfd, count);
    }
    else
    {
        if(count == 1)
        {
            //printf("count is 1\n");
            send(clientfd, CAUTION, strlen(CAUTION), 0);
            return len;
        }
        sprintf(message, SERVER_MESSAGE, clientfd, buf);
        //printf("count is %d\n", count);
        for(i = 0; i < count; i++)
        {
            //printf("%d: clients[%d].clientfd: %d\n", i, i, clients[i].clientfd);
            if(clients[i].clientfd != clientfd)
            {
                //printf("sendbroadcastmsg:\n");
                //fprintf(stdout, message);
                //printf("clients.clientfd = %d\n", clients[i].clientfd);
                if(send(clients[i].clientfd, message, BUFSIZ, 0) < 0)
                {
                    perror("sendbroadcastmsg_send");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    return len;
}

/* 设置阻塞 */
int setnonblocking(int sd)
{
    fcntl(sd, F_SETFL, fcntl(sd, F_GETFD, 0)|O_NONBLOCK);
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
