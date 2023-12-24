#include "gtest/gtest.h"

#include "adr/parse_get_parameters.h"

TEST(adr, parse_get_parameters) {
  auto i = 0U;
  adr::parse_get_parameters("?help=yes&deppert=no&yo",
                            [&](std::string_view key, std::string_view value) {
                              switch (i++) {
                                case 0:
                                  EXPECT_EQ(key, "help");
                                  EXPECT_EQ(value, "yes");
                                  break;

                                case 1:
                                  EXPECT_EQ(key, "deppert");
                                  EXPECT_EQ(value, "no");
                                  break;

                                case 2:
                                  EXPECT_EQ(key, "yo");
                                  EXPECT_EQ(value, "");
                                  break;
                              }
                            });
}

TEST(adr, parse_latlng) {
  auto p = adr::parse_latlng("37.76999,-122.44696");
  ASSERT_TRUE(p.has_value());
  EXPECT_FLOAT_EQ(37.76999, p->lat_);
  EXPECT_FLOAT_EQ(-122.44696, p->lng_);

  p = adr::parse_latlng("37.76999,");
  ASSERT_FALSE(p);

  p = adr::parse_latlng("");
  ASSERT_FALSE(p);
}