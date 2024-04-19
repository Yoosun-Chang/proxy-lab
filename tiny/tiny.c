/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd; // 서버 및 클라이언트 소켓 파일 디스크립터
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
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 수락
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // 클라이언트 호스트네임 및 포트번호 추출
    printf("Accepted connection from (%s, %s)\n", hostname, port); // 연결 확인 메시지 출력
    doit(connfd); // 클라이언트 요청 처리 함수 호출
    Close(connfd); // 연결 종료 및 소켓 닫음
  }
}

void doit(int fd)
{
  int is_static; // 정적 파일 여부를 나타내는 플래그
  struct stat sbuf; // 파일의 상태 정보를 저장하는 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 버퍼 및 요청 라인 구성 요소
  char filename[MAXLINE], cgiargs[MAXLINE]; // 요청된 파일의 경로 및 CGI 인자
  rio_t rio; // 리오 버퍼

  /* 클라이언트가 rio로 보낸 요청 라인 분석 */
  Rio_readinitb(&rio, fd);                          //  리오 버퍼를 특정 파일 디스크립터(fd=>connfd)와 연결
  Rio_readlineb(&rio, buf, MAXLINE);                // rio(connfd)에 있는 string 한 줄(Request line)을 모두 buf로 옮긴다.
  printf("Request headers:\n");
  printf("%s", buf);                                // GET /index.html HTTP/1.1
  sscanf(buf, "%s %s %s", method, uri, version); 
  // method: "GET"
	// uri: "/index.html"
	// version: "HTTP/1.1"

  /* HTTP 요청의 메서드가 "GET" 또는 "HEAD"가 아닌 경우에 501 오류를 클라이언트에게 반환 */
  if (strcasecmp(method, "GET") * strcasecmp(method, "HEAD"))
  { // 조건문에서 하나라도 0이면 0
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  
  /* read_requesthdrs: 요청 헤더를 읽어들여서 출력 */
  /* 헤더를 읽는 것은 클라이언트의 요청을 처리하고 응답하기 위해 필요한 전처리 작업의 일부 */
  read_requesthdrs(&rio);

  /* parse_uri: 클라이언트 요청 라인에서 받아온 uri를 이용해 정적/동적 컨텐츠를 구분한다.*/
  /* is_static이 1이면 정적 컨텐츠, 0이면 동적 컨텐츠 */
  is_static = parse_uri(uri, filename, cgiargs);

  /* filename: 클라이언트가 요청한 파일의 경로 */
  if (stat(filename, &sbuf) < 0) 
  { // 파일이 없다면 -1, 404에러
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  /* 정적 컨텐츠 */
  if (is_static)
  {
    // !(일반 파일이다) or !(읽기 권한이 있다)
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    // 정적 서버에 파일의 사이즈를 같이 보낸다. => Response header에 Content-length 위해
    serve_static(fd, filename, sbuf.st_size);
  }

  /* 동적 컨텐츠 */
  else
  {
    // !(일반 파일이다) or !(실행 권한이 있다)
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    // 동적 서버에 인자를 같이 보낸다.
    // filename은 CGI 프로그램의 경로이고, cgiargs는 CGI 프로그램을 실행할 때 필요한 인자들을 포함
    serve_dynamic(fd, filename, cgiargs);
  }
}

