#include "adr/formatter.h"

#include <map>
#include <optional>
#include <ranges>
#include <variant>
#include <vector>

#include "boost/algorithm/string/trim.hpp"

#include "rfl.hpp"
#include "rfl/yaml.hpp"

#include "mustache.hpp"

#include "utl/parser/cstr.h"
#include "utl/verify.h"

#include "address_formatting_res.h"

using namespace std::string_view_literals;

namespace adr {

template <rfl::internal::StringLiteral Name, size_t I = 0, char... Chars>
consteval auto drop_last() {
  if constexpr (I == Name.arr_.size() - 2) {
    return rfl::internal::StringLiteral<sizeof...(Chars) + 1>(Chars...);
  } else {
    return drop_last<Name, I + 1, Chars..., Name.arr_[I]>();
  }
}

struct drop_trailing {
public:
  template <typename StructType>
  static auto process(auto&& named_tuple) {
    const auto handle_one = []<typename FieldType>(FieldType&& f) {
      if constexpr (FieldType::name() != "xml_content" &&
                    !rfl::internal::is_rename_v<typename FieldType::Type>) {
        return handle_one_field(std::move(f));
      } else {
        return std::move(f);
      }
    };
    return named_tuple.transform(handle_one);
  }

private:
  template <typename FieldType>
  static auto handle_one_field(FieldType&& _f) {
    using NewFieldType =
        rfl::Field<drop_last<FieldType::name_>(), typename FieldType::Type>;
    return NewFieldType(_f.value());
  }
};

struct formatting_info {
  std::optional<std::string> address_template_;
  std::optional<std::string> change_country_;
  std::optional<std::string> use_country_;
  std::optional<std::string> fallback_template_;
  std::optional<std::vector<std::array<std::string, 2>>> postformat_replace_;
  std::optional<std::vector<std::array<std::string, 2>>> replace_;
  std::optional<std::string> add_component_;
};

using config_t =
    std::map<std::string, std::variant<std::string, formatting_info>>;

std::string to_str(address_formatting_res::resource const s) {
  utl::verify(s.ptr_ != nullptr, "address formatting: resource not found");
  return std::string{static_cast<char const*>(s.ptr_), s.size_};
}

struct formatter::impl {
  impl()
      : formatting_info_{
            rfl::yaml::read<config_t, drop_trailing, rfl::DefaultIfMissing>(
                to_str(address_formatting_res::get_resource(
                    "countries/worldwide.yaml")))
                .value()} {}

  config_t formatting_info_;
};

std::string formatter::format(adr::suggestion const& s) { return ""; }

std::string formatter::format(address const& x) {
  auto const it = impl_->formatting_info_.find(x.country_code_);
  if (it == end(impl_->formatting_info_) ||
      !std::holds_alternative<formatting_info>(it->second) ||
      !std::get<formatting_info>(it->second).address_template_.has_value()) {
    return "";
  }

  using namespace kainjow::mustache;
  auto const& address_template =
      *std::get<formatting_info>(it->second).address_template_;

  std::cout << "---\n";
  std::cout << address_template << "\n";
  std::cout << "---\n";

  auto d = data{};
  d["house_number"] = x.house_number_;
  d["road"] = x.road_;
  d["neighbourhood"] = x.neighbourhood_;
  d["suburb"] = x.suburb_;
  d["postcode"] = x.postcode_;
  d["city"] = x.city_;
  d["county"] = x.county_;
  d["state"] = x.state_;
  d["country"] = x.country_;
  d["country_code"] = x.country_code_;
  d["first"] = data{lambda{[&](std::string const& x) {
    namespace sv = std::ranges::views;
    auto const rendered = mustache{x}.render(d);
    auto non_empty =
        std::ranges::split_view{std::string_view{rendered}, "||"sv}  //
        |
        sv::transform([](auto&& str) {
          return utl::cstr{&*str.begin(),
                           static_cast<std::size_t>(std::ranges::distance(str))}
              .trim();
        })  //
        | sv::filter([](auto&& s) { return !s.empty(); });
    for (auto&& sv : non_empty) {
      return sv.to_str();
    }
    return std::string{""};
  }}};
  return mustache{address_template}.render(d);
}

formatter::formatter() : impl_{std::make_unique<impl>()} {}

formatter::~formatter() = default;

}  // namespace adr