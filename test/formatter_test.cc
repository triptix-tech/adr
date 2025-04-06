#include "gtest/gtest.h"

#include "adr/formatter.h"

TEST(adr, formatting) {
  auto const a = adr::address{.house_number_ = "17",
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
  std::cout << f.format(a) << "\n";
}