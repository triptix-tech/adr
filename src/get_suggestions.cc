#include "adr/adr.h"

#include "adr/typeahead.h"

namespace adr {

struct area {
  friend bool operator==(area const& a, area const& b) {
    return a.area_ == b.area_;
  }
  area_idx_t area_;
  coordinates coordinates_;
  float cos_sim_{0.0};
};

void get_suggestions(typeahead const& t,
                     geo::latlng const& coord,
                     std::string_view in,
                     unsigned n_suggestions,
                     guess_context& ctx) {
  t.guess(in, ctx);

  auto input_hashes = cista::raw::ankerl_set<cista::hash_t>{};
  utl::for_each_token(utl::cstr{in}, ' ', [&](utl::cstr s) {
    std::cout << s.view() << " -> hash=" << cista::hash(s.view()) << "\n";
    input_hashes.emplace(cista::hash(s.view()));
  });

  for (auto const m : ctx.street_matches_) {
    std::cout << "STREET " << t.strings_[t.street_names_[m.idx_]].view()
              << ", similarity=" << m.cos_sim_ << ", matches="
              << static_cast<unsigned>(ctx.street_match_counts_[m.idx_])
              << "\n";

    for (auto const& [house_number, house_coordinates] :
         utl::zip(t.house_numbers_[m.idx_], t.house_coordinates_[m.idx_])) {
      if (input_hashes.contains(cista::hash(t.strings_[house_number].view()))) {
        std::cout << "  HOUSE NUMBER " << t.strings_[house_number].view()
                  << ": "
                  << osmium::Location{house_coordinates.lat_,
                                      house_coordinates.lng_}
                  << "\n";
      }
    }

    auto areas = std::vector<area>{};
    for (auto const [coordinates, area_sets] :
         utl::zip(t.street_coordinates_[m.idx_], t.street_areas_[m.idx_])) {
      for (auto const a : t.area_sets_[area_sets]) {
        utl::insert_sorted(areas, area{a, coordinates}, [](auto&& a, auto&& b) {
          return a.area_ < b.area_;
        });
      }
    }

    for (auto& [area, coordiantes, cos_sim] : areas) {
      auto const area_name_str_idx = t.area_names_[area];
      auto const match_count = ctx.area_match_counts_[area];
      cos_sim = static_cast<float>(match_count) /
                (ctx.sqrt_len_vec_in_ * t.match_sqrts_[area_name_str_idx]);
    }

    std::sort(begin(areas), end(areas),
              [](auto&& a, auto&& b) { return a.cos_sim_ > b.cos_sim_; });

    for (auto const& area : areas) {
      if (t.area_admin_level_[area.area_] == kPostalCodeAdminLevel &&
          input_hashes.contains(
              cista::hash(t.strings_[t.area_names_[area.area_]].view()))) {
        std::cout << "  POSTCODE "
                  << t.strings_[t.area_names_[area.area_]].view() << "\n";
        continue;
      }
      if (area.cos_sim_ < 0.1) {
        continue;
      }
      std::cout << "  AREA " << t.strings_[t.area_names_[area.area_]].view()
                << " [" << to_str(t.area_admin_level_[area.area_])
                << "]: " << area.cos_sim_ << "\n";
    }
  }
}

}  // namespace adr