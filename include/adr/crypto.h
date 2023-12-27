#pragma once

#include <cinttypes>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "openssl/evp.h"

namespace adr {

struct crypto {
  // From the OpenSSL documentation:
  // The operation performed depends on the value of the enc parameter. It
  // should be set to 1 for encryption, 0 for decryption
  enum mode {
    kDecrypt = 0,
    kEncrypt = 1,
  };

  static std::vector<std::uint8_t> read_base64_file(
      std::filesystem::path const&, std::size_t);

  crypto(std::vector<std::uint8_t> const& iv,
         std::vector<std::uint8_t> const& key);
  crypto(crypto const&) = delete;
  crypto(crypto&&) = delete;
  crypto& operator=(crypto const&) = delete;
  crypto& operator=(crypto&&) = delete;
  ~crypto();

  std::span<std::uint8_t const> crypt(std::span<std::uint8_t const>, mode);

  std::vector<std::uint8_t> const &iv_, &key_;
  std::vector<std::uint8_t> buf_;
  EVP_CIPHER_CTX* ctx_{nullptr};
};

}  // namespace adr