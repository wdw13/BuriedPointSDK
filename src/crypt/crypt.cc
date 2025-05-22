#include "crypt/crypt.h"

#include <string>

#include "third_party/mbedtls/include/mbedtls/cipher.h"
#include "third_party/mbedtls/include/mbedtls/error.h"
#include "third_party/mbedtls/include/mbedtls/md.h"
#include "third_party/mbedtls/include/mbedtls/pkcs5.h"

namespace buried {

// 通过 PBKDF2-HMAC-SHA256 从密码和盐生成 32 字节 AES 密钥
std::string AESCrypt::GetKey(const std::string& salt,
                             const std::string password) {
  int32_t keylen = 32;         // 密钥长度 32 字节（256位）
  uint32_t iterations = 1000;  // 迭代次数
  unsigned char key[32] = {0}; // 输出密钥缓冲区
  mbedtls_md_context_t md_ctx;
  mbedtls_md_init(&md_ctx);    // 初始化消息摘要上下文
  const mbedtls_md_info_t* md_info =
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256); // 获取SHA256算法信息
  mbedtls_md_setup(&md_ctx, md_info, 1);            // 设置上下文
  mbedtls_md_starts(&md_ctx);                       // 启动消息摘要
  // 生成密钥
  int ret = mbedtls_pkcs5_pbkdf2_hmac(
      &md_ctx, reinterpret_cast<const unsigned char*>(password.data()),
      password.size(), reinterpret_cast<const unsigned char*>(salt.data()),
      salt.size(), iterations, keylen, key);
  mbedtls_md_free(&md_ctx); // 释放消息摘要上下文
  if (ret != 0) {
    return ""; // 失败返回空字符串
  }
  return std::string((char*)key, keylen); // 返回密钥
}

// AES 加解密实现类，封装 mbedtls 的加解密上下文
class AESImpl {
 public:
  // 构造函数，初始化加解密上下文
  explicit AESImpl(const std::string& key) { Init(key.data(), key.size()); }

  ~AESImpl() { UnInit(); } // 析构时释放资源

  AESImpl(const AESImpl& other) = delete;
  AESImpl& operator=(const AESImpl& other) = delete;

  // 初始化加解密上下文
  void Init(const char* key, size_t key_size);

  // 释放加解密上下文
  void UnInit();

  // 加密接口
  std::string Encrypt(const void* input, size_t input_size);

  // 解密接口
  std::string Decrypt(const void* input, size_t input_size);

 private:
  mbedtls_cipher_context_t encrypt_ctx_; // 加密上下文
  mbedtls_cipher_context_t decrypt_ctx_; // 解密上下文

  uint32_t encrypt_block_size_ = 0; // 加密块大小
  uint32_t decrypt_block_size_ = 0; // 解密块大小

  unsigned char iv_[16] = {0}; // 初始向量（全0，实际应用建议随机）
};

// 初始化加解密上下文
void AESImpl::Init(const char* key, size_t key_size) {
  // 初始化加密上下文
  mbedtls_cipher_init(&encrypt_ctx_);
  mbedtls_cipher_setup(
      &encrypt_ctx_, mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_CBC));
  mbedtls_cipher_set_padding_mode(&encrypt_ctx_, MBEDTLS_PADDING_PKCS7);
  mbedtls_cipher_setkey(&encrypt_ctx_,
                        reinterpret_cast<const unsigned char*>(key),
                        key_size * 8, MBEDTLS_ENCRYPT); // 设置密钥（位数）

  encrypt_block_size_ = mbedtls_cipher_get_block_size(&encrypt_ctx_);

  // 初始化解密上下文
  mbedtls_cipher_init(&decrypt_ctx_);
  mbedtls_cipher_setup(
      &decrypt_ctx_, mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_CBC));
  mbedtls_cipher_set_padding_mode(&decrypt_ctx_, MBEDTLS_PADDING_PKCS7);
  mbedtls_cipher_setkey(&decrypt_ctx_,
                        reinterpret_cast<const unsigned char*>(key),
                        key_size * 8, MBEDTLS_DECRYPT);

  decrypt_block_size_ = mbedtls_cipher_get_block_size(&decrypt_ctx_);
}

// 释放加解密上下文资源
void AESImpl::UnInit() {
  mbedtls_cipher_free(&encrypt_ctx_);
  mbedtls_cipher_free(&decrypt_ctx_);
}

// 加密实现
std::string AESImpl::Encrypt(const void* input, size_t input_size) {
  mbedtls_cipher_set_iv(&encrypt_ctx_, iv_, sizeof(iv_)); // 设置初始向量
  mbedtls_cipher_reset(&encrypt_ctx_);                    // 重置上下文

  std::string output(input_size + encrypt_block_size_, 0); // 预分配输出缓冲区
  size_t olen = 0;
  // 加密数据
  int ret = mbedtls_cipher_update(
      &encrypt_ctx_, reinterpret_cast<const unsigned char*>(input), input_size,
      reinterpret_cast<unsigned char*>(output.data()), &olen);
  if (ret != 0) {
    return ""; // 失败返回空
  }
  size_t olen2 = 0;
  // 结束加密，处理填充
  ret = mbedtls_cipher_finish(
      &encrypt_ctx_, reinterpret_cast<unsigned char*>(output.data()) + olen,
      &olen2);
  if (ret != 0) {
    return "";
  }
  output.resize(olen + olen2); // 调整输出长度
  return output;
}

// 解密实现
std::string AESImpl::Decrypt(const void* input, size_t input_size) {
  mbedtls_cipher_set_iv(&decrypt_ctx_, iv_, sizeof(iv_)); // 设置初始向量
  mbedtls_cipher_reset(&decrypt_ctx_);                    // 重置上下文

  std::string output(input_size + decrypt_block_size_, 0); // 预分配输出缓冲区
  size_t olen = 0;
  // 解密数据
  int ret = mbedtls_cipher_update(
      &decrypt_ctx_, reinterpret_cast<const unsigned char*>(input), input_size,
      reinterpret_cast<unsigned char*>(output.data()), &olen);
  if (ret != 0) {
    return "";
  }
  size_t olen2 = 0;
  // 结束解密，处理填充
  ret = mbedtls_cipher_finish(
      &decrypt_ctx_, reinterpret_cast<unsigned char*>(output.data()) + olen,
      &olen2);
  if (ret != 0) {
    return "";
  }
  output.resize(olen + olen2); // 调整输出长度
  return output;
}

// AESCrypt 构造与析构，持有 AESImpl 实例
AESCrypt::AESCrypt(const std::string& key)
    : impl_(std::make_unique<AESImpl>(key)) {}

AESCrypt::~AESCrypt() {}

// 加密字符串接口
std::string AESCrypt::Encrypt(const std::string& input) {
  return impl_->Encrypt(input.data(), input.size());
}

// 解密字符串接口
std::string AESCrypt::Decrypt(const std::string& input) {
  return impl_->Decrypt(input.data(), input.size());
}

// 加密任意数据接口
std::string AESCrypt::Encrypt(const void* input, size_t input_size) {
  return impl_->Encrypt(input, input_size);
}

// 解密任意数据接口
std::string AESCrypt::Decrypt(const void* input, size_t input_size) {
  return impl_->Decrypt(input, input_size);
}

}  // namespace buried