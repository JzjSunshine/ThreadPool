//
// Created by Huster2021 on 2023/3/13.
//

#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H
#include "taskqueue.h"

template<typename T>
class ThreadPool{
public:
    ThreadPool(int min, int max);
    ~ThreadPool();

    //添加任务
    void addTask(Task<T> task);
    // 获取忙碌线程的个数
    int getBusyNumber();
    // 获取活着的线程个数
    int getAliveNumber();

private:
    //工作线程的任务函数
    static void *worker(void *arg);
    // 管理者线程的任务函数
    static void *manager(void *arg);
    void threadExit();

private:
    pthread_mutex_t m_lock; // 共享资源的访问，包括几个int变量值，任务队列
    // 当任务队列为空时，线程池中的线程会等待条件变量的信号，当有新任务加入任务队列时，线程池会发送信号给等待的线程
    // 互斥锁和 条件变量来实现任务队列的同步
    pthread_cond_t m_notEmpty; // 多线程环境中等待特定事件发生
    pthread_t* m_threadIDs; // 工作线程 IDS
    pthread_t m_managerID; // 管理者线程 ID
    TaskQueue<T>* m_taskQ; // 任务队列，存储要处理的任务
    int m_minNum; // 最小线程数量
    int m_maxNum; // 最大线程数量
    int m_busyNum; // 忙碌的线程数量
    int m_aliveNum;// 存活的线程数量
    int m_exitNum; // 需要销毁的线程数量
    bool m_shutdown = false; // 线程池是否关闭

};
#endif //THREADPOOL_THREADPOOL_H
