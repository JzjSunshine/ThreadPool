//
// Created by Huster2021 on 2023/3/12.
//

#ifndef THREADPOOL_TASKQUEUE_H
#define THREADPOOL_TASKQUEUE_H

#include <pthread.h>
#include <iostream>
#include <queue>
// 任务结构体

// 使用 using 定义类型别名，使得一个已有类型可以用另一个名称表示
using callback = void(*)(void*);
template<typename T>
struct Task{
    Task(){
        function = nullptr;
        arg = nullptr;
    }
    Task(callback f,void*arg){
        function = f;
        this->arg = (T*)arg;
    }

    callback function;
    T *arg;
};
// 任务队列
template<typename T>
class TaskQueue{
public:
    TaskQueue();
    ~TaskQueue();

    // 添加任务
    void addTask(Task<T>& task);
    void addTask(callback func, void*arg);

    //取出一个任务
    Task<T> takeTask();

    // 获取当前队列中的任务个数
    // 内联函数在调用点展开，避免函数调用的开销，提高执行效率
    // 内联变量：可以在多个翻译单元中共享的变量，避免重复定义的问题
    inline int taskNumber()
    {
        return m_queue.size();
    }

private:
    pthread_mutex_t m_mutex;// 互斥锁
    std::queue<Task<T>> m_queue; // 任务队列
};
#endif //THREADPOOL_TASKQUEUE_H
