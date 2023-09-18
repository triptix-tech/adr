#include "adr/json.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace json = rapidjson;

namespace adr {

/*
{
  "predictions":
    [
      {
        "description": "Paris, France",
        "matched_substrings": [{ "length": 5, "offset": 0 }],
        "place_id": "ChIJD7fiBh9u5kcRYJSMaMOCCwQ",
        "reference": "ChIJD7fiBh9u5kcRYJSMaMOCCwQ",
        "structured_formatting":
          {
            "main_text": "Paris",
            "main_text_matched_substrings": [{ "length": 5, "offset": 0 }],
            "secondary_text": "France",
          },
        "terms":
          [
            { "offset": 0, "value": "Paris" },
            { "offset": 7, "value": "France" },
          ],
        "types": ["locality", "political", "geocode"],
      },
      {
        "description": "Paris, TX, USA",
        "matched_substrings": [{ "length": 5, "offset": 0 }],
        "place_id": "ChIJmysnFgZYSoYRSfPTL2YJuck",
        "reference": "ChIJmysnFgZYSoYRSfPTL2YJuck",
        "structured_formatting":
          {
            "main_text": "Paris",
            "main_text_matched_substrings": [{ "length": 5, "offset": 0 }],
            "secondary_text": "TX, USA",
          },
        "terms":
          [
            { "offset": 0, "value": "Paris" },
            { "offset": 7, "value": "TX" },
            { "offset": 11, "value": "USA" },
          ],
        "types": ["locality", "political", "geocode"],
      },
      {
        "description": "Paris, TN, USA",
        "matched_substrings": [{ "length": 5, "offset": 0 }],
        "place_id": "ChIJ4zHP-Sije4gRBDEsVxunOWg",
        "reference": "ChIJ4zHP-Sije4gRBDEsVxunOWg",
        "structured_formatting":
          {
            "main_text": "Paris",
            "main_text_matched_substrings": [{ "length": 5, "offset": 0 }],
            "secondary_text": "TN, USA",
          },
        "terms":
          [
            { "offset": 0, "value": "Paris" },
            { "offset": 7, "value": "TN" },
            { "offset": 11, "value": "USA" },
          ],
        "types": ["locality", "political", "geocode"],
      },
      {
        "description": "Paris, Brant, ON, Canada",
        "matched_substrings": [{ "length": 5, "offset": 0 }],
        "place_id": "ChIJsamfQbVtLIgR-X18G75Hyi0",
        "reference": "ChIJsamfQbVtLIgR-X18G75Hyi0",
        "structured_formatting":
          {
            "main_text": "Paris",
            "main_text_matched_substrings": [{ "length": 5, "offset": 0 }],
            "secondary_text": "Brant, ON, Canada",
          },
        "terms":
          [
            { "offset": 0, "value": "Paris" },
            { "offset": 7, "value": "Brant" },
            { "offset": 14, "value": "ON" },
            { "offset": 18, "value": "Canada" },
          ],
        "types": ["neighborhood", "political", "geocode"],
      },
      {
        "description": "Paris, KY, USA",
        "matched_substrings": [{ "length": 5, "offset": 0 }],
        "place_id": "ChIJsU7_xMfKQ4gReI89RJn0-RQ",
        "reference": "ChIJsU7_xMfKQ4gReI89RJn0-RQ",
        "structured_formatting":
          {
            "main_text": "Paris",
            "main_text_matched_substrings": [{ "length": 5, "offset": 0 }],
            "secondary_text": "KY, USA",
          },
        "terms":
          [
            { "offset": 0, "value": "Paris" },
            { "offset": 7, "value": "KY" },
            { "offset": 11, "value": "USA" },
          ],
        "types": ["locality", "political", "geocode"],
      },
    ],
  "status": "OK",
}
*/

/*
 {
 "description": "Paris, France",
 "matched_substrings": [{ "length": 5, "offset": 0 }],
 "place_id": "ChIJD7fiBh9u5kcRYJSMaMOCCwQ",
 "reference": "ChIJD7fiBh9u5kcRYJSMaMOCCwQ",
 "structured_formatting":
   {
     "main_text": "Paris",
     "main_text_matched_substrings": [{ "length": 5, "offset": 0 }],
     "secondary_text": "France",
   },
 "terms":
   [
     { "offset": 0, "value": "Paris" },
     { "offset": 7, "value": "France" },
   ],
 "types": ["locality", "political", "geocode"],
}
 */

// ==
// G
// --
template <typename Writer>
void to_g_json(typeahead const& t, suggestion const& s, Writer& w) {
  w.StartObject();

  w.Key("description");
  w.String(s.description(t));

  w.Key("matched_substrings");
  w.StartArray();

  w.EndArray();

  w.EndObject();
}

template <typename Writer>
void to_g_json(typeahead const& t,
               std::vector<suggestion> const& suggestions,
               Writer& w) {
  w.StartObject();

  w.Key("predictions");
  w.StartArray();
  for (auto const& s : suggestions) {
    to_g_json(t, s, w);
  }
  w.EndArray();

  w.Key("Status");
  w.String("OK");

  w.EndObject();
}

// ===
// MB
// ---
template <typename Writer>
void to_mb_json(typeahead const& t, suggestion const& s, Writer& w) {}

template <typename Writer>
void to_mb_json(typeahead const& t,
                std::vector<suggestion> const& s,
                Writer& writer) {}

// =======
// PELIAS
// -------
template <typename Writer>
void to_pelias_json(typeahead const& t, suggestion const& s, Writer& w) {}

template <typename Writer>
void to_pelias_json(typeahead const& t,
                    std::vector<suggestion> const& s,
                    Writer& w) {}

// ====
// MUX
// ----
std::string to_json(typeahead const& t,
                    std::vector<suggestion> const& s,
                    output_format const format) {
  auto buf = json::StringBuffer{};
  auto writer = json::Writer<json::StringBuffer>{buf};
  switch (format) {
    case output_format::kGPlaces: to_g_json(t, s, writer); break;
    case output_format::kMB: to_mb_json(t, s, writer); break;
    case output_format::kPelias: to_pelias_json(t, s, writer); break;
  }
  return {buf.GetString(), buf.GetLength()};
}

}  // namespace adr