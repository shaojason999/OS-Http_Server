#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include "server.h"
#include "status.h"

typedef struct queue {
    int capacity;
    int size;
    int front;
    int rear;
    int *elements;
} queue;

/*sock_fd is used to listen to the requests, and the sock_client is connted to the clients actually*/
int sock_fd, sock_client;
struct sockaddr_in my_addr;
int my_addr_len;
char hearder[3][100]= {"HTTP/1.x ","Content-type: ","Server: httpserver/1.x"};
char buf[1024];
int buf_len=sizeof(buf);
//char file_dir[256];
int category;
pthread_mutex_t mutex;
pthread_cond_t cond;
queue *q;
char buf[1024], file_dir[128];

int get_inform(char *buf, char *file_dir)
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
    memset(file_dir, 0, sizeof(char)*128);
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
    /*there exist a dot*/
    if(last_dot!=-1) {	/*may be one of the type*/
        int the_same;
        char temp[128];
        memset(temp, 0, sizeof(temp));
        strncpy(temp, &buf[space_start+1+last_dot+1],(space_end-(space_start+1+last_dot+1)));
        for(i=0; i<8; ++i) {
            the_same=strcmp(temp,extensions[i].ext);
            if(the_same==0) {
                category=i;
                return 0;	/*success, may be a file*/
            }
        }
        if(i==8)
            return 7;	/*unsupport_media_type*/
    }
    return	1;	/*success, may be a dir(not a file)*/


}
int output(int result, int sock_client, char *file_dir)
{
//    char buf[1024];
    FILE *fp=NULL;
    int read_num;


    memset(buf, 0, sizeof(buf));
    if(result==1) {
        DIR *dir_ptr;
        struct dirent *direntp;
        char temp[256], dest[256];
        char name_of_file_dir[256];
        char file_dir_arr[128][256];
        memset(temp, 0, sizeof(temp));
        sprintf(temp, ".%s",file_dir);
//        memset(file_dir, 0, sizeof(char)*128);
        //      strcpy(file_dir, temp);
        dir_ptr=opendir(temp);
        /*check if the dir exists*/
        if(dir_ptr==NULL) {
            printf("opendir failed\n");
            sprintf(buf,"%s404 NOT_FOUND\n%s\n%s\n\n",hearder[0],hearder[1],hearder[2]);
            send(sock_client, buf, strlen(buf), 0);
            return 0;
        }
        memset(dest, 0, sizeof(dest));
        memset(temp, 0, sizeof(temp));
        int i=0,j=0;
        /*get every subfile and subdir*/
        while((direntp=readdir(dir_ptr))!=NULL) {
            memset(name_of_file_dir, 0, sizeof(name_of_file_dir));
            strcpy(name_of_file_dir,direntp->d_name);
            /*exclude the . and .. dir*/
            if(name_of_file_dir[0]=='.')
                continue;
            /*deal with later*/
            memset(file_dir_arr[i], 0, sizeof(file_dir_arr[i]));
            strcpy(file_dir_arr[i++],name_of_file_dir);
            /*fisrt found and other founds*/
            if(j==0) {
                sprintf(dest,"%s",name_of_file_dir);
                ++j;
            } else
                sprintf(dest,"%s %s",temp, name_of_file_dir);
            memset(temp, 0, sizeof(temp));
            strcpy(temp,dest);
        }
        memset(buf, 0, sizeof(buf));
        sprintf(buf,"%s200 OK\n%s directory\n%s\n\n%s\n",hearder[0],hearder[1],hearder[2],dest);
        send(sock_client, buf, strlen(buf), 0);
        int num=i, last_dot, k;
        char Temp[i][256];
        char dot_temp[128];
        for(i=0; i<num; ++i) {
            memset(Temp[i], 0, sizeof(Temp[i]));
            for(j=1; j<strlen(file_dir_arr[i]); ++j)
                /*a file(not a dir)*/
                if(file_dir_arr[i][j]=='.') {
                    sprintf(Temp[i],"%s/%s",file_dir,file_dir_arr[i]);
//printf("%d %s %s\n",strlen(file_dir_arr[i]),Temp[i],file_dir_arr[i]);
                    /*find the category of the file(.thml, .css, ...)*/
                    last_dot=-1;
                    for(k=0; k<strlen(Temp[i]); ++k)
                        if(Temp[i][k]=='.')
                            last_dot=k;
                    memset(dot_temp, 0, sizeof(dot_temp));
                    strncpy(dot_temp, &Temp[i][last_dot+1], (strlen(Temp[i])-last_dot-1));
                    for(k=0; k<8; ++k) {
                        int the_same=strcmp(dot_temp, extensions[k].ext);
                        if(the_same==0) {
                            category=k;	/*a parameter in output()*/
                            break;
                        }
                    }
                    /*unsupported_media_type*/
                    if(k!=8)
                        output(0,sock_client,Temp[i]);
                    break;
                }
        }
        closedir(dir_ptr);
    }
    if(result==5) {
        sprintf(buf,"%s400 BAD_REQUEST\n%s\n%s\n\n",hearder[0],hearder[1],hearder[2]);
        send(sock_client, buf, strlen(buf), 0);
    } else if(result==8) {
        sprintf(buf,"%s405 METHOD_NOT_FOUND\n%s\n%s\n\n",hearder[0],hearder[1],hearder[2]);
        send(sock_client, buf, strlen(buf), 0);
    } else if(result==7) {
        sprintf(buf,"%s415 UNSUPPORT_MEDIA_TYPE\n%s\n%s\n\n",hearder[0],hearder[1],hearder[2]);
        send(sock_client, buf, strlen(buf), 0);
    } else if(result==0) {

        /*get the file or dir*/
        char tttemp[128];
        memset(tttemp, 0, sizeof(tttemp));
        sprintf(tttemp,".%s",file_dir);	/*open a file with ./xxx/xxx*/
        fp=fopen(tttemp,"rb");

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
            /*send a '\n' in the end of a file*/
            memset(buf, 0, sizeof(buf));
            sprintf(buf,"\n");
            send(sock_client, buf, strlen(buf), 0);
        }
    }

    if(fp!=NULL)
        fclose(fp);

    return 0;

}
queue *create_queue(int que_num)
{
    queue *Q;
    Q=(queue*)malloc(sizeof(queue));
    Q->elements=(int*)malloc(sizeof(int)*que_num);
    Q->size=0;
    Q->capacity=que_num;
    Q->front=0;
    Q->rear=-1;

    return Q;
}
int empty(queue *Q)
{
    if(Q->size==0)
        return 1;
    else
        return 0;
}
void pop(queue *Q)
{
    if(Q->size!=0) {
        Q->size--;
        Q->front++;

        if(Q->front==Q->capacity)
            Q->front=0;
    }
}
int peek(queue *Q)
{
    return Q->elements[Q->front];
}
void push(queue *Q, int element)
{
    if(Q->size!=Q->capacity) {
        Q->size++;
        Q->rear=Q->rear+1;
        if(Q->rear==Q->capacity)
            Q->rear=0;
        Q->elements[Q->rear]=element;
    }
}
int queue_get()
{
    pthread_mutex_lock(&mutex);
    while(empty(q)==1) {	/*not empty*/
        /**/		if(pthread_cond_wait(&cond, &mutex)!=0) {
            printf("cond wai error\n");
            return -1;
        }
    }
    /*pop*/
    int que_num=peek(q);
    pop(q);
    pthread_mutex_unlock(&mutex);

    return que_num;

}
void do_processing(int que_num)
{
    int result;
    memset(buf, 0, sizeof(buf));
    recv(que_num, buf, sizeof(buf), 0);
    result=get_inform(buf, file_dir);
// printf("%d\n",result);
    output(result, que_num, file_dir);

    close(que_num);

}
void *accept_connection()
{
    int que_num;
    while(1) {
        que_num=accept(sock_fd, (struct sockaddr*)&my_addr, (socklen_t*)&my_addr_len);
        if(que_num<0) {
            printf("accept failed\n");
            exit(-1);
        }
        pthread_mutex_lock(&mutex);
        push(q,que_num);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
    }
}
void *handle_connection()
{
    int que_num=0;
    while(1) {
        que_num=queue_get();
        do_processing(que_num);
    }
}
int main(int argc, char *argv[])
{
    int i;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(atoi(argv[4]));	/*solved the problem of big and little endian*/
    my_addr.sin_addr.s_addr=INADDR_ANY;
    int thread_num;
    my_addr_len=sizeof(my_addr);

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

    thread_num=atoi(argv[6]);
    q=create_queue(thread_num);

    /*initialize the mutex global variable and declare the thread pool array with 10 threads*/
    pthread_mutex_init(&mutex,NULL);
    pthread_t thread_pool[thread_num];
    /*thread create with one main thread and other threads*/
    pthread_create(&thread_pool[0], NULL, accept_connection, (void*)NULL);
    for(i=1; i<thread_num; ++i)
        pthread_create(&thread_pool[i], NULL, handle_connection, (void*)NULL);

    /*waiting for the clients*/
    if(listen(sock_fd, 3)<0) {
        printf("listen failed\n");
        exit(-1);
    }

    /*creat a new socket to recv/send with client*/
    while(1) {
    }

    /*    recv(sock_client, buf, sizeof(buf), 0);

        result=get_inform();
        output(result);
    */
    close(sock_client);
    close(sock_fd);

    return 0;
}


