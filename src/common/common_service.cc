#include "common/common_service.h"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <random>
#include "buried_config.h"

namespace buried {

// CommonService 构造函数，调用 Init() 进行初始化
CommonService::CommonService() { Init(); }

// 向 Windows 注册表写入键值对
static void WriteRegister(const std::string& key, const std::string& value) {
  HKEY h_key;
  // 创建或打开注册表键
  LONG ret = ::RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Buried", 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                               &h_key, NULL);
  if (ret != ERROR_SUCCESS) {
    return;
  }
  // 设置键值
  ret = ::RegSetValueExA(h_key, key.c_str(), 0, REG_SZ,
                         reinterpret_cast<const BYTE*>(value.c_str()),
                         value.size());
  if (ret != ERROR_SUCCESS) {
    return;
  }
  ::RegCloseKey(h_key);
}

// 从 Windows 注册表读取指定键的值
static std::string ReadRegister(const std::string& key) {
  HKEY h_key;
  // 打开注册表键
  LONG ret = ::RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Buried", 0,
                             KEY_ALL_ACCESS, &h_key);
  if (ret != ERROR_SUCCESS) {
    return "";
  }
  // 读取键值
  char buf[1024] = {0};
  DWORD buf_size = sizeof(buf);
  ret = ::RegQueryValueExA(h_key, key.c_str(), NULL, NULL,
                           reinterpret_cast<BYTE*>(buf), &buf_size);
  if (ret != ERROR_SUCCESS) {
    return "";
  }
  ::RegCloseKey(h_key);
  return buf;
}

// 获取设备 ID，优先从注册表读取，没有则生成新的并保存
static std::string GetDeviceId() {
  static constexpr auto kDeviceIdKey = "device_id";
  static std::string device_id = ReadRegister(kDeviceIdKey);
  if (device_id.empty()) {
    device_id = CommonService::GetRandomId();
    WriteRegister(kDeviceIdKey, device_id);
  }
  return device_id;
}

// 获取生命周期 ID，每次运行程序生成新的
static std::string GetLifeCycleId() {
  static std::string life_cycle_id = CommonService::GetRandomId();
  return life_cycle_id;
}

// 获取 Windows 系统版本号
static std::string GetSystemVersion() {
  OSVERSIONINFOEXA os_version_info;
  ZeroMemory(&os_version_info, sizeof(OSVERSIONINFOEXA));
  os_version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
  ::GetVersionExA(reinterpret_cast<OSVERSIONINFOA*>(&os_version_info));
  // 拼接主版本号.次版本号.构建号
  std::string system_version =
      std::to_string(os_version_info.dwMajorVersion) + "." +
      std::to_string(os_version_info.dwMinorVersion) + "." +
      std::to_string(os_version_info.dwBuildNumber);
  return system_version;
}

// 获取计算机名
static std::string GetDeviceName() {
  char buf[1024] = {0};
  DWORD buf_size = sizeof(buf);
  ::GetComputerNameA(buf, &buf_size);
  return buf;
}

// 获取当前进程的创建时间
std::string CommonService::GetProcessTime() {
  // 获取当前进程 ID
  DWORD pid = ::GetCurrentProcessId();
  // 打开进程句柄
  HANDLE h_process =
      ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
  if (h_process == NULL) {
    return "";
  }
  // 获取进程时间信息
  FILETIME create_time, exit_time, kernel_time, user_time;
  BOOL ret = ::GetProcessTimes(h_process, &create_time, &exit_time,
                               &kernel_time, &user_time);
  ::CloseHandle(h_process);
  if (ret == 0) {
    return "";
  }

  // 转换为本地时间
  FILETIME create_local_time;
  ::FileTimeToLocalFileTime(&create_time, &create_local_time);

  // 转换为系统时间结构
  SYSTEMTIME create_sys_time;
  ::FileTimeToSystemTime(&create_local_time, &create_sys_time);
  
  // 格式化时间字符串: YYYY-MM-DD HH:mm:ss.fff
  char buf[128] = {0};
  sprintf_s(buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d", 
            create_sys_time.wYear, create_sys_time.wMonth, create_sys_time.wDay,
            create_sys_time.wHour, create_sys_time.wMinute, create_sys_time.wSecond,
            create_sys_time.wMilliseconds);
  return buf;
}

// 获取当前系统时间
std::string CommonService::GetNowDate() {
  auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  return std::ctime(&t);
}

// 生成 32 位随机字符串ID
std::string CommonService::GetRandomId() {
  static constexpr size_t len = 32;  // ID 长度
  // 字符集包含数字、大小写字母
  static constexpr auto chars =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  // 初始化随机数生成器
  static std::mt19937_64 rng{std::random_device{}()};
  static std::uniform_int_distribution<size_t> dist{0, 60};
  
  std::string result;
  result.reserve(len);
  // 生成随机字符串
  std::generate_n(std::back_inserter(result), len,
                  [&]() { return chars[dist(rng)]; });
  return result;
}

// 初始化 CommonService 的各项属性
void CommonService::Init() {
  system_version = GetSystemVersion();  // 系统版本
  device_name = GetDeviceName();       // 设备名称
  device_id = GetDeviceId();           // 设备ID
  buried_version = PROJECT_VER;        // SDK版本
  lifecycle_id = GetLifeCycleId();     // 生命周期ID
}

}  // namespace buried