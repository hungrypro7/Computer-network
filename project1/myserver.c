/*
 * 본 코드는 HTTP 웹 서버 기능을 제공합니다.
 * 터미널에서 ./myserver MYPORT 로 실행하여야 합니다.
 * (MYPORT 번호는 CODE LINE 24 에서 원하는 대로 바꿀 수 있습니다.)
 * 
 * 로컬에서는 127.0.0.1, 그 외에 다른 기기에서는 이 기기의 IP 주소로 접속을 허용합니다.
 * HTML object(.jpg, .jpeg, .pdf, .mp3, .gif)를 전송할 수 있습니다.
 * 
 * 제출자 : 한양대학교 ERICA ICT 융합학부 이찬영
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define MYPORT 2478       // PORT 번호를 지정
#define BACKLOG 10        
#define BUF_SIZE 1024     // 버퍼 크기를 지정

/* HTTP 서버에서 클라이언트 요청에 대한 응답으로 사용되는 HTTP 헤더를 정의함 */
char imageheader[] = "HTTP/1.1 200 Ok\r\n" "Content-Type: image/jpeg\r\n\r\n";
char gifheader[] = "HTTP/1.1 200 Ok\r\n" "Content-Type: image/gif\r\n\r\n";
char mp3header[] = "HTTP/1.1 200 Ok\r\n" "Content-Type: audio/mp3\r\n\r\n";
char pdfheader[] = "HTTP/1.1 200 Ok\r\n" "Content-Type: application/pdf\r\n\r\n";
char htmlheader[] = "HTTP/1.1 200 Ok\r\n" "Content-Type: text/html\r\n\r\n";
char defaulthtml[] =  "HTTP/1.1 404 Not Found\r\n";


int main()
{
    int sockfd, new_fd;
   
    struct sockaddr_in my_addr;         // my address (내 주소)
    struct sockaddr_in their_addr;      // connector addr (연결 주소)
    int sin_size;

    int n;
    char buffer[BUF_SIZE];            // 버퍼 선언
    char *strptr1;

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {       // 소켓 생성
        perror("socket");         // 오류 처리
        exit(1);
    }

    my_addr.sin_family = AF_INET;                 // IPv4 프로토콜을 사용하도록 설정
    my_addr.sin_port = htons(MYPORT);             // MYPORT 포트 번호를 네트워크 바이트 순서로 변환하여 할당
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 호스트의 모든 IP 주소에 대한 연결을 허용
    
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {     // 소켓에 IP 주소와 PORT 번호 할당
        perror("bind");         // 오류 처리
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {        // 클라이언트의 연결 요청을 가능 상태로 만듬
        perror("listen");         // 오류 처리
        exit(1);
    }

    /* main accept() loop */
    while(1) {              
        sin_size = sizeof(their_addr);
        if ((new_fd = accept(sockfd, (struct sockaddr*)&their_addr, (socklen_t*) &sin_size)) == -1) {            // 클라이언트의 연결 요청이 오면 수락함
            perror("accept");       // 오류 처리
            continue;
        }
        // 만약 연결이 성공적으로 되면, 32비트 IPv4 주소로 연결 정보를 출력
        printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));
        
        /* 소켓으로부터 들어오는 데이터를 읽음 */
        bzero(buffer, BUF_SIZE);             // buffer 배열을 모두 0으로 초기화 함
        n = read(new_fd, buffer, BUF_SIZE);     // new_fd로부터 최대 BUF_SIZE의 데이터를 읽어 buffer에 저장함
        if(n < 0) perror("READING ERROR");      // 데이터를 제대로 읽지 못했을 경우, 에러를 출력함
        printf("%s", buffer);         // 읽어온 데이터를 화면에 출력함
        
        /* HTTP GET 요청에서 요청된 파일 이름을 추출함 */
        strptr1 = strstr(buffer,"GET /");       // buffer 문자열에서 'GET /'이 시작되는 위치를 찾아 strptr1 포인터에 저장함
        strptr1 += 5;                   // strptr1 포인터를 5만큼 증가시켜 'GET /' 문자열을 건너뛴 위치로 이동함
        char filename[100];
        char pwd[BUF_SIZE];
        memset(filename,0,100);
        int i=0;
        int fd;
        while(1)        // strptr1 포인터에서부터 시작되는 파일 이름을 추출
        {
          if(strptr1[i]==' ')
          {
            filename[i] = '\0'; 
            break;
          }
          filename[i]=strptr1[i];     // strptr1 포인터에서부터 파일 이름을 한 글자씩 filename 배열에 복사함
          i++;
        }
    
        /* 요청한 파일을 서버에서 찾아내기 위해 경로를 생성 */
        getcwd(pwd, BUF_SIZE);
        strcat(pwd, "/");
        strcat(pwd, filename);
        char fbuf[BUF_SIZE];

        /* 요청받은 파일이 현재 작업 디렉토리에 없을 때 */
        if((fd=open(pwd,O_RDONLY))==-1)
        {
          // new_fd 파일 디스크립터를 통해 defaulthtml의 내용을 클라이언트에게 보냄, 없을 경우 404 메시지 보냄
          write(new_fd, defaulthtml, sizeof(defaulthtml)-1);
        }
        /* 파일을 찾아서 클라이언트에게 보냄 */
        else
        { // 각각의 파일 확장자를 보고 적절한 HTTP 응답 헤더를 전송함 (클라이언트에게 요청한 파일의 종류와 크기 등을 전송함) 
          if(strstr(filename,".jpg") != NULL || strstr(filename,".jpeg") != NULL)
            write(new_fd, imageheader, sizeof(imageheader)-1);
          else if(strstr(filename,".gif") != NULL)
            write(new_fd, gifheader, sizeof(gifheader)-1);
          else if(strstr(filename,".pdf") != NULL)
            write(new_fd, pdfheader, sizeof(pdfheader)-1);
          else if(strstr(filename,".mp3") != NULL)
            write(new_fd, mp3header, sizeof(mp3header)-1);
          else if(strstr(filename,".html") != NULL)
            write(new_fd, htmlheader, sizeof(htmlheader)-1);

          while((n=read(fd,fbuf,BUF_SIZE))>0)
              // fd에서 BUF_SIZE 바이트만큼 읽어와서 fbuf에 저장함, fbuf에 저장된 내용을 new_fd에 보냄
              write(new_fd, fbuf, n);
        }
      close(new_fd);      // 클라이언트 소켓 닫음
      close(fd);          // 파일 디스크립터 닫음
    }
}