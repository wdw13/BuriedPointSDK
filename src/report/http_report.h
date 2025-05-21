#pragma once

#include <stdint.h>

#include <memory>
#include <string>

// 声明 spdlog 日志库的 logger 类
namespace spdlog {
class logger;
}

namespace buried {

// HttpReporter 用于构建和发送 HTTP 报告的类
class HttpReporter {
 public:
  // 构造函数，接收一个 spdlog 日志器
  explicit HttpReporter(std::shared_ptr<spdlog::logger> logger);

  // 设置 host 地址，返回自身以支持链式调用
  HttpReporter& Host(const std::string& host) {
    host_ = host;
    return *this;
  }

  // 设置 topic 路径，返回自身以支持链式调用
  HttpReporter& Topic(const std::string& topic) {
    topic_ = topic;
    return *this;
  }

  // 设置端口号，返回自身以支持链式调用
  HttpReporter& Port(const std::string& port) {
    port_ = port;
    return *this;
  }

  // 设置请求体内容，返回自身以支持链式调用
  HttpReporter& Body(const std::string& body) {
    body_ = body;
    return *this;
  }

  // 执行 HTTP 报告（发送请求），返回是否成功
  bool Report();

 private:
  std::string host_;   // 服务器地址
  std::string topic_;  // 路径或主题
  std::string port_;   // 端口号
  std::string body_;   // 请求体内容

  std::shared_ptr<spdlog::logger> logger_; // 日志器
};

}  // namespace buried