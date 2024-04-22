#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000 // 최대 캐시 크기
#define MAX_OBJECT_SIZE 102400 // 최대 객체 크기

void doit(int fd);

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

