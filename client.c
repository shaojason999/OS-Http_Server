#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "client.h"


int main(int argc, char *argv[])
{
    int sock_fd;
    char buf[1024];
    int buf_len=sizeof(buf);
    /*important! never forget*/
    memset(buf, 0, buf_len);

    /*set the struct sockaddr_in, including the port and addr of the server*/
    struct  sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(atoi(argv[6]));	/*solved the problem of big and little endian*/
    /*stored the address into serv_addr*/
    if(inet_pton(AF_INET, argv[4], &serv_addr.sin_addr)<=0) {
        printf("invalid address\n");
        return -1;
    }

    /*creat a socket(doesn't know any information about server)*/
    sock_fd=socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd<0) {
        printf("socket creation is failed\n");
        return -1;
    }
    /*make the socket conneted to the server(let the socket know the serv_addr and serv_port)*/
    if(connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) {
        printf("connect failed\n");
        return -1;
    }
//    char *msg="sent from client";
    char msg[256];
    memset(msg, 0, sizeof(msg));
    sprintf(msg,"GET %s HTTP/1.x\r\nHOST: %s:%s\r\n\r\n",argv[2], argv[4], argv[6]);
    send(sock_fd, msg, strlen(msg), 0);

    while(recv(sock_fd, buf, buf_len-1, 0)) {
        printf("%s",buf);
        memset(buf, 0, buf_len);
    }
    printf("\n");
    close(sock_fd);
    return 0;

}
