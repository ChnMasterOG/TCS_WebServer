#ifndef _TIMER_H
#define _TIMER_H

#include "user.h"
#include <map>
#include <vector>
#include <list>

using namespace std;

class Timer {
public:
    Timer() {}
    ~Timer() {}

    bool update_timer(user_info*);
    bool delete_timer(user_info*);
    bool create_timer(user_info*);
    
    list<user_info*> timer_list;
    map<user_info*, list<user_info*>::iterator> timer_map;
};

uint get_now_time();
void timer_handler(int);

#endif

