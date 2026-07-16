#include "gtest/gtest.h"

#include "utl/helpers/algorithm.h"

#include "adr/adr.h"
#include "adr/cache.h"
#include "adr/guess_context.h"
#include "adr/import_context.h"
#include "adr/ngram.h"
#include "adr/normalize.h"
#include "adr/typeahead.h"

using namespace std::string_view_literals;

// UTF-8 bytes >= 0x80 must not be sign-extended when packing bigrams:
// a sign-extending compress_char fills the high byte with 0xFF, collapsing
// all bigrams with the same first byte (e.g. every Cyrillic "\xD0 x" pair)
// onto a single value. The distinct-match count then fails the cosine
// similarity cutoff in typeahead::guess even for exact matches.
TEST(adr, compress_bigram_non_ascii) {
  EXPECT_NE(adr::compress_bigram("\xD0\xB8"sv) /* и */,
            adr::compress_bigram("\xD0\xBD"sv) /* н */);
  EXPECT_EQ("\xD0\xB8",
            adr::decompress_bigram(adr::compress_bigram("\xD0\xB8"sv)));
}

TEST(adr, guess_cyrillic_exact_match) {
  auto t = adr::typeahead{};
  auto ictx = adr::import_context{};
  auto const str_idx = t.get_or_create_string(ictx, "Индже войвода");
  t.get_or_create_string(ictx, "София");
  t.get_or_create_string(ictx, "Aschaffenburg");
  t.build_ngram_index();

  auto cache = adr::cache{t.strings_.size(), 100U};
  auto ctx = adr::guess_context{cache};
  ctx.resize(t);

  t.guess<false>(adr::normalize("Индже войвода"), ctx);

  EXPECT_TRUE(utl::any_of(ctx.string_matches_,
                          [&](auto const& m) { return m.idx_ == str_idx; }));
}

// Block address in a quarter without street names: way 39741651 (Бургас,
// "ж.к. П. Р. Славейков", addr:housenumber="бл. 26") has addr:place instead
// of addr:street. The quarter name is indexed like a street name; the query
// token "26" matches "бл. 26" via sub-phrase matching in get_match_score.
TEST(adr, addr_place_block_address) {
  adr::extract("test/SlaveikovBurgas.osm.pbf", "adr_slaveikov", "/tmp");
  auto const t = adr::read("adr_slaveikov/t.bin");

  auto cache = adr::cache{t->strings_.size(), 100U};
  auto ctx = adr::guess_context{cache};
  ctx.resize(*t);

  auto const langs =
      adr::basic_string<adr::language_idx_t>{{adr::kDefaultLang}};
  adr::get_suggestions<false>(*t, "Славейков 26", 10U, langs, ctx, std::nullopt,
                              1.0F);

  ASSERT_FALSE(ctx.suggestions_.empty());
  auto const& s = ctx.suggestions_[0];
  EXPECT_EQ("ж.к. П. Р. Славейков", t->strings_[s.str_].view());
  ASSERT_TRUE(std::holds_alternative<adr::address>(s.location_));
  auto const addr = std::get<adr::address>(s.location_);
  ASSERT_NE(adr::address::kNoHouseNumber, addr.house_number_);
  EXPECT_EQ(
      "бл. 26",
      t->strings_[t->house_numbers_[addr.street_][addr.house_number_]].view());
}

// End-to-end: OSM node 273813212, place=village, name="Индже войвода".
TEST(adr, extract_and_suggest_cyrillic) {
  adr::extract("test/IndzheVoyvoda.osm.pbf", "adr_indzhe", "/tmp");
  auto const t = adr::read("adr_indzhe/t.bin");

  auto cache = adr::cache{t->strings_.size(), 100U};
  auto ctx = adr::guess_context{cache};
  ctx.resize(*t);

  auto const langs =
      adr::basic_string<adr::language_idx_t>{{adr::kDefaultLang}};
  adr::get_suggestions<false>(*t, "Индже войвода", 10U, langs, ctx,
                              std::nullopt, 1.0F);

  ASSERT_FALSE(ctx.suggestions_.empty());
  EXPECT_EQ("Индже войвода", t->strings_[ctx.suggestions_[0].str_].view());
}
