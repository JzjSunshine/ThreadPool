//
// Created by Huster2021 on 2023/3/13.
//
#include "../include/threadpool.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

using namespace std;

template<typename T>
ThreadPool<T>::ThreadPool(int minNum, int maxNum) {
    // 实例化任务队列
    m_taskQ = new TaskQueue<T>;
    pthread_mutex_lock(&m_lock);
    m_shutdown = false;
    pthread_mutex_unlock(&m_lock);
    do {
        // 初始化线程池
        m_minNum = minNum;
        m_maxNum = maxNum;
        m_busyNum = 0;
        m_aliveNum = minNum;

        // 根据线程的最大上限给线程数组分配内存
        m_threadIDs = new pthread_t[maxNum];
        if (m_threadIDs == nullptr) {
            cout << "malloc thread_t[] 失败 ..." << endl;
            break;
        }
        memset(m_threadIDs, 0, sizeof(pthread_t) * maxNum);
        //初始化互斥锁，条件变量
        if (pthread_mutex_init(&m_lock, NULL) != 0 || pthread_cond_init(&m_notEmpty, NULL) != 0) {
            cout << "init mutex or condition failed ..." << endl;
            break;
        }

        /////////////////// 创建线程 //////////////////
        // 根据最小线程个数, 创建线程
        for (int i = 0; i < minNum; i++) {
            // 思路：worker 和 manager 都是静态函数，不能访问类的普通成员，可以通过对象访问对象的普通成员
            pthread_create(&m_threadIDs[i], NULL, worker, this);
            cout << "创建子线程，ID：" << to_string(m_threadIDs[i]) << endl;
        }

        // 管理线程 1 个
        pthread_create(&m_managerID, NULL, manager, this);

    } while (0);
}

template<typename T>
ThreadPool<T>::~ThreadPool() {
    pthread_mutex_lock(&m_lock);
    m_shutdown = true;
    pthread_mutex_lock(&m_lock);
    // 销毁管理者线程
    pthread_join(m_managerID, NULL);
    // 唤醒所有消费者线程
    for (int i = 0; i < m_aliveNum; i++) {
        pthread_cond_signal(&m_notEmpty);
    }
    if (m_taskQ) delete m_taskQ;
    if (m_threadIDs) delete[] m_threadIDs;
    pthread_mutex_destroy(&m_lock);
    pthread_cond_destroy(&m_notEmpty);

}

template<typename T>
void ThreadPool<T>::addTask(Task<T> task) {
    if (m_shutdown) {
        return;
    }
    // 添加任务，不需要加锁，任务队列中有锁
    m_taskQ->addTask(task);
    // 唤醒工作的线程
    pthread_cond_signal(&m_notEmpty);
}

template<typename T>
int ThreadPool<T>::getAliveNumber() {
    int threadNum = 0;
    pthread_mutex_lock(&m_lock);
    threadNum = m_aliveNum;
    pthread_mutex_unlock(&m_lock);
    return threadNum;
}

template<typename T>
int ThreadPool<T>::getBusyNumber() {
    int busyNum = 0;
    pthread_mutex_lock(&m_lock);
    busyNum = m_busyNum;
    pthread_mutex_unlock(&m_lock);
    return busyNum;
}

// 工作线程任务函数
template<typename T>
void *ThreadPool<T>::worker(void *arg) {
    ThreadPool *pool = static_cast<ThreadPool *>(arg);// 将 void* 类型指针转换为 ThreadPool类型的指针
    // 一直不停的工作
    while (true) {
        // 访问任务队列(共享资源)加锁
        pthread_mutex_lock(&pool->m_lock);
        // 判断任务队列是否为空, 如果为空工作线程阻塞
        while (pool->m_taskQ->taskNumber() == 0 && !pool->m_shutdown) {
            cout << "thread " << to_string(pthread_self()) << " waiting..." << endl;
            // 阻塞线程
            pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);

            // 解除阻塞之后, 判断是否要销毁线程
            if (pool->m_exitNum > 0) {
                pool->m_exitNum--;
                if (pool->m_aliveNum > pool->m_minNum) {
                    pool->m_aliveNum--;
                    pthread_mutex_unlock(&pool->m_lock);
                    pool->threadExit();
                }
            }
        }
        // 判断线程池是否被关闭了
        if (pool->m_shutdown) {
            pthread_mutex_unlock(&pool->m_lock);
            pool->threadExit();
        }

        // 从任务队列中取出一个任务
        Task<T> task = pool->m_taskQ->takeTask();
        // 工作的线程+1
        pool->m_busyNum++;
        // 线程池解锁
        pthread_mutex_unlock(&pool->m_lock);
        // 执行任务
        cout << "thread " << to_string(pthread_self()) << " start working..." << endl;
        task.function(task.arg);
        delete task.arg;
        task.arg = nullptr;

        // 任务处理结束
        cout << "thread " << to_string(pthread_self()) << " end working...";
        pthread_mutex_lock(&pool->m_lock);
        pool->m_busyNum--;
        pthread_mutex_unlock(&pool->m_lock);
    }

    return nullptr;

}

// 管理者线程任务函数
template<typename T>
void *ThreadPool<T>::manager(void *arg) {
    ThreadPool *pool = static_cast<ThreadPool *>(arg);// 将 void* 指针转换成 ThreadPool* 类型
    // 如果线程池没有关系，则一直检测
    while (!pool->m_shutdown) {
        // 每隔 5秒检测一次
        sleep(5);
        // 取出线程池中的任务数 和 线程数量
        // 取出工作的线程池数量
        pthread_mutex_lock(&pool->m_lock);
        int queueSize = pool->m_taskQ->taskNumber();
        int liveNum = pool->m_aliveNum;
        int busyNum = pool->m_busyNum;
        pthread_mutex_unlock(&pool->m_lock);

        // 创建线程
        const int NUMBER = 2;
        // 当当前任务个数>存活的线程数 && 存活的线程数<最大线程个数
        if (queueSize > liveNum && liveNum < pool->m_maxNum) {
            // 线程池加锁
            pthread_mutex_lock(&pool->m_lock);
            int num = 0;
            for (int i = 0; i < pool->m_maxNum && num < NUMBER
                            && pool->m_aliveNum < pool->m_maxNum; ++i) {
                if (pool->m_threadIDs[i] == 0) {
                    pthread_create(&pool->m_threadIDs[i], NULL, worker, pool);
                    num++;
                    pool->m_aliveNum++;
                }
            }
            pthread_mutex_unlock(&pool->m_lock);
        }

        // 销毁多余的线程
        // 忙线程*2 < 存活的线程数目 && 存活的线程数 > 最小线程数量
        if (busyNum * 2 < liveNum && liveNum > pool->m_minNum) {
            pthread_mutex_lock(&pool->m_lock);
            pool->m_exitNum = NUMBER;
            pthread_mutex_unlock(&pool->m_lock);
            for (int i = 0; i < NUMBER; ++i) {
                pthread_cond_signal(&pool->m_notEmpty);
            }
        }

    }
    return nullptr;
}

// 线程退出
template<typename T>
void ThreadPool<T>::threadExit() {
    pthread_t tid = pthread_self();
    for (int i = 0; i < m_maxNum; i++) {
        if (m_threadIDs[i] == tid) {
            cout << "threadExit() function: thread " << to_string(pthread_self()) << " exiting..." << endl;
            m_threadIDs[i] = 0;
            break;
        }
    }
    pthread_exit(NULL);
}