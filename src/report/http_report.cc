#include "report/http_report.h"

#include "boost/asio/connect.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/version.hpp"
#include "spdlog/spdlog.h"

// 命名空间别名，方便后续代码书写
namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace http = beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>
using tcp = net::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

namespace buried {

// 全局 io_context 对象，用于所有 I/O 操作
static boost::asio::io_context ioc;

// 构造函数，初始化 logger_
HttpReporter::HttpReporter(std::shared_ptr<spdlog::logger> logger)
    : logger_(logger) {}

// 执行 HTTP 报告（发送 POST 请求），返回是否成功
bool HttpReporter::Report() {
  try {
    int version = 11;  // HTTP 1.1

    // 创建解析器和 TCP 流对象
    tcp::resolver resolver(ioc);
    beast::tcp_stream stream(ioc);

    // 解析主机名和端口
    boost::asio::ip::tcp::resolver::query query(host_, port_);
    auto const results = resolver.resolve(query);

    // 连接到解析得到的 IP 地址
    stream.connect(results);

    // 构造 HTTP POST 请求
    http::request<http::string_body> req{http::verb::post, topic_, version};
    req.set(http::field::host, host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/json");
    req.body() = body_;
    req.prepare_payload();

    // 发送 HTTP 请求
    http::write(stream, req);

    // 用于读取响应的缓冲区
    beast::flat_buffer buffer;

    // 用于存放响应的对象
    http::response<http::dynamic_body> res;

    // 接收 HTTP 响应
    http::read(stream, buffer, res);

    // 优雅关闭 socket
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    // 有些情况下会出现 not_connected 错误，可以忽略
    if (ec && ec != beast::errc::not_connected) throw beast::system_error{ec};

    // 检查响应状态码
    auto res_status = res.result();
    if (res_status != http::status::ok) {
      SPDLOG_LOGGER_ERROR(logger_,
                          "report error " + std::to_string(res.result_int()));
      return false;
    }

    // 提取响应体内容并写入日志
    std::string res_body = boost::beast::buffers_to_string(res.body().data());
    SPDLOG_LOGGER_TRACE(logger_, "report success" + res_body);
    // 到这里说明连接已正常关闭
  } catch (std::exception const& e) {
    // 捕获并记录异常信息
    SPDLOG_LOGGER_ERROR(logger_, "report error " + std::string(e.what()));
    return false;
  }
  return true;
}

}  // namespace buried