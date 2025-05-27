#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <mysql/mysql.h>
#include <string>
#include <deque>
#include <memory>
#include <thread>
#include <chrono>
#include <pthread.h>

// 连接类型别名：shared_ptr 自动管理连接回收
using ConnectionPtr = std::shared_ptr<MYSQL>;

// 每个连接附带上一次使用时间，用于空闲判断
struct PooledConnection {
    MYSQL* conn;
    std::chrono::steady_clock::time_point last_used;
};

// 连接池类（单例）
class ConnectionPool {
public:
    // 获取单例对象
    static ConnectionPool& instance();

    // 初始化连接池
    void init(const std::string& host,
              const std::string& user,
              const std::string& password,
              const std::string& database,
              unsigned int port,
              size_t initSize,
              size_t maxSize,
              unsigned int connectionTimeout,
              unsigned int maxIdleTime);

    // 从连接池获取一个连接
    ConnectionPtr getConnection();

    // 销毁连接池
    void destroy();

private:
    ConnectionPool();
    ~ConnectionPool();

    // 禁止拷贝构造和赋值
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    // 创建新连接
    MYSQL* createConnection();

    // 回收连接，放回连接池
    void recycleConnection(MYSQL* conn);

    // 后台线程：定期回收空闲连接
    void idleReclaimer();

    // 连接池容器（双端队列）
    std::deque<PooledConnection> pool_;

    // 当前连接数、初始连接数、最大连接数
    size_t curSize_{0};
    size_t initSize_{0};
    size_t maxSize_{0};

    // 最大空闲时间、连接获取超时
    unsigned int connectionTimeout_{0};
    unsigned int maxIdleTime_{0};

    // 关闭标志（用于关闭回收线程）
    bool shuttingDown_{false};

    // POSIX 互斥锁 + 条件变量
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

    // MySQL 连接信息
    std::string host_, user_, password_, database_;
    unsigned int port_{3306};

    // 回收线程
    std::thread reclaimerThread_;

    friend struct ConnectionReclaimer;
    
};

// 自定义 deleter，当 shared_ptr 销毁时归还连接
struct ConnectionReclaimer {
    void operator()(MYSQL* conn) {
        ConnectionPool::instance().recycleConnection(conn);
    }
};

#endif // CONNECTION_POOL_H
