#ifndef _SERVER_H
#define _SERVER_H

#define SERVER_PORT 8989
#define EPOLL_SIZE 10
#define BUF_SIZE 1024

#include <iostream>

using namespace std;

void close_connection(int fd, int epfd, const string& reason = "");

#endif


