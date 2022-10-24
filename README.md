# LinuxDemo
用于个人学习Linux下C/C++程序开发仓库

# Tips
在使用SSH连接Linux远程主机或虚拟机之后，需要做如下设置，确保Visual Studio能正确加载Linux主机中的所有库文件：

在**项目名称右键->属性->VC++目录->常规->包含目录**下，添加Linux主机的库目录绝对路径，如：`usr/lib`

在**属性->C/C++->常规->附加包含目录**下，添加`${StlIncludeDirectories}`
在**属性->链接器->常规->附加库目录**下，添加`%(AdditionalLibraryDirectories)`

如果找不到<mysql/mysql.h>包文件，请在Linux虚拟机下执行：
```Shell
sudo apt-get install libmysqlclient-dev
```

# Day 01 自定义 block_queue 的实现
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

# Day 02 数据库连接池 SQL_connectionPool 的实现
数据库连接池中的资源由程序动态地对其进行使用和释放。

当系统开始处理客户请求时，如果需要相关资源，可以直接从池中获取，无需动态分配。而当系统处理完当前客户连接后，可以将相关资源放回池中，无需执行系统调用释放资源。

## 数据库访问的一般流程
1. 系统创建数据库连接
2. 完成数据库操作
3. 断开数据库连接

## 数据库连接池的具体实现
+ **采用单例模式**
+ **RAII 机制释放数据库连接**

### 单例模式
使用局部静态变量懒汉式创建连接池
```CPP
//接口定义
class connection_pool
{
public:
	//使用单例模式
	static connection_pool* GetInstance ();
	
private:
	connection_pool();
	~connection_pool ();
}

//类实现
connection_pool* connection_pool::GetInstance()
{
	static connection_pool connPool;
	return &connPool;
}
```
数据库连接池实现内容：
1. 连接池初始化
	```CPP
	connection_pool::connection_pool()
	{
		m_CurConn = 0;
		m_FreeConn = 0;
	}
	
	//初始化
	void connection_pool::init(string url, string user, 
							string password, string database, 
							int port, int maxConn, int closeLog)
	{
		//初始化数据库信息
		m_Url = url;
		m_User = user;
		m_Password = password;
		m_Database = database;
		m_Port = port;
		m_close_log = closeLog;
		//创建MaxConn条数据库库连接
		for (int i = 0; i < maxConn; i++)
		{
			MYSQL* con = NULL;
			con = mysql_init(con);
	
			if (con == NULL)
			{
				LOG_ERROR("MYSQL ERROR");
				exit(1);
			}
			con = mysql_real_connect(con, url.c_str(), user.c_str(), password.c_str(), database.c_str(), port, NULL, 0);
	
			if (con == NULL)
			{
				LOG_ERROR("MYSQL ERROR");
				exit(1);
			}
			//更新数据库连接池和空闲连接数量
			connList.push_back(con);
			++m_FreeConn;
		}
		//将信号量初始化为最大连接次数
		reserve = sem(m_FreeConn);
		m_MaxConn = m_FreeConn;
	}
	```
2. 获取数据库连接池
	当线程数大于数据库连接数时，将使用信号量进行同步，每次取出连接，信号量原子-1，释放连接，信号量原子+1，如果连接池内没有连接，则阻塞等待。
	```CPP
	//当有数据库连接请求时，从数据库连接池返回一个可用连接，更新使用和空闲连接数
	MYSQL* connection_pool::GetConnection()
	{
		MYSQL* conn = NULL;
	
		if (0 == connList.size())
		{
			return NULL;
		}
		//取出连接，信号量原子-1，如果信号量原子为0，则阻塞等待
		reserve.wait();
	
		lock.lock();
	
		conn = connList.front();
		connList.pop_front();
		//具体暂未使用下方变量，仅做标记使用
		--m_FreeConn;//创建连接后，空闲连接-1
		++m_CurConn;//现有连接+1
	
		lock.unlock();
		return conn;
	}
	```
3. 释放数据库连接池
	```CPP
	//释放当前使用的连接
	bool connection_pool::ReleaseConnection(MYSQL* conn)
	{
		if (NULL == conn) 
		{
			return false;
		}	
	
		lock.lock();
	
		connList.push_back(conn);
		++m_FreeConn;
		--m_CurConn;
	
		lock.unlock();
		//释放后信号量原子+1
		reserve.post();
		return true;
	}
	```
4. 销毁数据库连接池
	通过迭代器遍历连接池链表，关闭对应数据库连接，清空链表并重置空闲连接和现有的连接数
	```CPP
	//销毁数据库连接池
	void connection_pool::DestoryPool()
	{
		lock.lock();
		if (connList.size() > 0)
		{
			//使用迭代器遍历，关闭数据库连接
			list<MYSQL*>::iterator it;
			for (it = connList.begin(); it != connList.end(); ++it)
			{
				MYSQL* conn = *it;
				mysql_close(conn);
			}
			m_CurConn = 0;
			m_FreeConn = 0;
			//清空List
			connList.clear();
		}
		lock.unlock();
	}
	```


### RAII 机制
RAII 机制通过在栈上创建临时变量，此时临时变量接管了堆上内存的控制权，当该临时变量声明周期结束时，则对应的堆上内存自然被释放。

使用 RAII 机制可以避免手动释放内存，防止内存泄露现象的出现

在获取连接时，通过有参构造函数对传入的参数进行修改，其中**数据库连接本身是指针类型数据，所以参数需要通过双指针才能对其进行修改。**

RAII 的接口定义
```CPP
class connection_pool_RAII
{
private:
	MYSQL* conRAII;
	connection_pool* poolRAII;
public:
	//双指针对MYSQL *conRAII进行修改
	connection_pool_RAII(MYSQL** con, connection_pool* coonPool);
	~connection_pool_RAII();
};
```
RAII 的具体实现
```CPP
connection_pool_RAII::connection_pool_RAII(MYSQL** SQL, connection_pool* coonPool)
{
	*SQL = coonPool->GetConnection();
	conRAII = *SQL;
	poolRAII = coonPool;
}

connection_pool_RAII::~connection_pool_RAII()
{
	poolRAII->ReleaseConnection(conRAII);
}
```

# Day 03 定时器
## 基础定义
+ **非活跃**：客户端与服务器建立连接后，长时间不交换数据，一直占用服务器端的文件描述符，导致资源浪费。
+ **定时事件**：指固定一段时间后，自动触发某段代码，由该段代码处理一个事件。
+ **定时器**：指利用结构体或者其他形式，将多种定时事件封装起来。
+ **定时器容器**：指使用某种容器类数据结构，将上述多个定时器组合起来，便于对定时事件统一管理。

## 具体使用
在服务器主循环，为每一个连接创建一个定时器，并对每个连接进行定时。

同时利用升序时间链表容器，串联所有定时器，若主循环接收到定时通知，则在链表中依次执行定时任务。

> Linux 下提供的三种定时方法：
+ socket 选项 `SO_RECVTIMEO` 和 `SO_SNDTIMEO`
+ `SIGALRM` 信号
+ I/O 复用系统调用的超时参数
在 Demo 中，采用 `SIGALRM` 信号的形式定时：
```CPP
//定时器链表：一个升序的双向链表，且带有头结点和尾结点
class sort_timer_lst
{
public:
	sort_timer_lst();
	~sort_timer_lst();

	void add_timer(util_timer* timer);
	void adjust_timer(util_timer* timer);
	void del_timer(util_timer* timer);
	void tick();

private:
	void add_timer(util_timer* timer, util_timer* lst_head);

	util_timer* head;
	util_timer* tail;

};
```

## 使用的 API
### `sigaction` 结构体
```CPP
struct sigaction
  {
	void (*sa_handler)(int); //sa_handler函数指针，指向信号处理函数 
    void (*sa_sigaction)(int, siginfo_t *, void *);  
    sigset_t sa_mask;  //指定在信号处理函数执行期间需要被屏蔽的信号
    int sa_flags;  //指定信号处理的行为
    void (*sa_restorer)(void);
  };
```
`sa_flags` 的行为包括：
+ `SA_RESTART` ：使被信号打断的系统调用自动重新发起
+ `SA_NOCLDSTOP` ：使父进程在它的子进程暂停或者继续运行时不会收到 `SIGCHLD` 信号
+ `SA_NOCLDWAIT` ：使父进程在它的子进程退出时不会收到 `SIGCHLD` 信号，这时子进程如果退出也不会成为僵尸进程
+ `SA_NODEFER` ：使对信号的屏蔽无效，在信号处理函数执行期间仍然能够发出这个信号
+ `SA_RESETHAND` ：信号处理之后重新设置为默认的处理方式
+ `SA_SIGINFO` ：使用 `sa_sigaction` 成员而不是 `sa_handler` 作为信号处理函数
