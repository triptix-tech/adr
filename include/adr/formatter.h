#pragma once

#include <memory>
#include <string>

namespace adr {

struct suggestion;

struct address {
  std::string house_number_;
  std::string road_;
  std::string neighbourhood_;
  std::string suburb_;
  std::string postcode_;
  std::string city_;
  std::string county_;
  std::string state_;
  std::string country_;
  std::string country_code_;
};

struct formatter {
  formatter();
  ~formatter();

  std::string format(suggestion const&);
  std::string format(address const&);

  struct impl;
  std::unique_ptr<impl> impl_;
};

}  // namespace adr