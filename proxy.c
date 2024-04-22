#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000 // 최대 캐시 크기
#define MAX_OBJECT_SIZE 102400 // 최대 객체 크기

void doit(int fd);
void parse_uri(char *uri, char *hostname, char *pathname, char *port);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  int listenfd, clientfd; // 서버 및 클라이언트 소켓 파일 디스크립터
  char hostname[MAXLINE], port[MAXLINE]; // 클라이언트 호스트네임 및 포트번호
  socklen_t clientlen; // 클라이언트 주소 구조체 크기
  struct sockaddr_storage clientaddr; // 클라이언트 주소 구조체

  // 명령행 인수 확인
  if (argc != 2) { // 인수 개수가 2가 아니면 오류 메시지 출력
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 클라이언트 연결 수신 소켓 생성
  listenfd = Open_listenfd(argv[1]);

  // 클라이언트 요청 수락 및 처리
  while (1) {
    clientlen = sizeof(clientaddr); // 클라이언트 주소 구조체 크기 설정
    clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 수락
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // 클라이언트 호스트네임 및 포트번호 추출
    printf("Accepted connection from (%s, %s)\n", hostname, port); // 연결 확인 메시지 출력
    doit(clientfd); // 클라이언트 요청 처리 함수 호출
    Close(clientfd); // 연결 종료 및 소켓 닫음
  }
}

/* 프록시 서버의 핵심 동작을 담당하는 함수 */
void doit(int fd)
{
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 버퍼 및 요청 라인 구성 요소
  rio_t rio_client, rio_server;

  /* 클라이언트가 보낸 요청 라인 분석 */
  Rio_readinitb(&rio_client, fd); // 클라이언트 소켓 디스크립터를 리오 버퍼에 연결
  Rio_readlineb(&rio_client, buf, MAXLINE); // 클라이언트로부터 요청 라인 읽기
  sscanf(buf, "%s %s %s", method, uri, version); // 요청 라인을 메소드, URI, 버전으로 파싱
  // GET /index.html HTTP/1.1
  // method: "GET"
	// uri: "/index.html"
	// version: "HTTP/1.1"

  /* HTTP 요청의 메서드가 "GET"이 아닌 경우에 501 오류를 클라이언트에게 반환 */
  if (strcasecmp(method, "GET"))
  { // 조건문에서 하나라도 0이면 0
    clienterror(fd, method, "501", "Not Implemented", "Proxy does not implement this method");
    return;
  }
  
  // 서버 호스트명과 포트를 추출
  char hostname[MAXLINE], pathname[MAXLINE], port[MAXLINE];
  parse_uri(uri, hostname, pathname, port);

  // 서버로 연결
  int serverfd = Open_clientfd(hostname, port);

  if (serverfd < 0) { // 파일이 없다면 -1, 404에러
    clienterror(fd, hostname, "404", "Not found", "Proxy couldn't connect to the server");
    return;
  }

  // 서버에 HTTP 요청 전송
  Rio_readinitb(&rio_server, serverfd); // 서버 소켓 디스크립터를 리오 버퍼에 연결
  Rio_writen(serverfd, buf, strlen(buf)); // 요청 라인을 서버에 전송
  
  // 요청 헤더와 본문을 서버로 전송
  while (Rio_readlineb(&rio_client, buf, MAXLINE) > 0) {  // 클라이언트로부터 헤더 읽기
      if (strcmp(buf, "\r\n") == 0) break; // 헤더 끝 확인
      Rio_writen(serverfd, buf, strlen(buf)); // 헤더를 서버에 전송
  }

  // 서버로부터 응답을 받아 클라이언트에 전송
  while (Rio_readlineb(&rio_server, buf, MAXLINE) > 0) { // 서버로부터 응답 읽기
      Rio_writen(fd, buf, strlen(buf)); // 응답을 클라이언트에 전송
  }

  // 연결 종료
  Close(serverfd);

}

/* 주어진 URI를 호스트명, 포트, 경로로 파싱하는 함수 */
void parse_uri(char *uri, char *hostname, char *pathname, char *port) {
    char *ptr;
    ptr = strstr(uri, "://"); // URI에서 "://" 문자열을 찾음
    if (ptr != NULL) { // "://" 문자열을 찾았을 경우
        sscanf(ptr+3, "%[^:/]:%[^/]%s", hostname, port, pathname); // 호스트명, 포트, 경로를 추출하여 변수에 저장
    } else { // "://" 문자열을 찾지 못한 경우
        sscanf(uri, "%[^:/]:%[^/]%s", hostname, port, pathname); // URI 전체를 호스트명, 포트, 경로로 간주하여 추출
    }
}

