#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <iostream>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include <queue>

#define MAX_THREADS 10

using namespace std;

class ThreadPool{

public:
    typedef function<void()> Task;
    ThreadPool();
    ~ThreadPool();
    
    size_t initnum; //threads number
    vector<thread> threads; //vector threads
    queue<Task> task;   //queue task
    
    mutex mtx;
    condition_variable cond;

    bool done;  //if true when work done
    bool isEmpty;  //if true when queue is empty
    bool isFull;    //if true when task is full

    void addTask(const Task& f);
    void start(int num);
    void setSize(int num);
    void runTask();
    void finish();
};

#endif
