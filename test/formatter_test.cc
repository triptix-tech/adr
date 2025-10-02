#include "gtest/gtest.h"

#include "adr/formatter.h"

TEST(adr, formatting) {
  auto const a =
      adr::formatter::address{.house_number_ = "17",
                              .road_ = "Rue du Médecin-Colonel Calbairac",
                              .neighbourhood_ = "Lafourguette",
                              .suburb_ = "Toulouse Ouest",
                              .postcode_ = "31000",
                              .city_ = "Toulouse",
                              .county_ = "Toulouse",
                              .state_ = "Midi-Pyrénées",
                              .country_ = "France",
                              .country_code_ = "FR"};
  auto f = adr::formatter{};
  auto const formatted = f.format(a);
  EXPECT_EQ("17 Rue du Médecin-Colonel Calbairac, 31000 Toulouse, France",
            formatted);

  auto const b = adr::formatter::address{
      .house_number_ = "", .road_ = "Test", .country_code_ = "US"};
  auto const formatted1 = f.format(b);
  EXPECT_EQ("Test", formatted1);
}
