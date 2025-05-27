// test.cpp
#include "ConnectionPool.hpp"
#include <iostream>

int main() {
    // 1. 初始化连接池（根据实际环境替换参数）
    ConnectionPool::instance().init(
        "127.0.0.1",      // host
        "tom",       // user
        "2548635ww",   // password
        "testdb",         // database
        3306,             // port
        2,                // initSize
        10,               // maxSize
        5000,             // connectionTimeout (ms)
        60000             // maxIdleTime (ms)
    );

    {
        // 2. 从连接池获取一个连接
        auto connPtr = ConnectionPool::instance().getConnection();

        // 3. 执行简单查询:SELECT VERSION()
        if (mysql_query(connPtr.get(), "SELECT VERSION()")) {
            std::cerr << "Query failed: " << mysql_error(connPtr.get()) << "\n";
            return EXIT_FAILURE;
        }

        MYSQL_RES* result = mysql_store_result(connPtr.get());
        if (!result) {
            std::cerr << "Store result failed: " << mysql_error(connPtr.get()) << "\n";
            return EXIT_FAILURE;
        }

        // 获取并打印第一行第一列
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row && row[0]) {
            std::cout << "MySQL version: " << row[0] << "\n";
        }

        mysql_free_result(result);
        // 4. 离开作用域时,connPtr 的自定义删除器会自动调用 recycleConnection()
    }

    // 5. 销毁连接池并停止后台回收线程
    ConnectionPool::instance().destroy();

    return EXIT_SUCCESS;
}
