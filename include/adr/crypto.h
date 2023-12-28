#pragma once

#include <cinttypes>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "openssl/evp.h"
#include "openssl/rand.h"

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
  static std::vector<std::uint8_t> decode_base64(std::string_view, std::size_t);
  static std::string encode_base64(std::span<std::uint8_t const>);
  static std::vector<std::uint8_t> generate_random_bytes(std::size_t n);

  crypto(std::vector<std::uint8_t> const& key);
  crypto(crypto const&) = delete;
  crypto(crypto&&) = delete;
  crypto& operator=(crypto const&) = delete;
  crypto& operator=(crypto&&) = delete;
  ~crypto();

  std::span<std::uint8_t const> crypt(std::span<std::uint8_t const> iv,
                                      std::span<std::uint8_t const> input,
                                      mode);

  std::vector<std::uint8_t> const& key_;
  std::vector<std::uint8_t> buf_;
  EVP_CIPHER_CTX* ctx_{nullptr};
};

}  // namespace adr