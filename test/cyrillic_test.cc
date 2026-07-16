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
