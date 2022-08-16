#ifndef LOCKER_H_
#define LOCKER_H_

#include<exception>
#include<pthread.h>
#include<semaphore.h>

//定义信号量sem
class sem{
private:
    sem_t m_sem;

public:
    sem()
    {
        /*extern int sem_init (sem_t *__sem, int __pshared, unsigned int __value) 
        *
        * 用于初始化sem指定的信号量
        * sem_t *__sem：指向信号量结构的一个指针
        * int __pshared：控制信号量的类型，如果值为0，则为当前里程的局部信号量；否则，其他进程就可以共享该信号量。
        * unsigned int __value：信号量的初始值
        */
        if (sem_init(&m_sem, 0, 0) != 0)  //pshared参数传递一个非零将会使函数调用失效      
        {
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);    //释放信号量
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;   //用来阻塞当前线程直到信号量的值大于0。解除阻塞后sem的值将-1。表明公共资源经过使用后减少。
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;   //用于增加信号量的值，当有线程阻塞在该信号量上时，调用这个函数会使其中的一个线程不再阻塞。
    }
};

//互斥锁
class locker
{
private:
    pthread_mutex_t m_mutex;

public:
    locker()
    {
        /*
        * extern int pthread_mutex_init (pthread_mutex_t *__mutex,const pthread_mutexattr_t *__mutexattr)
        * pthread_mutex_t *__mutex：互斥锁地址
        * const pthread_mutexattr_t *__mutexattr：mattr属性，通常默认为0
        */
        if (pthread_mutex_init(&m_mutex, NULL) != 0)    //初始化互斥锁
        {
            throw::std::exception();
        }
    }

    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);    //销毁互斥锁
    }

    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;   //锁定互斥锁，如果尝试锁定已经被上锁的互斥锁，则阻塞直到可用
    }

    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0; //释放互斥锁
    }

    pthread_mutex_t* get()  //获取互斥锁
    {
        return &m_mutex;
    }
};

//条件变量
class cond
{
private:
    pthread_cond_t m_cond;

public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)  //初始化条件变量
        {
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);  //销毁条件变量
    }
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        ret = pthread_cond_wait(&m_cond, m_mutex);  //阻塞条件变量
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex,struct timespec t)
    {
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t); //阻塞条件变量直到指定时间
        return ret == 0;

    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;   //唤醒第一个调用pthread_cond_wait()而进入睡眠的线程
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;    //释放所有阻塞的条件变量
    }
};


#endif
