/*************************************************************************
	> File Name: make_socket_connect.c
	> Author: jiangxiaobai 
	> Mail: jiangxiaobai1989@gmail.com
	> Created Time: Wed 17 Aug 2016 08:15:49 PM CST
 ************************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <strings.h>

#define SADDR "115.28.49.209"

int make_socket_connect(int portnum)
{
    int sd;
    struct sockaddr_in saddr;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd == -1)
    {
        perror("socket");
        return -1;
    }

    bzero((void *)&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(portnum);
    saddr.sin_addr.s_addr = inet_addr(SADDR);

    if(connect(sd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
    {
        perror("connect");
        return -1;
    }

    return sd;
}

