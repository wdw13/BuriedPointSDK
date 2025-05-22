#pragma once
#include <memory>
#include <string>

namespace buried {

// 加解密算法基类，定义通用接口
class Crypt {
 public:
  virtual ~Crypt() = default;

  // 加密字符串（纯虚函数，需子类实现）
  virtual std::string Encrypt(const std::string& input) = 0;

  // 解密字符串（纯虚函数，需子类实现）
  virtual std::string Decrypt(const std::string& input) = 0;

  // 加密任意数据（纯虚函数，需子类实现）
  virtual std::string Encrypt(const void* input, size_t input_size) = 0;

  // 解密任意数据（纯虚函数，需子类实现）
  virtual std::string Decrypt(const void* input, size_t input_size) = 0;
};

// 前置声明，AES 加解密实现类（具体实现见 .cc 文件）
class AESImpl;

// 非线程安全的 AES 加解密类，继承自 Crypt
class AESCrypt : public Crypt {
 public:
  // 通过 PBKDF2-HMAC-SHA256 从密码和盐生成 32 字节 AES 密钥
  static std::string GetKey(const std::string& salt,
                            const std::string password);

  // 构造函数，传入密钥字符串
  explicit AESCrypt(const std::string& key);

  // 析构函数
  ~AESCrypt();

  // 禁止拷贝构造
  AESCrypt(const AESCrypt& other) = delete;

  // 禁止拷贝赋值
  AESCrypt& operator=(const AESCrypt& other) = delete;

  // 加密字符串，重载基类接口
  std::string Encrypt(const std::string& input) override;

  // 解密字符串，重载基类接口
  std::string Decrypt(const std::string& input) override;

  // 加密任意数据，重载基类接口
  std::string Encrypt(const void* input, size_t input_size) override;

  // 解密任意数据，重载基类接口
  std::string Decrypt(const void* input, size_t input_size) override;

 private:
  // 持有 AES 实现类的智能指针
  std::unique_ptr<AESImpl> impl_;
};

}  // namespace buried