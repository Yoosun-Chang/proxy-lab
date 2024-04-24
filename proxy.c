#include <stdio.h>
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void parse_uri(char *uri, char *hostname, char *pathname, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp);
void *thread(void *vargp);

static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
  int listenfd;
  int *clientfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  signal(SIGPIPE, SIG_IGN); // broken pipe 에러 해결용 코드 -프로세스 전체에 대한 시그널 핸들러 설정

  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    clientfd = Malloc(sizeof(int));
    *clientfd = Accept(listenfd, (SA * ) & clientaddr, &clientlen);
    Getnameinfo((SA * ) & clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    Pthread_create(&tid, NULL, thread, clientfd);
  }
}

void *thread(void *vargp) {
    int clientfd = *((int *) vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(clientfd);
    Close(clientfd);
    return NULL;
}

void doit(int clientfd) {
    int serverfd;
    char request_buf[MAXLINE], response_buf[MAX_OBJECT_SIZE];
    char method[MAXLINE], uri[MAXLINE], path[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE];
    rio_t request_rio, response_rio;

    Rio_readinitb(&request_rio, clientfd);
    Rio_readlineb(&request_rio, request_buf, MAXLINE);

    sscanf(request_buf, "%s %s", method, uri);

    if (!strcasecmp(uri, "/favicon.ico"))
      return;

    parse_uri(uri, hostname, port, path);

    sprintf(request_buf, "%s /%s %s\r\n", method, path, "HTTP/1.0");
    sprintf(request_buf, "%sConnection: close\r\n", request_buf);
    sprintf(request_buf, "%sProxy-Connection: close\r\n", request_buf);
    sprintf(request_buf, "%s%s\r\n", request_buf, user_agent_hdr);

    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {
        clienterror(clientfd, method, "501", "Not Implemented", "Proxy does not implement this method");
        return;
    }

    serverfd = Open_clientfd(hostname, port);
    if (serverfd < 0) {
        clienterror(clientfd, hostname, "404", "Not found", "Proxy couldn't connect to the server");
        return;
    }

    Rio_writen(serverfd, request_buf, strlen(request_buf));
    Rio_readinitb(&response_rio, serverfd);

    ssize_t n;

    while ((n = Rio_readlineb(&response_rio, response_buf, MAX_OBJECT_SIZE)) > 0) {
      Rio_writen(clientfd, response_buf, n);
      if (!strcmp(response_buf, "\r\n"))
        break;
    }

    while ((n = Rio_readlineb(&response_rio, response_buf, MAX_OBJECT_SIZE)) > 0) {
      Rio_writen(clientfd, response_buf, n);
    }
    Close(serverfd);
}

void parse_uri(char *uri, char *hostname, char *port, char *path) {
    char *hostname_ptr = strstr(uri, "//") != NULL ? strstr(uri, "//") + 2 : uri + 1;
    char *port_ptr = strstr(hostname_ptr, ":");
    char *path_ptr = strstr(hostname_ptr, "/");
    
    if (path_ptr > 0) {
        *path_ptr = '\0';
        strcpy(path, path_ptr+1);
    }
    if (port_ptr > 0) {
        *port_ptr = '\0';
        strcpy(port, port_ptr + 1);
    }

    strcpy(hostname, hostname_ptr);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %lu\r\n\r\n", strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
    }
    return;
}
