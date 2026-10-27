#include <map>
#include <set>
#include <string>

#include "gtest/gtest.h"

#include "adr/adr.h"
#include "adr/cache.h"
#include "adr/guess_context.h"
#include "adr/typeahead.h"

namespace {

std::map<std::string, std::set<std::string>> house_numbers_by_street(
    adr::typeahead const& t) {
  auto numbers = std::map<std::string, std::set<std::string>>{};
  for (auto i = 0U; i != t.house_numbers_.size(); ++i) {
    auto const street = adr::street_idx_t{i};
    auto& street_numbers =
        numbers[std::string{t.strings_[t.street_names_[street][0]].view()}];
    for (auto const house_number : t.house_numbers_[street]) {
      street_numbers.emplace(t.strings_[house_number].view());
    }
  }
  return numbers;
}

}  // namespace

// Bordeaux, rue Brémontier: node 2485419095 has only addr:housenumber=36, its
// street is the name of associatedStreet relation 3250120.
TEST(adr, associated_street_house_numbers) {
  adr::extract("test/associated_street.osm", "adr_associated_street", "/tmp");
  auto const t = adr::read("adr_associated_street/t.bin");

  auto const numbers = house_numbers_by_street(*t);

  // A member node and a member building get the relation's street.
  EXPECT_EQ((std::set<std::string>{"36", "40"}), numbers.at("Rue Brémontier"));

  // A member's own addr:street wins over the relation.
  EXPECT_EQ((std::set<std::string>{"5"}), numbers.at("Rue Saint-Genès"));

  // A house number in no relation and with no street stays out.
  for (auto const& [street, street_numbers] : numbers) {
    EXPECT_FALSE(street_numbers.contains("99")) << street;
  }
}

TEST(adr, associated_street_suggestion) {
  adr::extract("test/associated_street.osm", "adr_associated_street", "/tmp");
  auto const t = adr::read("adr_associated_street/t.bin");

  auto cache = adr::cache{t->strings_.size(), 100U};
  auto ctx = adr::guess_context{cache};
  ctx.resize(*t);

  auto const langs =
      adr::basic_string<adr::language_idx_t>{{adr::kDefaultLang}};
  adr::get_suggestions<false>(*t, "36 rue bremontier", 10U, langs, ctx,
                              std::nullopt, 1.0F);

  ASSERT_FALSE(ctx.suggestions_.empty());
  auto const& s = ctx.suggestions_[0];
  EXPECT_EQ("Rue Brémontier", t->strings_[s.str_].view());
  ASSERT_TRUE(std::holds_alternative<adr::address>(s.location_));
  auto const addr = std::get<adr::address>(s.location_);
  ASSERT_NE(adr::address::kNoHouseNumber, addr.house_number_);
  EXPECT_EQ(
      "36",
      t->strings_[t->house_numbers_[addr.street_][addr.house_number_]].view());
}
