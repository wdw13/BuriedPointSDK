#include "report/buried_report.h"

#include <chrono>
#include <filesystem>

#include "boost/asio/deadline_timer.hpp"
#include "boost/asio/io_service.hpp"
#include "context/context.h"
#include "crypt/crypt.h"
#include "database/database.h"
#include "report/http_report.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace buried {

// 数据库文件名常量
static const char kDbName[] = "buried.db";

// 具体实现类，负责埋点数据的加密、存储、定时上报等逻辑
class BuriedReportImpl {
 public:
  // 构造函数，初始化日志、服务信息、工作目录等
  BuriedReportImpl(std::shared_ptr<spdlog::logger> logger,
                   CommonService common_service, std::string work_path)
      : logger_(std::move(logger)),
        common_service_(std::move(common_service)),
        work_dir_(std::move(work_path)) {
    // 如果没有传入 logger，则创建一个默认的彩色控制台 logger
    if (logger_ == nullptr) {
      logger_ = spdlog::stdout_color_mt("buried");
    }
    // 生成 AES 密钥并初始化加解密器
    std::string key = AESCrypt::GetKey("buried_salt", "buried_password");
    crypt_ = std::make_unique<AESCrypt>(key);
    SPDLOG_LOGGER_INFO(logger_, "BuriedReportImpl init success");
    // 在上报 strand 上异步初始化数据库
    Context::GetGlobalContext().GetReportStrand().post([this]() { Init_(); });
  }

  ~BuriedReportImpl() = default;

  // 启动定时上报
  void Start();

  // 插入一条埋点数据
  void InsertData(const BuriedData& data);

 private:
  // 初始化数据库
  void Init_();

  // 上报缓存中的数据
  void ReportCache_();

  // 进入下一次上报周期
  void NextCycle_();

  // 将 BuriedData 转换为数据库存储格式
  BuriedDb::Data MakeDbData_(const BuriedData& data);

  // 生成上报用的 JSON 字符串
  std::string GenReportData_(const std::vector<BuriedDb::Data>& datas);

  // 执行 HTTP 上报
  bool ReportData_(const std::string& data);

 private:
  std::shared_ptr<spdlog::logger> logger_; // 日志器
  std::string work_dir_;                   // 工作目录
  std::unique_ptr<BuriedDb> db_;           // 数据库对象
  CommonService common_service_;           // 公共服务信息
  std::unique_ptr<buried::Crypt> crypt_;   // 加解密器

  std::unique_ptr<boost::asio::deadline_timer> timer_; // 定时器

  std::vector<BuriedDb::Data> data_caches_; // 缓存待上报的数据
};

// 数据库初始化，设置路径并创建数据库对象
void BuriedReportImpl::Init_() {
  std::filesystem::path db_path = work_dir_;
  SPDLOG_LOGGER_INFO(logger_, "BuriedReportImpl init db path: {}",
                     db_path.string());
  db_path /= kDbName;
  db_ = std::make_unique<BuriedDb>(db_path.string());
}

// 启动定时器，定时触发上报逻辑
void BuriedReportImpl::Start() {
  SPDLOG_LOGGER_INFO(logger_, "BuriedReportImpl start");

  // 创建定时器，5秒后触发
  timer_ = std::make_unique<boost::asio::deadline_timer>(
      Context::GetGlobalContext().GetMainContext(),
      boost::posix_time::seconds(5));

  // 异步等待，回调在上报 strand 上执行
  timer_->async_wait(Context::GetGlobalContext().GetReportStrand().wrap(
      [this](const boost::system::error_code& ec) {
        if (ec) {
          logger_->error("BuriedReportImpl::Start error: {}", ec.message());
          return;
        }
        ReportCache_();
      }));
}

// 插入数据，实际操作在上报 strand 上异步执行
void BuriedReportImpl::InsertData(const BuriedData& data) {
  Context::GetGlobalContext().GetReportStrand().post(
      [this, data]() { db_->InsertData(MakeDbData_(data)); });
}

// 执行 HTTP 上报，返回是否成功
bool BuriedReportImpl::ReportData_(const std::string& data) {
  HttpReporter reporter(logger_);
  return reporter.Host(common_service_.host)
      .Topic(common_service_.topic)
      .Port(common_service_.port)
      .Body(data)
      .Report();
}

// 上报缓存中的数据，如果上报成功则从数据库删除
void BuriedReportImpl::ReportCache_() {
  SPDLOG_LOGGER_INFO(logger_, "BuriedReportImpl report cache");
  // 如果缓存为空，从数据库查询最多10条数据
  if (data_caches_.empty()) {
    data_caches_ = db_->QueryData(10);
  }

  // 如果有数据，生成上报内容并尝试上报
  if (!data_caches_.empty()) {
    std::string report_data = GenReportData_(data_caches_);
    if (ReportData_(report_data)) {
      db_->DeleteDatas(data_caches_); // 上报成功则删除
      data_caches_.clear();
    }
  }

  // 进入下一次上报周期
  NextCycle_();
}

// 将数据库数据解密并组装为 JSON 数组字符串
std::string BuriedReportImpl::GenReportData_(
    const std::vector<BuriedDb::Data>& datas) {
  nlohmann::json json_datas;
  for (const auto& data : datas) {
    std::string content =
        crypt_->Decrypt(data.content.data(), data.content.size());
    SPDLOG_LOGGER_INFO(logger_, "BuriedReportImpl report data content size: {}",
                       data.content.size());
    json_datas.push_back(content);
  }
  std::string ret = json_datas.dump();
  return ret;
}

// 将 BuriedData 转换为数据库存储格式，并加密内容
BuriedDb::Data BuriedReportImpl::MakeDbData_(const BuriedData& data) {
  BuriedDb::Data db_data;
  db_data.id = -1;
  db_data.priority = data.priority;
  db_data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  nlohmann::json json_data;
  json_data["title"] = data.title;
  json_data["data"] = data.data;
  json_data["user_id"] = common_service_.user_id;
  json_data["app_version"] = common_service_.app_version;
  json_data["app_name"] = common_service_.app_name;
  json_data["custom_data"] = common_service_.custom_data;
  json_data["system_version"] = common_service_.system_version;
  json_data["device_name"] = common_service_.device_name;
  json_data["device_id"] = common_service_.device_id;
  json_data["buried_version"] = common_service_.buried_version;
  json_data["lifecycle_id"] = common_service_.lifecycle_id;
  json_data["priority"] = data.priority;
  json_data["timestamp"] = CommonService::GetNowDate();
  json_data["process_time"] = CommonService::GetProcessTime();
  json_data["report_id"] = CommonService::GetRandomId();
  // 加密 JSON 字符串
  std::string report_data = crypt_->Encrypt(json_data.dump());
  db_data.content = std::vector<char>(report_data.begin(), report_data.end());
  SPDLOG_LOGGER_INFO(logger_, "BuriedReportImpl insert data size: {}",
                     db_data.content.size());

  return db_data;
}

// 进入下一次上报周期，定时器顺延5秒
void BuriedReportImpl::NextCycle_() {
  SPDLOG_LOGGER_INFO(logger_, "BuriedReportImpl next cycle");
  timer_->expires_at(timer_->expires_at() + boost::posix_time::seconds(5));
  timer_->async_wait([this](const boost::system::error_code& ec) {
    if (ec) {
      logger_->error("BuriedReportImpl::NextCycle_ error: {}", ec.message());
      return;
    }
    // 下一次上报逻辑仍然在上报 strand 上执行
    Context::GetGlobalContext().GetReportStrand().post(
        [this]() { ReportCache_(); });
  });
}

// =================== BuriedReport 外部接口实现 ===================

// 构造函数，创建实现对象
BuriedReport::BuriedReport(std::shared_ptr<spdlog::logger> logger,
                           CommonService common_service, std::string work_path)
    : impl_(std::make_unique<BuriedReportImpl>(
          std::move(logger), std::move(common_service), std::move(work_path))) {
}

// 启动上报
void BuriedReport::Start() { impl_->Start(); }

// 插入埋点数据
void BuriedReport::InsertData(const BuriedData& data) {
  impl_->InsertData(data);
}

// 析构函数
BuriedReport::~BuriedReport() {}

}  // namespace buried