#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* for close() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> /* netbd.h is needed for struct hostent =) */
#include<errno.h>
#include <arpa/inet.h>
#include <time.h>
#include<sys/time.h>

void error(const char *msg) // socket 錯誤時印出錯誤訊息
{
    perror(msg);
    exit(1);
}
void sender(char *argv[]); // sender socket code
void receiver(char *argv[]); // receiver socket code
void gettime(); // 用來取得時間

// protocol = argv[1] ; send|rec = argv[2] ; ip = argv[3] ; port = argv[4] ; file = argv[5];

int main(int argc, char *argv[])
{
    if (argc != 6) {
        fprintf(stderr,"ERROR, arguments number must be six\n");
        exit(1);
    }

    if(strcmp(argv[2], "send") == 0) {
        sender(&argv[0]);
    }
    else if(strcmp(argv[2], "recv") == 0) {
        receiver(&argv[0]);
    }
    else {
        fprintf(stderr,"ERROR, second arguments must be send or rec\n");
        exit(1);
    }
    return 0;
}


void sender(char *argv[]) {
    int sockfd, newsockfd, portno, n;
    socklen_t clilen;
    unsigned char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    FILE *fp = fopen(argv[5],"rb");
    fseek(fp,0,SEEK_END);
    int size = ftell(fp);
    fseek(fp,0,SEEK_SET);
    int transSize = 0;
    int count = 1;
    struct timeval start, end;

    if(strcmp(argv[1], "tcp") == 0) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("ERROR opening socket");
        bzero((char *) &serv_addr, sizeof(serv_addr)); // 清為0
        portno = atoi(argv[4]);
        serv_addr.sin_family = AF_INET; // IPv4 網路協定
        serv_addr.sin_addr.s_addr = INADDR_ANY; // 指任何連上來的 address。如果要接受所有來自 internet 的 connection 可使用
        serv_addr.sin_port = htons(portno);
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
                error("ERROR on binding");
        listen(sockfd,5);
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        int readlen;
        bzero(buffer,256);
        printf("0%% ");
        gettime();
        gettimeofday(&start, NULL);
        while((readlen = fread(buffer, sizeof(unsigned char), 255, fp))) {
            n = write(newsockfd,buffer,readlen);
            transSize += 255;
            if(transSize >= size/4*count) {
                printf("%d%% ",count*25);
                count++;
                gettime();
            }
            if (n < 0) error("ERROR reading from socket");
            if(readlen == 0) {
                printf("end of file\n");
                break;
            }
            bzero(buffer,256);
        }
        close(newsockfd);
        close(sockfd);
    }
    else if(strcmp(argv[1], "udp") == 0) {
        sockfd = socket(PF_INET, SOCK_DGRAM, 0);
        if(sockfd < 0) 
            error("ERROR opening socket");
        bzero((char *) &serv_addr, sizeof(serv_addr)); // 清為0
        portno = atoi(argv[4]);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(portno);
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR on binding");
        bzero(buffer,256);
        int readlen;

        clilen = sizeof(cli_addr);
        n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &clilen);
        bzero(buffer,256);

        printf("0%% ");
        gettime();
        gettimeofday(&start, NULL);
        
        while(1) {
            if(n == -1) {
                if(errno != EINTR)
                    error("recvfrom error");
            }
            readlen = fread(buffer, sizeof(unsigned char), 255, fp);
            if(readlen == 0) {
                sendto(sockfd, buffer, 0, 0, (struct sockaddr *)&cli_addr,clilen);
                break;
            }
            sendto(sockfd, buffer, readlen, 0, (struct sockaddr *)&cli_addr,clilen);
            transSize += 255;
            if(transSize >= size/4*count) {
                printf("%d%% ",count*25);
                count++;
                gettime();
            }
            bzero(buffer,256);
        }
        int senddata_success_num = 0;
        recvfrom(sockfd, &senddata_success_num, sizeof(int), 0, (struct sockaddr *)&cli_addr, &clilen);
        printf("miss rate : %f\n",((float)(size-senddata_success_num)/size));
        close(sockfd);
    }
    gettimeofday(&end, NULL);
    printf("total transfer time : %ld(ms)\n",((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000));
    printf("file size : %d(Byte)\n",size);
    return;
}

void receiver(char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    unsigned char buffer[256];
    FILE *fp = fopen(argv[5],"wb");
    int datasend_success_num = 0;

    if(strcmp(argv[1], "tcp") == 0) {
        portno = atoi(argv[4]);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");
        server = gethostbyname(argv[3]);
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, 
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
        serv_addr.sin_port = htons(portno);
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
            error("ERROR connecting");

        bzero(buffer,256);
        while((n = read(sockfd, buffer, 255))) {
            if (n < 0) 
                error("ERROR writing to socket");
            fwrite(buffer,sizeof(unsigned char),n,fp);
            if(n == 0) {
                printf("all data receive\n");
                break;
            }
            bzero(buffer,256);
        }
        close(sockfd);
    }
    else if(strcmp(argv[1], "udp") == 0) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        portno = atoi(argv[4]);
        if (sockfd < 0) 
            error("ERROR opening socket");
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(portno);
        serv_addr.sin_addr.s_addr = inet_addr(argv[3]);

        sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

        while(1) {
            n = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
            datasend_success_num += n;
            if(n == 0) {
                printf("transfer end\n");
                break;
            }
            if(n == -1) {
                if(errno == EINTR)
                    continue;
                    error("recvfrom");
            }
            fwrite(buffer,sizeof(unsigned char),n,fp);
            bzero(buffer,256);
        }
        printf("%d\n",datasend_success_num);
        sendto(sockfd, &datasend_success_num, sizeof(int), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        close(sockfd);
    } 
    return;
}

void gettime() {
    int year,month,day,hour,minute,second;
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    /* Get GMT time */
    info = localtime(&rawtime );
    year = info->tm_year;
    month = info->tm_mon;
    day = info->tm_mday;
    hour = info->tm_hour;
    minute = info->tm_min;
    second = info->tm_sec;
    if(hour/24 > 1)
        day++;
        hour = hour % 24;
    printf("%d/%02d/%02d %2d:%02d:%02d\n",1900+year,1+month,day,hour,minute,second);
}