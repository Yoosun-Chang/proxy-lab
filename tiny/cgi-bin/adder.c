/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
    char *buf, *p, *method;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    /* Extract the two arguments */
    /* 두 개의 인자 추출 */
    if ((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        *p = '\0';
        strcpy(arg1, buf);
        strcpy(arg2, p + 1);
        n1 = atoi(arg1); // 첫 번째 인자를 정수로 변환
        n2 = atoi(arg2); // 두 번째 인자를 정수로 변환
    }

    /* Make the response body */
    /* 응답 본문 생성 */
    sprintf(content, "QUERY_STRING=%s", buf);
    sprintf(content, "Welcome to add.com: ");
    sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
    sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>",
            content, n1, n2, n1 + n2);
    sprintf(content, "%sThanks for visiting!\r\n", content);

    /* Generate the HTTP response */
    /* HTTP 응답 생성 */
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int) strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content); // 생성된 응답 본문 출력
    fflush(stdout); // 출력 버퍼를 비워줌

    exit(0);
}
/* $end adder */