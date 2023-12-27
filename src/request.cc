#include "adr/request.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "utl/verify.h"

namespace json = rapidjson;

namespace adr {

void decode(crypto& c, std::span<std::uint8_t const> encoded, http& req) {
  auto const decrypted = c.crypt(encoded, adr::crypto::kDecrypt);
  auto const input = std::string_view{
      reinterpret_cast<char const*>(decrypted.data()), decrypted.size()};

  auto doc = json::Document{};
  doc.Parse(input.data(), input.size());

  utl::verify(!doc.HasParseError(), "could not parse json \"{}\"", input);
  utl::verify(doc.IsObject(), "parsed JSON no Object (type={})", doc.GetType());

  if (doc.HasMember("path") && doc["path"].IsString()) {
    auto const& path = doc["path"];
    req.path_.resize(path.GetStringLength());
    std::strncpy(req.path_.data(), path.GetString(), path.GetStringLength());
  } else {
    req.path_.clear();
  }

  if (doc.HasMember("body") && doc["body"].IsString()) {
    auto const& body = doc["body"];
    req.body_.resize(body.GetStringLength());
    std::strncpy(req.body_.data(), body.GetString(), body.GetStringLength());
  } else {
    req.body_.clear();
  }
}

std::span<std::uint8_t const> encode(crypto& c, http const& res) {
  auto buf = json::StringBuffer{};
  auto w = json::Writer<json::StringBuffer>{buf};

  w.StartObject();
  if (!res.body_.empty()) {
    w.String("body");
    w.String(res.body_);
  }
  if (!res.path_.empty()) {
    w.String("path");
    w.String(res.path_);
  }
  w.EndObject();

  return c.crypt(
      std::span{reinterpret_cast<std::uint8_t const*>(buf.GetString()),
                buf.GetLength()},
      adr::crypto::kEncrypt);
}

}  // namespace adr