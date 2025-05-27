# Makefile

# 目标可执行文件名称
TARGET := test

# 源文件列表
SRCS := ConnectionPool.cpp test.cpp

# 根据源文件自动生成 .o 文件列表
OBJS := $(SRCS:.cpp=.o)

# C++ 编译器及标准
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread \
             $(shell pkg-config --cflags mysqlclient)

# 链接选项
LDFLAGS := -pthread \
           $(shell pkg-config --libs mysqlclient)

.PHONY: all clean

# 默认目标：编译可执行文件
all: $(TARGET)

# 链接可执行文件
$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

# 编译 .cpp 到 .o
%.o: %.cpp ConnectionPool.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理中间文件和可执行文件
clean:
	rm -f $(OBJS) $(TARGET)
