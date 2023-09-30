#include "tg.h"

#include <iostream>

#include "utl/read_file.h"

constexpr auto const geojson =
    R"({"type" : "FeatureCollection", "features" : [{"type": "Feature", "geometry": {"type":"MultiPolygon","coordinates":[[[[-180,-9.5851607],[-179.9470917,-9.512575],[-179.92575,-9.4114167],[-179.9437528,-9.3236194],[-180,-9.2281701],[-180,-9.5851607]]],[[[179.260425,-9.1226645],[179.7378083,-9.6009528],[179.8051,-9.6298861],[179.873275,-9.6370889],[179.9416083,-9.6215861],[180,-9.5851607],[180,-9.2281701],[179.6681333,-8.7743389],[179.260425,-9.1226645]]]]}, "properties": {"osm_id": -3766670, "boundary": "administrative", "admin_level": 8, "parents": "-2177266", "name": "Nukulaelae", "local_name": "Nukulaelae", "name_en": null}}]})";

int main(int ac, char** av) {
  if (ac != 4) {
    std::cout << "usage: " << (ac >= 1 ? av[0] : "contains")
              << " <file.geojson> <lat> <lng>\n";
    return 1;
  }

  auto const f = utl::read_file(av[1]);
  if (!f.has_value()) {
    std::cout << "could not read file " << av[1] << "\n";
  }

  auto const geom = tg_parse_geojson(f.value().data());
  if (tg_geom_error(geom)) {
    std::cout << "invalid geometry: " << tg_geom_error(geom) << "\n";
    return 1;
  }

  auto const lat = std::stof(av[2]);
  auto const lng = std::stof(av[3]);
  auto const point = tg_geom_new_point(tg_point{lng, lat});
  auto const result = tg_geom_within(point, geom);

  std::cout << (result ? "true" : "false") << "\n";

  tg_geom_free(point);
  tg_geom_free(geom);

  return 0;
}