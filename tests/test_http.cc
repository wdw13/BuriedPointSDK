#include <cstdlib>
#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "src/third_party/boost/asio/connect.hpp"
#include "src/third_party/boost/asio/ip/tcp.hpp"
#include "src/third_party/boost/beast/core.hpp"
#include "src/third_party/boost/beast/http.hpp"
#include "src/third_party/boost/beast/version.hpp"

// 命名空间别名
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// 执行一个HTTP POST请求并打印响应内容的单元测试
TEST(BuriedHttpTest, DISABLED_HttpPost) {
  try {
    auto const host = "localhost";  // 目标主机
    auto const target = "/buried";  // 请求路径
    int version = 11;               // HTTP 1.1

    // 所有I/O操作都需要io_context
    net::io_context ioc;

    // 创建解析器和TCP流对象用于I/O
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    // 解析主机名和端口
    boost::asio::ip::tcp::resolver::query query(host, "5678");
    auto const results = resolver.resolve(query);

    // 连接到解析得到的IP地址
    stream.connect(results);

    // 构造HTTP POST请求
    http::request<http::string_body> req{http::verb::post, target, version};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/json");
    req.body() = "{\"hello\":\"world\"}";
    req.prepare_payload();

    // 发送HTTP请求
    http::write(stream, req);

    // 用于读取响应的缓冲区
    beast::flat_buffer buffer;

    // 用于存放响应的对象
    http::response<http::dynamic_body> res;

    // 接收HTTP响应
    http::read(stream, buffer, res);

    // 提取响应体内容并打印
    std::string bdy = boost::beast::buffers_to_string(res.body().data());
    std::cout << "bdy " << bdy << std::endl;
    // 打印完整响应
    std::cout << "res " << res << std::endl;
    // 打印响应码
    std::cout << "res code " << res.result_int() << std::endl;

    // 优雅关闭socket
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    // 有些情况下会出现not_connected错误，可以忽略
    if (ec && ec != beast::errc::not_connected) throw beast::system_error{ec};

    // 到这里说明连接已正常关闭
  } catch (std::exception const& e) {
    // 捕获并打印异常信息
    std::cerr << "Error: " << e.what() << std::endl;
  }
}