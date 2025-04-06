#include "adr/formatter.h"

#include <map>
#include <optional>
#include <variant>
#include <vector>

namespace adr {

struct formatting_info {
  std::optional<std::string> address_template_;
  std::optional<std::string> change_country_;
  std::optional<std::string> use_country_;
  std::optional<std::string> fallback_template_;
  std::optional<std::vector<std::string>> postformat_replace_;
  std::optional<std::vector<std::array<std::string, 2>>> replace_;
  std::optional<std::string> add_component_;
};
struct formatter::impl {
  impl() {}

  std::map<std::string, std::variant<std::string, formatting_info>>
      formatting_info_;
};

}  // namespace adr