#include "threadpool.h"

ThreadPool::ThreadPool()
: done(false), isEmpty(true), isFull(false)
{}

void ThreadPool::setSize(int num) {
    (*this).initnum = num ;
}

void ThreadPool::addTask(const Task& f) {
    if (!done) {
        unique_lock<mutex> lk(mtx);
        
        //add task until task is not full
        while (isFull) {
            cond.wait(lk);
        }
        
        task.push(f);
        if (task.size() == initnum) isFull = true;   //detect full
        
        isEmpty = false;
        cond.notify_one();  //run a new task
    }
}

void ThreadPool::finish() {
    for (size_t i = 0; i < threads.size(); i++) {
        threads[i].join() ;
    }
}

void ThreadPool::runTask() {
    while(!done) {
        unique_lock<mutex>lk (mtx);

        //run task until queue is not empty
        while (isEmpty) {
            cond.wait(lk);
        }
        Task ta;
        ta = move(task.front());  
        task.pop();
        
        if (task.empty()) isEmpty = true ;  //detect empty
        
        isFull = false;
        ta();   //run real function
        cond.notify_one();
    }
}

void ThreadPool::start(int num) {
    setSize(num);

    for(int i = 0; i < num; i++) {        
        threads.push_back(thread(&ThreadPool::runTask, this));  //run task
    }
}

ThreadPool::~ThreadPool() {
}

