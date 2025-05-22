#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "boost/asio/io_context.hpp"
#include "boost/asio/io_context_strand.hpp"

namespace buried {

// Context 类用于管理全局异步 IO 上下文和线程，支持主逻辑和上报逻辑的分离
class Context {
 public:
  // 获取全局唯一的 Context 实例（单例模式）
  static Context& GetGlobalContext() {
    static Context global_context;
    return global_context;
  }

  // 析构函数，负责资源释放
  ~Context();

  // 类型别名，方便外部使用
  using Strand = boost::asio::io_context::strand;
  using IOContext = boost::asio::io_context;

  // 获取主逻辑的 strand（用于保证主逻辑任务的串行执行）
  Strand& GetMainStrand() { return main_strand_; }

  // 获取上报逻辑的 strand（用于保证上报任务的串行执行）
  Strand& GetReportStrand() { return report_strand_; }

  // 获取主逻辑的 io_context
  IOContext& GetMainContext() { return main_context_; }

  // 启动主线程和上报线程，运行 io_context
  void Start();

 private:
  // 构造函数，初始化 strand，分别绑定到各自的 io_context
  Context() : main_strand_(main_context_), report_strand_(report_context_) {}

  // 禁止拷贝构造和赋值，保证单例唯一性
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;

 private:
  boost::asio::io_context main_context_;   // 主逻辑的 io_context
  boost::asio::io_context report_context_; // 上报逻辑的 io_context

  boost::asio::io_context::strand main_strand_;   // 主逻辑串行器
  boost::asio::io_context::strand report_strand_; // 上报逻辑串行器

  std::unique_ptr<std::thread> main_thread_;   // 主逻辑线程
  std::unique_ptr<std::thread> report_thread_; // 上报逻辑线程

  std::atomic<bool> is_start_{false}; // 标记是否已启动
  std::atomic<bool> is_stop_{false};  // 标记是否已停止
};

}  // namespace buried