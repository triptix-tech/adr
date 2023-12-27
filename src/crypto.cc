#include "adr/crypto.h"

#include "utl/read_file.h"
#include "utl/verify.h"

#include "openssl/aes.h"
#include "openssl/evp.h"

namespace adr {

crypto::crypto(std::vector<std::uint8_t> const& iv,
               std::vector<std::uint8_t> const& key)
    : iv_{iv}, key_{key}, ctx_{EVP_CIPHER_CTX_new()} {
  utl::verify(iv.size() == 16U,
              "crypto: IV should be 16 bytes (128 bit), size={}", iv.size());
  utl::verify(key.size() == 32U,
              "crypto: key should be 32 bytes (256 bit), size={}", key.size());
  utl::verify(ctx_ != nullptr, "decrypt_ctx: EVP_CIPHER_CTX_new -> nullptr");
}

crypto::~crypto() { EVP_CIPHER_CTX_free(ctx_); }

std::vector<std::uint8_t> crypto::read_base64_file(
    std::filesystem::path const& path, std::size_t const expected_size) {
  auto const content = utl::read_file(path.generic_string().c_str());
  utl::verify(content.has_value(),
              "read_base64_file: count not read file \"{}\"", path);

  auto out = std::vector<std::uint8_t>(3U * content->size() / 4U + 1U);
  auto const length = EVP_DecodeBlock(
      out.data(), reinterpret_cast<std::uint8_t const*>(content->data()),
      content->size());
  utl::verify(length + 1U == out.size(),
              "read_base64_file: expected {} bytes, got {}", out.size(),
              length);
  utl::verify(length >= expected_size,
              "read_base64_file: expected_size={}, length={}", expected_size,
              length);
  out.resize(expected_size);
  return out;
}

std::span<std::uint8_t const> crypto::crypt(std::span<std::uint8_t const> input,
                                            mode const m) {
  EVP_CIPHER_CTX_reset(ctx_);
  EVP_CipherInit(ctx_, EVP_aes_256_cbc(), key_.data(), iv_.data(), m);

  auto out_size = 0;
  buf_.resize(input.size() + AES_BLOCK_SIZE);
  EVP_CipherUpdate(ctx_, buf_.data(), &out_size, input.data(), input.size());

  auto out_size_last = 0;
  EVP_CipherFinal(ctx_, buf_.data() + out_size, &out_size_last);

  return {buf_.data(), static_cast<std::size_t>(out_size + out_size_last)};
}

}  // namespace adr