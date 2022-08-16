# LinuxDemo
用于个人学习Linux下C/C++程序开发仓库

# Day 01 自定义block_queue的实现
自定义`block_queue`主要采用循环数组来实现，其中内部成员定义如下：
```CPP
private:
	locker m_mutex;
	cond m_cond;

	T* m_array;
	int m_size;
	int m_max_size;
	int m_front;	//front标记
	int m_back;		//back标记
```
其中 `locker` 与 `cond` 分别为互斥锁与条件变量。

在自定义 `block_queue` 中，主要对如下函数进行了实现：
```CPP
	block_queue(int max_size = 1000)//构造函数
	void clear()//清空队列
	~block_queue()//析构函数
	bool isFull()//判断队列是否已填满
	bool isEmpty()//判断队列是否为空
	bool front(T &value)//获取队首元素
	bool back(T& value)//获取队尾元素
	int size()//获取队列当前长度
	int max_size()//获取队列的最大长度
	bool push(const T &item)//将元素加入队列
	bool pop(T& item)//将元素从队列中弹出
	bool pop(T& item, int ms_timeout)//在弹出元素时，增加了超时处理
```
> 要注意的是：在操作队列前，都需要使用 `lock ()` 方法对队列加锁，在操作完成之后，都需要使用 `unlock ()` 对队列进行解锁。

为了保证多线程访问时，不出现资源竞争的问题，因此加入了锁操作，使得每个线程互斥地访问公有资源。
