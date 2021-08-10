#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <thread>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <map>

#include "httpconn/httpconn.h"
#include "server.h"
#include "timer/timer.h"
#include "user.h"
#include "threadpool/threadpool.h"
#include "mysqlconn/mysqlconn.h"

using namespace std;

map<int, user_info*> fd_user;
map<int, http_conn*> fd_httpconn;
map<string, string> md5_username;
map<string, string> username_md5;
mysql_conn mysqlconn;
int epfd;
Timer m_timer;
ThreadPool m_threadpool;

void close_connection(int epfd, int fd, const string& reason)
{
	std::cout << "Close: fd = " << fd << reason << std::endl;
	m_timer.delete_timer(fd_user[fd]);
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
	delete fd_user[fd];
	delete fd_httpconn[fd];
	fd_user.erase(fd);
	fd_httpconn.erase(fd);
	close(fd);
}

int main()
{
	cout << "This is server." << endl;
	//connect database
	if (mysqlconn.connect_database()) {
		cout << "Error: connect to MySQL database" << endl;
		return 0;
	}
	//create socket
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	if (sockfd == -1) {
		cout << "Error: socket" << endl;
 		return 0;
 	}
	//bind
	struct sockaddr_in sever_address;
	sever_address.sin_family = AF_INET; //IPv4
	sever_address.sin_addr.s_addr = htons(INADDR_ANY);  //accept any address
	sever_address.sin_port = htons(SERVER_PORT);    //port
	if (bind(sockfd, (struct sockaddr*)&sever_address, sizeof(sever_address)) == -1) {
		cout << "Error: bind" << endl;
		return 0;
	}
	else cout << "OK: bind" << endl;
	//listen
	if (listen(sockfd, 100) == -1) {
		cout << "Error: listen" << endl;
		return 0;
	}
	else cout << "OK: listen" << endl;
	//create epfd
	epfd = epoll_create(EPOLL_SIZE);
	if (epfd < 0) {
		cout << "Error: epollfd" << endl;
		return 0;
	}
	else cout << "OK: epoll created, epollfd = " << epfd << endl;
	//add event
	addfd(epfd, sockfd, 0, 0);
	struct epoll_event events[EPOLL_SIZE];
	//start threadpool
	m_threadpool.start(MAX_THREADS);
	//register signal handler
	signal(SIGALRM, timer_handler);
	alarm(5);
	//main loop
	while(1) {
		int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
 		if (epoll_events_count < 0 && errno != EINTR) {
			cout << "Error: epoll count 0" << endl;
			break;
		}
		//handle events --- reactor model
 		for (int i = 0; i < epoll_events_count; ++i) {
			int evfd = events[i].data.fd;
			if (evfd == sockfd) {	//new connection
				cout << "New connection" << endl;
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
				if (1)	//LT mode
				{
					int connfd = accept(sockfd, (struct sockaddr *)&client_address, &client_addrlength);	//accept
					if (connfd < 0) {
						cout << "Error: accept" << endl;
						continue;
					}
					else cout << "OK: accept" << endl;
					//create information
					fd_user[connfd] = new user_info(connfd, client_address);
					fd_httpconn[connfd] = new http_conn(connfd, epfd);
					m_timer.create_timer(fd_user[connfd]);
					//add epoll event
					addfd(epfd, connfd);
				}
			}
			else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {	//close connection
				close_connection(epfd, evfd);
			}
			else if (events[i].events & EPOLLIN) {
				m_timer.update_timer(fd_user[evfd]);
				//process
				m_threadpool.addTask(bind(&http_conn::process, fd_httpconn[evfd]));
			}
			else if (events[i].events & EPOLLOUT) {
				//process
				m_threadpool.addTask(bind(&http_conn::process, fd_httpconn[evfd]));
			}
		}
	}
	close(sockfd);
	close(epfd);
	m_threadpool.finish();
}
