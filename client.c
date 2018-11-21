#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "client.h"
FILE *fp=NULL;
char new_dest[128];

void creat_dir(char *dest)
{
    struct stat st= {0};
    if(stat("./output", &st)==-1)
        mkdir("./output", 0700);

    memset(new_dest, 0, sizeof(new_dest));
    sprintf(new_dest,"./output");
//	printf("%s\n",dest);

    int i, start=-1;
    char temp[128];
    memset(temp, 0, sizeof(temp));
    for(i=0; i<strlen(dest); ++i) {
        if(dest[i]=='/' && start==-1)
            start=i;
        else if(dest[i]=='/') {
            strncpy(temp, &dest[start], i-start);
            strcat(new_dest,temp);
            if(stat(new_dest, &st)==-1)
                mkdir(new_dest, 0700);
            start=i;	/*become next start '/'*/
        }
    }
    strncpy(temp, &dest[start], i-start);
    strcat(new_dest, temp);

}
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
    char msg[256];
    memset(msg, 0, sizeof(msg));
    sprintf(msg,"GET %s HTTP/1.x\r\nHOST: %s:%s\r\n\r\n",argv[2], argv[4], argv[6]);
    send(sock_fd, msg, strlen(msg), 0);


    int i=0, flag=0;
    while(recv(sock_fd, buf, buf_len-1, 0)) {
        printf("%s",buf);
        if(flag==0)	/*first time recv the msg*/
            if(buf[9]=='2') {	/*only when it is 200 OK, we fopen and deal with it*/
                creat_dir(argv[2]);
                fp=fopen(new_dest, "wb");
                for(i=0; i<strlen(buf) && flag<4; ++i)	/*do not need to write header*/
                    if(buf[i]=='\n')	/*there is 4 'n's in the header and 'n' is the last char in hearder*/
                        ++flag;	/*flag will not return to zero*/
                strncpy(buf, &buf[i], strlen(buf)-i);
                fwrite(buf, sizeof(char), strlen(buf), fp);
            }
        memset(buf, 0, buf_len);
    }
    printf("\n");
    if(flag!=0)
        fclose(fp);
    close(sock_fd);
    return 0;

}
