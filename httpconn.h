#ifndef _HTTPCONN_H
#define _HTTPCONN_H

#include <iostream>
#include <ctime>
#include <cstring>
#include <map>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "user.h"
#include "timer.h"

#define _HTTP_200_and_KeepAlive "HTTP/1.1 200 OK\r\nconnection: keep-alive\r\n\r\n"
#define _HTTP_200_and_Close "HTTP/1.1 200 OK\r\nconnection: close\r\n\r\n"
#define _HTTP_400_and_Close "HTTP/1.1 400 Bad Request\r\nconnection: close\r\n\r\nYour request has bad syntax or is inherently impossible to staisfy.\n"
#define _HTTP_403_and_Close "HTTP/1.1 403 Not Found\r\nconnection: close\r\n\r\nYou do not have permission to get file form this server.\n"
#define _HTTP_404_and_Close "HTTP/1.1 404 Not Found\r\nconnection: close\r\n\r\nThe requested file was not found on this server.\n"

#define READ_BUFFER_SIZE 2048
#define WRITE_BUFFER_SIZE 2048
#define FP_SIZE 128

#define STA_RECV 0
#define STA_SEND 1

class http_conn {
public:
    http_conn(int, int);
    ~http_conn() {}

    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    bool _read();
    bool send_html();
    bool send_response();
    bool send_table();

    bool check_cookies(string& _username);
    char* search_configuration(char* _buf, string field, string& value);
    HTTP_CODE check_head(METHOD& m_method, string& m_url);
    HTTP_CODE do_request();
    bool do_response(HTTP_CODE http_code);

    bool process_read();
    bool process_write();
    void process();

public:
    bool m_state;

private:
    char m_read_buf[READ_BUFFER_SIZE];
    char m_write_buf[WRITE_BUFFER_SIZE];
    char m_fp_buf[FP_SIZE];
    string m_md5;
    vector<vector<string>> m_table;
    bool m_md5_flag;
    bool m_table_flag;
    int m_read_p;   //read pointer
    bool m_TRIGMode;    //0 - LT, 1 - ET

    int m_sockfd;   //http socket fd
    int m_epollfd;  //epoll fd
};

void addfd(int epollfd, int fd, int TRIGMode = 0, bool one_shot = 1);
void modfd(int epollfd, int fd, int ev, int TRIGMode = 0, bool one_shot = 1);

#endif
