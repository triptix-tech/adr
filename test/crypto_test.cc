#include "gtest/gtest.h"

#include <string_view>

#include "fmt/core.h"
#include "fmt/ranges.h"

#include "adr/crypto.h"
#include "adr/request.h"

TEST(crypto, crypto) {
  auto const iv = std::vector<std::uint8_t>{1, 2,  3,  4,  5,  6,  7,  8,
                                            9, 10, 11, 12, 13, 14, 15, 16};
  auto const key = std::vector<std::uint8_t>{
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  auto c = adr::crypto{iv, key};

  auto const input =
      std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  fmt::print("input: {}, size={}\n", input, input.size());

  for (auto i = 0U; i != 10; ++i) {
    auto const encrypted = c.crypt(std::span{input}, adr::crypto::kEncrypt);
    auto const encrypted_copy =
        std::vector<std::uint8_t>{encrypted.begin(), encrypted.end()};
    fmt::print("encrypted: {}, size={}\n", encrypted_copy,
               encrypted_copy.size());

    auto const decrypted =
        c.crypt(std::span{encrypted_copy}, adr::crypto::kDecrypt);
    auto const decrypted_copy =
        std::vector<std::uint8_t>{decrypted.begin(), decrypted.end()};
    fmt::print("decrypted: {}, size={}\n", decrypted_copy,
               decrypted_copy.size());

    EXPECT_EQ(decrypted_copy, input);
  }
}

TEST(crypto, read_key_iv) {
  auto const key = adr::crypto::read_base64_file("test/key.txt", 32);
  auto const iv = adr::crypto::read_base64_file("test/iv.txt", 16);

  EXPECT_EQ(
      "1234567890abcdef",
      (std::string_view{reinterpret_cast<char const*>(iv.data()), iv.size()}));
  EXPECT_EQ("1234567890abcdef1234567890abcdef",
            (std::string_view{reinterpret_cast<char const*>(key.data()),
                              key.size()}));

  auto const input =
      std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  fmt::print("input: {}, size={}\n", input, input.size());

  auto c = adr::crypto{iv, key};

  for (auto i = 0U; i != 10; ++i) {
    auto const encrypted = c.crypt(std::span{input}, adr::crypto::kEncrypt);
    auto const encrypted_copy =
        std::vector<std::uint8_t>{encrypted.begin(), encrypted.end()};
    fmt::print("encrypted: {}, size={}\n", encrypted_copy,
               encrypted_copy.size());

    auto const decrypted =
        c.crypt(std::span{encrypted_copy}, adr::crypto::kDecrypt);
    auto const decrypted_copy =
        std::vector<std::uint8_t>{decrypted.begin(), decrypted.end()};
    fmt::print("decrypted: {}, size={}\n", decrypted_copy,
               decrypted_copy.size());

    EXPECT_EQ(decrypted_copy, input);
  }
}

TEST(crypto, request_response) {
  auto const key = adr::crypto::read_base64_file("test/key.txt", 32);
  auto const iv = adr::crypto::read_base64_file("test/iv.txt", 16);
  auto c = adr::crypto{iv, key};

  auto const h = adr::http{.path_ = "abc", .body_ = "def"};
  auto const encoded = adr::encode(c, h);
  auto const encoded_copy =
      std::vector<std::uint8_t>{encoded.begin(), encoded.end()};

  auto decoded = adr::http{};
  adr::decode(c, encoded_copy, decoded);

  EXPECT_EQ(h, decoded);
}