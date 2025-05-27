#include <chrono>
#include <iostream>
#include <thread>

#include "boost/asio.hpp"
#include "gtest/gtest.h"

// 测试多个 strand 和 io_context 的基本用例
TEST(BuriedExecutorTest, DISABLED_BasicTest) {
  // 创建三个 io_context 对象，用于处理异步事件
  boost::asio::io_context io_context1;
  boost::asio::io_context io_context2;
  boost::asio::io_context io_context3;

  // 为每个 io_context 创建对应的 strand，用于序列化操作
  boost::asio::io_context::strand strand1(io_context1);
  boost::asio::io_context::strand strand2(io_context2);
  boost::asio::io_context::strand strand3(io_context3);

  // 在 strand1 上投递任务1
  boost::asio::post(strand1, []() {
    std::cout << "Operation 1 executed in strand1 on thread id "
              << std::this_thread::get_id() << std::endl;
  });

  // 在 strand2 上投递任务2，该任务又会投递其他任务
  boost::asio::post(strand2, [&]() {
    std::cout << "Operation 2 executed in strand2 on thread id "
              << std::this_thread::get_id() << std::endl;

    // 在 strand2 上投递任务3
    boost::asio::post(strand2, []() {
      std::cout << "Operation 3 executed in strand2 on thread id "
                << std::this_thread::get_id() << std::endl;
    });

    // 在 strand1 上投递任务4
    boost::asio::post(strand1, []() {
      std::cout << "Operation 4 executed in strand1 on thread id "
                << std::this_thread::get_id() << std::endl;
    });

    // 在 strand3 上投递任务5
    boost::asio::post(strand3, []() {
      std::cout << "Operation 5 executed in strand3 on thread id "
                << std::this_thread::get_id() << std::endl;
    });
  });

  // 在 strand3 上投递任务6
  boost::asio::post(strand3, []() {
    std::cout << "Operation 6 executed in strand3 on thread id "
              << std::this_thread::get_id() << std::endl;
  });

  // 创建工作线程运行 io_context1 和 io_context2
  std::thread io_thread([&]() {
    io_context1.run();
    io_context2.run();

    // 在 strand3 上投递任务7
    boost::asio::post(strand3, []() {
      std::cout << "Operation 7 executed in strand3 on thread id "
                << std::this_thread::get_id() << std::endl;
    });

    // 循环运行 io_context
    for (int i = 0; i < 20; ++i) {
      io_context1.run();
      io_context2.run();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  // 在 strand3 上投递任务8
  boost::asio::post(strand3, []() {
    std::cout << "Operation 8 executed in strand3 on thread id "
              << std::this_thread::get_id() << std::endl;
  });

  // 在主线程运行 io_context3
  io_context3.run();

  std::cout << "Main thread id: " << std::this_thread::get_id() << std::endl;

  std::this_thread::sleep_for(std::chrono::seconds(1));

  // 在 strand3 上投递任务9
  boost::asio::post(strand3, []() {
    std::cout << "Operation 9 executed in strand3 on thread id "
              << std::this_thread::get_id() << std::endl;
  });

  // 循环运行 io_context3
  for (int i = 0; i < 20; ++i) {
    io_context3.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // 等待工作线程结束
  io_thread.join();
}

// 定时器测试类
class printer {
 public:
  // 构造函数，初始化 strand 和两个定时器
  printer(boost::asio::io_service& io)
      : strand_(io),
        timer1_(io, boost::posix_time::seconds(1)),
        timer2_(io, boost::posix_time::seconds(1)),
        count_(0) {
    // 启动两个定时器，使用 strand 包装回调以确保线程安全
    timer1_.async_wait(strand_.wrap(std::bind(&printer::print1, this)));
    timer2_.async_wait(strand_.wrap(std::bind(&printer::print2, this)));
  }

  // 析构函数，打印最终计数
  ~printer() { std::cout << "Final count is " << count_ << std::endl; }

  // 定时器1的回调函数
  void print1() {
    if (count_ < 10) {
      std::cout << "Timer 1: " << count_ << std::endl;
      ++count_;

      // 设置下一次触发时间并继续等待
      timer1_.expires_at(timer1_.expires_at() + boost::posix_time::seconds(1));
      timer1_.async_wait(strand_.wrap(std::bind(&printer::print1, this)));
    }
  }

  // 定时器2的回调函数
  void print2() {
    if (count_ < 10) {
      std::cout << "Timer 2: " << count_ << std::endl;
      ++count_;

      // 设置下一次触发时间并继续等待
      timer2_.expires_at(timer2_.expires_at() + boost::posix_time::seconds(1));
      timer2_.async_wait(strand_.wrap(std::bind(&printer::print2, this)));
    }
  }

 private:
  boost::asio::io_service::strand strand_;  // 用于序列化操作的 strand
  boost::asio::deadline_timer timer1_;      // 定时器1
  boost::asio::deadline_timer timer2_;      // 定时器2
  int count_;                               // 计数器
};

// 定时器功能测试
TEST(BuriedExecutorTest, TimerTest) {
  boost::asio::io_service io;
  printer p(io);                    // 创建 printer 对象
  std::thread t([&]() { io.run(); }); // 在新线程中运行 io_service
  t.join();                         // 等待线程结束
}