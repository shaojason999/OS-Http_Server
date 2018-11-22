#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include "server.h"
#include "status.h"

/*sock_fd is used to listen to the requests, and the sock_client is connted to the clients actually*/
int sock_fd, sock_client;
char hearder[3][100]= {"HTTP/1.x ","Content-type: ","Server: httpserver/1.x"};
char buf[1024];
int buf_len=sizeof(buf);
char file_dir[256];
int category;

int get_inform()
{
    int space_start, space_end;
    int i;
    int last_slash, last_dot;
    space_start=-1;
    space_end=256;
    /*get file_dir*/
    for(i=0; i<strlen(buf); ++i) {
        if(buf[i]==' ' && space_start==-1)
            space_start=i;
        else if(buf[i]==' ') {
            space_end=i;
            break;
        }
    }
    memset(file_dir, 0, sizeof(file_dir));
    strncpy(file_dir, &buf[space_start+1], (space_end-space_start-1));

    /*get the .html or others*/
    if(file_dir[0]!='/')
        return 5;	/*bad request*/

    if(buf[0]!='G' || buf[1]!='E' || buf[2]!='T')
        return 8;	/*method_not_allowed*/

    last_slash=-1;
    last_dot=-1;
    for(i=0; i<strlen(file_dir); ++i)
        if(file_dir[i]=='/')
            last_slash=i;
    for(i=last_slash+1; i<strlen(file_dir); ++i)
        if(file_dir[i]=='.')
            last_dot=i;
    if(last_dot!=-1) {	/*may be one of the type*/
        int the_same;
        char temp[128];
        strncpy(temp, &buf[space_start+1+last_dot+1],(space_end-(space_start+1+last_dot+1)));
        for(i=0; i<8; ++i) {
            the_same=strcmp(temp,extensions[i].ext);
            if(the_same==0) {
                /**/			printf("%s\n",file_dir);
                category=i;
                return 0;	/*success, may be a file*/
            }
        }
        if(i==8)
            return 7;	/*unsupport_media_type*/
    }
    return	1;	/*success, may be a dir(not a file)*/


}
int output(int result)
{
    FILE *fp=NULL;
    int read_num;
    memset(buf, 0, sizeof(buf));
    if(result==5) {
        sprintf(buf,"%s400 BAD_REQUEST\n%s\n%s\n\n",hearder[0],hearder[1],hearder[2]);
        send(sock_client, buf, strlen(buf), 0);
    } else if(result==8) {
        sprintf(buf,"%s405 METHOD_NOT_FOUND\n%s\n%s\n\n",hearder[0],hearder[1],hearder[2]);
        send(sock_client, buf, strlen(buf), 0);
    } else if(result==7) {
        sprintf(buf,"%s415 UNSUPPORT_MEDIA_TYPE\n%s\n%s\n\n",hearder[0],hearder[1],hearder[2]);
        send(sock_client, buf, strlen(buf), 0);
    } else if(result==0 || result==1) {

        /*get the file or dir*/
        if(result==0) {
            fp=fopen(&file_dir[1], "rb");	/*ignore the first slash'/'*/

            if(fp==NULL) {
                sprintf(buf,"%s404 NOT_FOUND\n%s\n%s\n\n",hearder[0],hearder[1],hearder[2]);
                send(sock_client, buf, strlen(buf), 0);
            } else {
                sprintf(buf, "%s200 OK\n%s%s\n%s\n\n", hearder[0],hearder[1],extensions[category].mime_type,hearder[2]);
                send(sock_client, buf, strlen(buf), 0);
                while(!feof(fp)) {
                    read_num=fread(buf, sizeof(char), sizeof(buf)-1, fp);
                    send(sock_client, buf, read_num, 0);
                }
            }
        }
    }

    if(fp!=NULL)
        fclose(fp);

    return 0;

}
int main(int argc, char *argv[])
{
    int result;
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

    int on=1;
    if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {	/*reuse the port*/
        printf("can't reuse the port, you may wait for 1 minute\n");
        return -1;
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

    result=get_inform();
    output(result);

    close(sock_client);
    close(sock_fd);

    return 0;
}


