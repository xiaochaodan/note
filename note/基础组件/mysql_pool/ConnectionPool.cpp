#include "ConnectionPool.hpp"
#include <stdexcept>
#include <iostream>
#include <unistd.h>

ConnectionPool& ConnectionPool::instance() {
    static ConnectionPool pool;
    return pool;
}

// 构造函数：初始化锁和条件变量
ConnectionPool::ConnectionPool() {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&cond_, nullptr);
}

// 析构函数：释放资源
ConnectionPool::~ConnectionPool() {
    destroy();
    pthread_cond_destroy(&cond_);
    pthread_mutex_destroy(&mutex_);
}

// 初始化连接池
void ConnectionPool::init(const std::string& host,
                          const std::string& user,
                          const std::string& password,
                          const std::string& database,
                          unsigned int port,
                          size_t initSize,
                          size_t maxSize,
                          unsigned int connectionTimeout,
                          unsigned int maxIdleTime) {
    // 保存连接信息和参数
    host_ = host;
    user_ = user;
    password_ = password;
    database_ = database;
    port_ = port;
    initSize_ = initSize;
    maxSize_ = maxSize;
    connectionTimeout_ = connectionTimeout;
    maxIdleTime_ = maxIdleTime;

    // 预创建初始连接
    pthread_mutex_lock(&mutex_);
    for (size_t i = 0; i < initSize_; ++i) {
        MYSQL* conn = createConnection();
        if (conn) {
            pool_.push_back({conn, std::chrono::steady_clock::now()});
            ++curSize_;
        }
    }
    pthread_mutex_unlock(&mutex_);

    // 启动空闲连接回收线程
    shuttingDown_ = false;
    reclaimerThread_ = std::thread(&ConnectionPool::idleReclaimer, this);
}

// 创建一个 MySQL 连接
MYSQL* ConnectionPool::createConnection() {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) return nullptr;
    if (!mysql_real_connect(conn,
                            host_.c_str(),
                            user_.c_str(),
                            password_.c_str(),
                            database_.c_str(),
                            port_,
                            nullptr, 0)) {
        std::cerr << "MySQL connect error: " << mysql_error(conn) << "\n";
        mysql_close(conn);
        return nullptr;
    }
    return conn;
}

// 获取连接（带阻塞等待 + 超时）
ConnectionPtr ConnectionPool::getConnection() {
    MYSQL* conn = nullptr;
    pthread_mutex_lock(&mutex_);
    // 设置超时时间点
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(connectionTimeout_);
    timespec ts;
    auto secs = std::chrono::time_point_cast<std::chrono::seconds>(deadline);
    auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - secs);
    ts.tv_sec = secs.time_since_epoch().count();
    ts.tv_nsec = nsecs.count(); 

    // 没有连接可用且不能扩容，等待条件变量
    while (pool_.empty() && curSize_ >= maxSize_) {
        int rc = pthread_cond_timedwait(&cond_, &mutex_, &ts);
        if (rc == ETIMEDOUT) break;
    }

    // 优先复用空闲连接
    if (!pool_.empty()) {
        auto pc = pool_.front();
        pool_.pop_front();
        conn = pc.conn;
    }
    // 可以扩容时创建新连接
    else if (curSize_ < maxSize_) {
        conn = createConnection();
        if (conn) ++curSize_;
    }

    pthread_mutex_unlock(&mutex_);

    if (!conn) {
        throw std::runtime_error("获取 MySQL 连接失败：超时");
    }

    return ConnectionPtr(conn, ConnectionReclaimer());
}

// 回收连接（归还到连接池并唤醒等待线程）
void ConnectionPool::recycleConnection(MYSQL* conn) {
    pthread_mutex_lock(&mutex_);
    pool_.push_back({conn, std::chrono::steady_clock::now()});
    pthread_cond_signal(&cond_); // 唤醒一个等待线程
    pthread_mutex_unlock(&mutex_);
}

// 后台线程：定时回收长时间未使用的连接
void ConnectionPool::idleReclaimer() {
    while (true) {
        sleep(maxIdleTime_ / 2 + 1); // 每隔一段时间清理一次

        pthread_mutex_lock(&mutex_);
        if (shuttingDown_) {
            pthread_mutex_unlock(&mutex_);
            break;
        }

        auto now = std::chrono::steady_clock::now();

        for (auto it = pool_.begin(); it != pool_.end();) {
            if (curSize_ <= initSize_) break; // 保留初始连接
            auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(now - it->last_used).count();
            if (idleTime > static_cast<long>(maxIdleTime_)) {
                mysql_close(it->conn);
                it = pool_.erase(it);
                --curSize_;
            } else {
                ++it;
            }
        }

        pthread_mutex_unlock(&mutex_);
    }
}

// 销毁连接池（回收线程 + 关闭所有连接）
void ConnectionPool::destroy() {
    pthread_mutex_lock(&mutex_);
    shuttingDown_ = true;
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);

    if (reclaimerThread_.joinable()) {
        reclaimerThread_.join();
    }

    pthread_mutex_lock(&mutex_);
    for (auto& pc : pool_) {
        mysql_close(pc.conn);
    }
    pool_.clear();
    curSize_ = 0;
    pthread_mutex_unlock(&mutex_);
}
