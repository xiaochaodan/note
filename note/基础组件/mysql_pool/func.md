# 函数随笔

## mysql

- mysql_thread_init():初始化当前线程所需的 MySQL 客户端环境

- mysql_thread_end():清理该线程的 MySQL 客户端环境

- static thread_local MySQLThreadInit t_mysql_thread_init

- void mysql_close(MYSQL *mysql); 终止与 MySQL 服务器的连接，并释放与该连接相关的资源

- int mysql_real_query(MYSQL *mysql, const char *stmt_str, unsigned long length);

  - 用于执行 SQL 语句的同步函数

  - **`mysql`**：由 `mysql_init()` 初始化的连接句柄。
  - **`stmt_str`**：指向要执行的 SQL 语句的字符串。
  - **`length`**：`stmt_str` 的长度（以字节为单位）

- unsigned int mysql_errno(MYSQL *mysql);

  - **`mysql`**：指向已初始化的 `MYSQL` 连接句柄的指针。
  - 返回最近一次执行产生得错误码，没有就是0

- const char *mysql_error(MYSQL *mysql);

  - 返回一个指向包含错误消息的 null 终止字符串的指针。如果没有发生错误，返回空字符串

- unsigned int mysql_field_count(MYSQL *mysql);

  - 返回最近一次查询结果集中的列数。如果返回值为 `0`，表示该查询未返回任何列
  - `mysql`：指向已初始化的 `MYSQL` 连接句柄的指针

- MYSQL_RES *mysql_store_result(MYSQL *mysql);

  - `mysql`：指向已连接的 `MYSQL` 对象的指针。
  - 成功时，返回指向 `MYSQL_RES` 结构的指针，表示查询结果集，否则NULL

- MYSQL_ROW mysql_fetch_row(MYSQL_RES *result);

  - 指向 `MYSQL_RES` 结构的指针，表示查询结果集

  - 成功时，返回类型为 `MYSQL_ROW` 的指针，表示结果集中的下一行数据

    ```cpp
    typedef char **MYSQL_ROW; /* 返回数据作为字符串数组 */
    ```

- void mysql_free_result(MYSQL_RES *result);

  - 用于释放由 mysql_store_result() 或 mysql_use_result() 分配的结果集内存

- unsigned int mysql_num_fields(MYSQL_RES *result);

  - 用于返回结果集中字段（列）的数量。

- my_ulonglong mysql_affected_rows(MYSQL *mysql);

  - 用于返回最近一次执行的 `INSERT`、`UPDATE`、`DELETE` 或 `REPLACE` 语句所影响的行数

- int mysql_options(MYSQL *mysql, enum mysql_option option, const void *arg);

  ​	设置mysql连接选项

- MYSQL *mysql_real_connect(MYSQL *mysql,  ----由 mysql_init() 初始化的连接句柄

  ​             const char *host,  ----服务器主机名或 IP 地址

  ​             const char *user,  ----登录用户名

  ​             const char *passwd, ----用户密码

  ​             const char *db,   ----默认连接的数据库名

  ​             unsigned int port,  ----服务器端口

  ​             const char *unix_socket,   ----unix系统下的套接字路径

  ​             unsigned long client_flag); ----连接选项标志

    用来建立数据库连接

## chrono（计时）

```mysql
std::chrono::time_point<std::chrono::steady_clock>dealine=
    std::chrono::steady_clock::now()+sd::chrono::seconds(connectionTimeout_);
timespec ts;
std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds> secs = std::chrono::time_point_cast<std::chrono::seconds>(deadline);
std::chrono::nanoseconds nsecs =std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - secs);
    ts.tv_sec = secs.time_since_epoch().count();
    ts.tv_nsec = nsecs.count();
/************
*std::chrono::time_point:
*        template<
*            class Clock,
*            class Duration = typename Clock::duration
*        > class time_point;
*Clock:时钟种类，如system_clock(系统范围的时钟时间)，steady_clock(永不调整的单调时钟)
*   这两个的成员类型
*       rep 表示时钟持续时间中刻度数的算术类型
*       period  一个 std::ratio 类型，表示时钟的嘀嗒周期，以秒为单位
*       duration    std::chrono::duration<rep, period>
*       time_point  std::chrono::time_point<std::chrono::steady_clock>
*Duration:表示时长，类型为duration
*       template<
*       class Rep,          //节拍数
*       class Period = std::ratio<1>    //节拍周期
*   > class duration;
*now():stready_clock的成员，返回类型为std::chrono::time_point的当前时间节点
*time_point成员函数:
*1. duration time_since_epoch() const;
*   返回一个 duration，表示 *this 和时钟纪元之间的时间量
*
*2. time_point_cast:
*   template< class ToDuration, class Clock, class Duration >
*   std::chrono::time_point<Clock, ToDuration>
*   time_point_cast( const std::chrono::time_point<Clock, *Duration> &t );
*   用于将一个 time_point（时间点）转换成具有不同 duration 类型的 time_point，底层即是对该时间点的time_since_epoch() *调用 duration_cast，再重建新的时间点。
*timespec:保存分解的秒和纳秒的时间间隔
*struct timespec{
*   time_t tv_sec;      //秒
*   long tv_nsec;       //纳秒
*}；
*************/
```
