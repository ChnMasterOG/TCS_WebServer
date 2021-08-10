#include <cstring>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>

#include "timer.h"
#include "server.h"

using namespace std;

extern Timer m_timer;
extern int epfd;

uint get_now_time() {
	return (uint)time(0);
}

void timer_handler(int arg) {
	uint now_time = get_now_time();
	vector<list<user_info*>::iterator> close_it;
	for (list<user_info*>::iterator it = m_timer.timer_list.begin(); it != m_timer.timer_list.end(); it++) {
		if ((*it)->get_activate_time() + ALIVE_SECONDs > now_time) break;
		else close_it.push_back(it);
	}
	for (auto it : close_it) close_connection(epfd, (*it)->get_fd(), ", time out");
	alarm(5);
}

bool Timer::update_timer(user_info* user) {
	if (timer_map.find(user) == timer_map.end()) return 1;	//error
	timer_list.erase(timer_map[user]);
	timer_list.push_back(user);
	list<user_info*>::iterator _end = timer_list.end();
	timer_map[user] = --_end;
	user->get_activate_time() = get_now_time();
	return 0;	//operation success
}

bool Timer::delete_timer(user_info* user) {
	if (timer_map.find(user) == timer_map.end()) return 1;	//error
	timer_list.erase(timer_map[user]);
	timer_map.erase(user);
	return 0;	//operation success
}

bool Timer::create_timer(user_info* user) {
	if (timer_map.find(user) != timer_map.end()) return 1;	//error
	timer_list.push_back(user);
	list<user_info*>::iterator _end = timer_list.end();
	timer_map[user] = --_end;
	return 0;	//operation success
}




