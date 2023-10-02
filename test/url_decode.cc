#include "gtest/gtest.h"

#include "adr/url_decode.h"

TEST(adr, url_decode) {
  std::string out;
  adr::url_decode("Test%20%23%C3%A4%C3%B6%C3%BC77%2610", out);
  EXPECT_EQ("Test #äöü77&10", out);
}