//
// Created by Huster2021 on 2023/3/13.
//
#include "../include/taskqueue.h"

template<typename T>
TaskQueue<T>::TaskQueue() {
    pthread_mutex_init(&m_mutex,NULL);
}
template<typename T>
TaskQueue<T>::~TaskQueue() {
    pthread_mutex_destroy(&m_mutex);
}
template<typename T>
void TaskQueue<T>::addTask(Task<T> &task) {
    pthread_mutex_lock(&m_mutex);
    m_queue.push(task);
    pthread_mutex_unlock(&m_mutex);
}
template<typename T>
void TaskQueue<T>::addTask(callback fun, void  *arg) {
    pthread_mutex_lock(&m_mutex);
    Task<T> task;
    task.function = fun;
    task.arg = arg;
    m_queue.push(task);
    pthread_mutex_unlock(&m_mutex);
}
template<typename T>
Task<T> TaskQueue<T>::takeTask() {
    Task<T> t;
    pthread_mutex_lock(&m_mutex);
    if(m_queue.size() > 0){
        t = m_queue.front();
        m_queue.pop();
    }
    pthread_mutex_unlock(&m_mutex);
    return t;
}
