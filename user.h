
#ifndef _USER_H
#define _USER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <iostream>

#define ALIVE_SECONDs 300

class user_info {
public:
	user_info(int _fd, sockaddr_in _address)
	: fd(_fd), m_address(_address)
	{
		activate_time = time(0);
	}
	int const get_fd() { return fd; }
	std::string get_uuid() { return uuid; }
	unsigned int& get_activate_time() { return activate_time; }
	sockaddr_in& get_address() { return m_address; }

private:
	int fd;
	sockaddr_in m_address;
	unsigned int activate_time;
	std::string uuid;

};

#endif



