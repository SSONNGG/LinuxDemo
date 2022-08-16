#ifndef LOCKER_H_
#define LOCKER_H_

#include<exception>
#include<pthread.h>
#include<semaphore.h>

//�����ź���sem
class sem{
private:
    sem_t m_sem;

public:
    sem()
    {
        /*extern int sem_init (sem_t *__sem, int __pshared, unsigned int __value) 
        *
        * ���ڳ�ʼ��semָ�����ź���
        * sem_t *__sem��ָ���ź����ṹ��һ��ָ��
        * int __pshared�������ź��������ͣ����ֵΪ0����Ϊ��ǰ��̵ľֲ��ź����������������̾Ϳ��Թ�����ź�����
        * unsigned int __value���ź����ĳ�ʼֵ
        */
        if (sem_init(&m_sem, 0, 0) != 0)  //pshared��������һ�����㽫��ʹ��������ʧЧ      
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
        sem_destroy(&m_sem);    //�ͷ��ź���
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;   //����������ǰ�߳�ֱ���ź�����ֵ����0�����������sem��ֵ��-1������������Դ����ʹ�ú���١�
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;   //���������ź�����ֵ�������߳������ڸ��ź�����ʱ���������������ʹ���е�һ���̲߳���������
    }
};

//������
class locker
{
private:
    pthread_mutex_t m_mutex;

public:
    locker()
    {
        /*
        * extern int pthread_mutex_init (pthread_mutex_t *__mutex,const pthread_mutexattr_t *__mutexattr)
        * pthread_mutex_t *__mutex����������ַ
        * const pthread_mutexattr_t *__mutexattr��mattr���ԣ�ͨ��Ĭ��Ϊ0
        */
        if (pthread_mutex_init(&m_mutex, NULL) != 0)    //��ʼ��������
        {
            throw::std::exception();
        }
    }

    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);    //���ٻ�����
    }

    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;   //������������������������Ѿ��������Ļ�������������ֱ������
    }

    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0; //�ͷŻ�����
    }

    pthread_mutex_t* get()  //��ȡ������
    {
        return &m_mutex;
    }
};

//��������
class cond
{
private:
    pthread_cond_t m_cond;

public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)  //��ʼ����������
        {
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);  //������������
    }
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        ret = pthread_cond_wait(&m_cond, m_mutex);  //������������
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex,struct timespec t)
    {
        int ret = 0;
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t); //������������ֱ��ָ��ʱ��
        return ret == 0;

    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;   //���ѵ�һ������pthread_cond_wait()������˯�ߵ��߳�
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;    //�ͷ�������������������
    }
};


#endif
