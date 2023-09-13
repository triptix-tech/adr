#include "gtest/gtest.h"

#include "utl/to_vec.h"

#include "fmt/core.h"

#include "adr/adr.h"
#include "adr/ngram.h"
#include "adr/normalize.h"
#include "adr/score.h"
#include "adr/sift4.h"

TEST(adr, simple) {
  adr::extract("test/Darmstadt.osm.pbf", "adr_darmstadt.cista", "/tmp");
}

TEST(adr, trigram_no_overflow) {
  auto const t = adr::compress_trigram("az4");
  EXPECT_EQ("az4", adr::decompress_trigram(t));
}

TEST(adr, trigram_overflow) {
  auto const t = adr::compress_trigram("az6");
  EXPECT_EQ("az1", adr::decompress_trigram(t));
}

TEST(adr, for_each_trigram) {
  constexpr auto const in = std::string_view{"Landwehrstraße"};
  std::vector<std::string> v;
  auto s = std::string{};
  auto decomposed_buf = std::basic_string<utf8proc_int32_t>{};
  auto const normalized = adr::normalize(in, decomposed_buf);
  adr::for_each_trigram(normalized,
                        [&](std::string_view s) { v.emplace_back(s); });
  EXPECT_EQ((std::vector<std::string>{{"lan", "and", "ndw", "dwe", "weh", "ehr",
                                       "hrs", "rst", "str", "tra", "ras", "ass",
                                       "sse"}}),
            v);
}

TEST(adr, for_each_bigram) {
  constexpr auto const in = std::string_view{"Landwehrstraße"};
  std::vector<adr::ngram_t> v;
  auto s = std::string{};
  adr::for_each_bigram(adr::normalize(in), [&](std::string_view s) {
    v.emplace_back(adr::compress_bigram(s));
  });
  EXPECT_EQ(
      (std::vector<std::string>{{"la", "an", "nd", "dw", "we", "eh", "hr", "rs",
                                 "st", "tr", "ra", "as", "ss", "se"}}),
      utl::to_vec(v, [](adr::ngram_t const ngram) {
        return adr::decompress_bigram(ngram);
      }));
}

TEST(adr, phrase) {
  auto const phrases = adr::get_phrases(
      {"willy", "brandt", "platz", "abert", "ainstein", "illme"});
  auto const expected = std::vector<std::pair<std::string, std::string>>{
      {"willy", "10000000"},
      {"willy brandt", "11000000"},
      {"willy brandt platz", "11100000"},
      {"brandt", "01000000"},
      {"brandt platz", "01100000"},
      {"brandt platz abert", "01110000"},
      {"platz", "00100000"},
      {"platz abert", "00110000"},
      {"platz abert ainstein", "00111000"},
      {"abert", "00010000"},
      {"abert ainstein", "00011000"},
      {"abert ainstein illme", "00011100"},
      {"ainstein", "00001000"},
      {"ainstein illme", "00001100"},
      {"illme", "00000100"}};
  auto i = 0U;
  for (auto const& p : phrases) {
    EXPECT_EQ((std::pair{p.s_, adr::bit_mask_to_str(p.token_bits_)}),
              (expected[i]));
    ++i;
  }
}

TEST(adr, numeric_tokens) {
  EXPECT_EQ("01100000", adr::bit_mask_to_str(adr::get_numeric_tokens_mask(
                            {"abc", "98", "9a", "0aa"})));
}

TEST(adr, score_test) {
  auto lev_dist = std::vector<adr::edit_dist_t>{};
  auto sift4_dist = std::vector<adr::sift_offset>{};
  auto tmp = std::string{};
  //  EXPECT_EQ(
  //      1, adr::get_match_score("Spießstraße", "spessartrasse", lev_dist,
  //      tmp));
  //  EXPECT_EQ(1, adr::get_match_score("Spessartstraße", "spessartrasse",
  //  lev_dist,
  //                                    tmp));
  //  EXPECT_EQ(1, adr::get_match_score("64747", "647", lev_dist, tmp));
  //  EXPECT_EQ(1, adr::get_match_score("76437", "647", lev_dist, tmp));

  //  EXPECT_EQ(1, adr::get_match_score("Albert-Einstein-Straße", "abert
  //  einstein",
  //                                    lev_dist, tmp));
  //  EXPECT_EQ(1, adr::get_match_score("Rennsteig", "einstein", lev_dist,
  //  tmp));

  //  EXPECT_EQ(1, adr::get_match_score("Albert-Einstein-Straße",
  //                                    "abert einstein ilmenau", lev_dist,
  //                                    tmp));
  //  EXPECT_EQ(1, adr::get_match_score("Albert-Einstein-Straße", "abert
  //  einstein",
  //                                    lev_dist, tmp));

  //  EXPECT_EQ(1, adr::get_match_score("Almere", "ilme", lev_dist, tmp));

  //  EXPECT_EQ(1, adr::get_match_score("Landwehrstraße", "ladnwerhstrae",
  //  lev_dist,
  //                                    tmp));

  //  EXPECT_EQ(1, adr::get_match_score("Darmstadt", "dar", lev_dist, tmp));

  //  EXPECT_EQ(1,
  //            adr::get_match_score("Froschgraben", "frochgabe", lev_dist,
  //            tmp));

  auto ctx = adr::guess_context{};
  //  EXPECT_EQ(1, adr::get_match_score("Darmstadt", "damrstadt", lev_dist,
  //                                    ctx.normalize_buf_));
  //  EXPECT_EQ(1, adr::get_match_score("Darmstadt", "damrstadt", lev_dist,
  //  tmp));

  EXPECT_EQ(1,
            adr::get_match_score("10a", "5", sift4_dist, ctx.normalize_buf_));
}

TEST(adr, sift4) {
  auto offset_arr = std::vector<adr::sift_offset>{};
  std::cout << static_cast<int>(
                   adr::sift4("Froschgraben", "frochgabe", 4U, 10U, offset_arr))
            << "\n";
}
