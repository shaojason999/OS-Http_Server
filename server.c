#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include "server.h"


int main(int argc, char *argv[])
{
    /*sock_fd is used to listen to the requests, and the sock_client is connted to the clients actually*/
    int sock_fd, sock_client;
    char buf[1024];
    char *msg="sent from server";
    int buf_len=sizeof(buf);
    /*important! never forget*/
    memset(buf, 0, buf_len);

    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(atoi(argv[4]));	/*solved the problem of big and little endian*/
    my_addr.sin_addr.s_addr=INADDR_ANY;
    int my_addr_len=sizeof(my_addr);

    /*creat the socket and set the let the socket know the information about server itself*/
    sock_fd=socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd<0) {
        printf("socket creation is failed\n");
        exit(-1);
    }
    if(bind(sock_fd, (struct sockaddr*)&my_addr, sizeof(my_addr))<0) {
        printf("bind failed\n");
        exit(-1);
    }

    /*waiting for the clients*/
    if(listen(sock_fd, 3)<0) {
        printf("listen failed\n");
        exit(-1);
    }

    /*creat a new socket to recv/send with client*/
    sock_client=accept(sock_fd, (struct sockaddr*)&my_addr, (socklen_t*)&my_addr_len);
    if(sock_client<0) {
        printf("accept failed\n");
        exit(-1);
    }

    recv(sock_client, buf, sizeof(buf), 0);
    printf("%s\n",buf);
    send(sock_client, msg, strlen(msg), 0);
    printf("message sent\n");


    close(sock_client);
    close(sock_fd);

    return 0;
}


