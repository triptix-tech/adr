#include "gtest/gtest.h"

#include "adr/adr.h"
#include "adr/normalize.h"
#include "adr/trigram.h"

TEST(adr, simple) {
  adr::extract("test/Darmstadt.osm.pbf", "adr_darmstadt.cista");
}

TEST(adr, trigram_no_overflow) {
  auto const t = adr::compress_trigram("az5");
  EXPECT_EQ("az5", adr::decompress_trigram(t));
}

TEST(adr, trigram_overflow) {
  auto const t = adr::compress_trigram("az6");
  EXPECT_EQ("az0", adr::decompress_trigram(t));
}

TEST(adr, for_each_trigram) {
  constexpr auto const in = std::string_view{"Landwehrstraße"};
  std::vector<std::string> v;
  auto s = std::string{};
  adr::normalize(in, s);
  adr::for_each_trigram(s, [&](std::string_view s) { v.emplace_back(s); });
  EXPECT_EQ((std::vector<std::string>{{"lan", "and", "ndw", "dwe", "weh", "ehr",
                                       "hrs", "rst", "str", "tra", "ras", "ass",
                                       "sse"}}),
            v);
}