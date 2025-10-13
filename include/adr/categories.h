// WARNING: This file is auto-generated. Do not edit manually.        

#pragma once

#include "cista/hash.h"
#include "osmium/osm/object.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace adr {

enum class amenity_category : std::uint16_t {
  kNone = 0,
  kRestaurant,
  kCafe,
  kFastFood,
  kBar,
  kPub,
  kIceCream,
  kBiergarten,
  kOutdoorSeating,
  kArtwork,
  kCommunityCentre,
  kLibrary,
  kMuseum,
  kTheatre,
  kCinema,
  kNightclub,
  kArtsCentre,
  kGallery,
  kInternetCafe,
  kCasino,
  kPublicBookcase,
  kAmusementArcade,
  kMemorial,
  kArchaeologicalSite,
  kCartoShrine,
  kMonument,
  kCastle,
  kPlaque,
  kStatue,
  kStone,
  kPalace,
  kFortress,
  kHistoricFort,
  kBust,
  kCityGate,
  kManor,
  kObelisk,
  kPlayground,
  kFitness,
  kGolfIcon,
  kSwimming,
  kMassage,
  kSauna,
  kPublicBath,
  kMiniatureGolf,
  kBeachResort,
  kFishing,
  kBowlingAlley,
  kDogPark,
  kLeisureDance,
  kLeisureGolfPin,
  kToilets,
  kRecycling,
  kWasteBasket,
  kWasteDisposal,
  kExcrementBags,
  kBench,
  kShelter,
  kDrinkingWater,
  kPicnicSite,
  kFountain,
  kCamping,
  kTable,
  kCaravan,
  kBbq,
  kShower,
  kFirepit,
  kBirdHide,
  kGuidepost,
  kBoard,
  kMap,
  kOffice,
  kTerminal,
  kAudioguide,
  kViewpoint,
  kHotel,
  kTourismGuestHouse,
  kHostel,
  kChalet,
  kMotel,
  kApartment,
  kAlpinehut,
  kWildernessHut,
  kBank,
  kAtm,
  kBureauDeChange,
  kPharmacy,
  kHospital,
  kDoctors,
  kDentist,
  kVeterinary,
  kPostBox,
  kPostOffice,
  kParcelLocker,
  kTelephone,
  kEmergencyPhone,
  kParking,
  kParkingSubtle,
  kBusStop,
  kFuel,
  kParkingBicycle,
  kRenderingRailwayTramStopMapnik,
  kAmenityBusStation,
  kHelipad,
  kAerodrome,
  kRentalBicycle,
  kTransportSlipway,
  kTaxi,
  kParkingTickets,
  kSubwayEntrance,
  kChargingStation,
  kElevator,
  kRentalCar,
  kParkingEntrance,
  kPublicTransportTickets,
  kFerryIcon,
  kParkingMotorcycle,
  kBicycleRepairStation,
  kBoatRental,
  kParkingEntranceMultiStorey,
  kOneway,
  kBarrierGate,
  kTrafficLight,
  kLevelCrossing2,
  kLevelCrossing,
  kBarrier,
  kLiftgate,
  kCycleBarrier,
  kBarrierStile,
  kHighwayMiniRoundabout,
  kTollBooth,
  kBarrierCattleGrid,
  kKissingGate,
  kFullHeightTurnstile,
  kMotorcycleBarrier,
  kFord,
  kMountainPass,
  kDamNode,
  kWeirNode,
  kLockGateNode,
  kTurningCircleOnHighwayTrack,
  kTree,
  kPeak,
  kSpring,
  kCave,
  kWaterfall,
  kSaddle,
  kVolcano,
  kPolice,
  kTownHall,
  kFireStation,
  kSocialFacility,
  kCourthouse,
  kDiplomatic,
  kOfficeDiplomaticConsulate,
  kPrison,
  kChristian,
  kJewish,
  kMuslim,
  kTaoist,
  kHinduist,
  kBuddhist,
  kShintoist,
  kSikhist,
  kPlaceOfWorship,
  kMarketplace,
  kConvenience,
  kSupermarket,
  kClothes,
  kHairdresser,
  kBakery,
  kCarRepair,
  kDoityourself,
  kPurpleCar,
  kNewsagent,
  kBeauty,
  kCarWash,
  kButcher,
  kAlcohol,
  kFurniture,
  kFlorist,
  kMobilePhone,
  kElectronics,
  kShoes,
  kCarParts,
  kGreengrocer,
  kLaundry,
  kOptician,
  kJewellery,
  kBooks,
  kGift,
  kDepartmentStore,
  kBicycle,
  kConfectionery,
  kVarietyStore,
  kTravelAgency,
  kSports,
  kChemist,
  kComputer,
  kStationery,
  kPet,
  kBeverages,
  kPerfumery,
  kTyres,
  kShopMotorcycle,
  kGardenCentre,
  kCopyshop,
  kToys,
  kDeli,
  kTobacco,
  kSeafood,
  kInteriorDecoration,
  kTicket,
  kPhoto,
  kTrade,
  kOutdoor,
  kHouseware,
  kArt,
  kPaint,
  kFabric,
  kBookmaker,
  kSecondHand,
  kCharity,
  kBed,
  kMedicalSupply,
  kHifi,
  kShopMusic,
  kCoffee,
  kHearingAids,
  kMusicalInstrument,
  kTea,
  kVideo,
  kBag,
  kCarpet,
  kVideoGames,
  kVehicleInspection,
  kDairy,
  kShopOther,
  kSocialAmenityDarken,
  kStorageTank,
  kTowerFreestanding,
  kTowerCantileverCommunication,
  kGeneratorWind,
  kHuntingStand,
  kWaterTower,
  kMastGeneral,
  kBunkerOsmcarto,
  kChimney,
  kTowerObservation,
  kTowerBellTower,
  kTowerLighting,
  kLighthouse,
  kColumn,
  kCrane,
  kWindmill,
  kTowerLatticeCommunication,
  kMastLighting,
  kMastCommunications,
  kCommunicationTower,
  kTowerDefensive,
  kTowerCooling,
  kTowerLattice,
  kTowerLatticeLighting,
  kTowerDish,
  kTowerDome,
  kTelescopeDish,
  kTelescopeDome,
  kPowerTower,
  kPowerPole,
  kPlace,
  kPlaceCapital,
  kRect,
  kEntranceMain,
  kEntrance,
  kRectdiag,
  kExtra
};

constexpr std::array<char const*, 276> amenity_category_names = {
  "none",
  "restaurant",
  "cafe",
  "fast_food",
  "bar",
  "pub",
  "ice_cream",
  "biergarten",
  "outdoor_seating",
  "artwork",
  "community_centre",
  "library",
  "museum",
  "theatre",
  "cinema",
  "nightclub",
  "arts_centre",
  "gallery",
  "internet_cafe",
  "casino",
  "public_bookcase",
  "amusement_arcade",
  "memorial",
  "archaeological_site",
  "carto_shrine",
  "monument",
  "castle",
  "plaque",
  "statue",
  "stone",
  "palace",
  "fortress",
  "historic_fort",
  "bust",
  "city_gate",
  "manor",
  "obelisk",
  "playground",
  "fitness",
  "golf_icon",
  "swimming",
  "massage",
  "sauna",
  "public_bath",
  "miniature_golf",
  "beach_resort",
  "fishing",
  "bowling_alley",
  "dog_park",
  "leisure_dance",
  "leisure_golf_pin",
  "toilets",
  "recycling",
  "waste_basket",
  "waste_disposal",
  "excrement_bags",
  "bench",
  "shelter",
  "drinking_water",
  "picnic_site",
  "fountain",
  "camping",
  "table",
  "caravan",
  "bbq",
  "shower",
  "firepit",
  "bird_hide",
  "guidepost",
  "board",
  "map",
  "office",
  "terminal",
  "audioguide",
  "viewpoint",
  "hotel",
  "tourism_guest_house",
  "hostel",
  "chalet",
  "motel",
  "apartment",
  "alpinehut",
  "wilderness_hut",
  "bank",
  "atm",
  "bureau_de_change",
  "pharmacy",
  "hospital",
  "doctors",
  "dentist",
  "veterinary",
  "post_box",
  "post_office",
  "parcel_locker",
  "telephone",
  "emergency_phone",
  "parking",
  "parking_subtle",
  "bus_stop",
  "fuel",
  "parking_bicycle",
  "rendering_railway_tram_stop_mapnik",
  "amenity_bus_station",
  "helipad",
  "aerodrome",
  "rental_bicycle",
  "transport_slipway",
  "taxi",
  "parking_tickets",
  "subway_entrance",
  "charging_station",
  "elevator",
  "rental_car",
  "parking_entrance",
  "public_transport_tickets",
  "ferry_icon",
  "parking_motorcycle",
  "bicycle_repair_station",
  "boat_rental",
  "parking_entrance_multi_storey",
  "oneway",
  "barrier_gate",
  "traffic_light",
  "level_crossing2",
  "level_crossing",
  "barrier",
  "liftgate",
  "cycle_barrier",
  "barrier_stile",
  "highway_mini_roundabout",
  "toll_booth",
  "barrier_cattle_grid",
  "kissing_gate",
  "full_height_turnstile",
  "motorcycle_barrier",
  "ford",
  "mountain_pass",
  "dam_node",
  "weir_node",
  "lock_gate_node",
  "turning_circle_on_highway_track",
  "tree",
  "peak",
  "spring",
  "cave",
  "waterfall",
  "saddle",
  "volcano",
  "police",
  "town_hall",
  "fire_station",
  "social_facility",
  "courthouse",
  "diplomatic",
  "office_diplomatic_consulate",
  "prison",
  "christian",
  "jewish",
  "muslim",
  "taoist",
  "hinduist",
  "buddhist",
  "shintoist",
  "sikhist",
  "place_of_worship",
  "marketplace",
  "convenience",
  "supermarket",
  "clothes",
  "hairdresser",
  "bakery",
  "car_repair",
  "doityourself",
  "purple_car",
  "newsagent",
  "beauty",
  "car_wash",
  "butcher",
  "alcohol",
  "furniture",
  "florist",
  "mobile_phone",
  "electronics",
  "shoes",
  "car_parts",
  "greengrocer",
  "laundry",
  "optician",
  "jewellery",
  "books",
  "gift",
  "department_store",
  "bicycle",
  "confectionery",
  "variety_store",
  "travel_agency",
  "sports",
  "chemist",
  "computer",
  "stationery",
  "pet",
  "beverages",
  "perfumery",
  "tyres",
  "shop_motorcycle",
  "garden_centre",
  "copyshop",
  "toys",
  "deli",
  "tobacco",
  "seafood",
  "interior_decoration",
  "ticket",
  "photo",
  "trade",
  "outdoor",
  "houseware",
  "art",
  "paint",
  "fabric",
  "bookmaker",
  "second_hand",
  "charity",
  "bed",
  "medical_supply",
  "hifi",
  "shop_music",
  "coffee",
  "hearing_aids",
  "musical_instrument",
  "tea",
  "video",
  "bag",
  "carpet",
  "video_games",
  "vehicle_inspection",
  "dairy",
  "shop_other",
  "social_amenity_darken",
  "storage_tank",
  "tower_freestanding",
  "tower_cantilever_communication",
  "generator_wind",
  "hunting_stand",
  "water_tower",
  "mast_general",
  "bunker_osmcarto",
  "chimney",
  "tower_observation",
  "tower_bell_tower",
  "tower_lighting",
  "lighthouse",
  "column",
  "crane",
  "windmill",
  "tower_lattice_communication",
  "mast_lighting",
  "mast_communications",
  "communication_tower",
  "tower_defensive",
  "tower_cooling",
  "tower_lattice",
  "tower_lattice_lighting",
  "tower_dish",
  "tower_dome",
  "telescope_dish",
  "telescope_dome",
  "power_tower",
  "power_pole",
  "place",
  "place_capital",
  "rect",
  "entrance_main",
  "entrance",
  "rectdiag",
  "extra"
};

inline constexpr char const* to_str(amenity_category const cat) {
  return amenity_category_names[static_cast<std::size_t>(cat)];
}

struct amenity_tags {
  explicit amenity_tags(osmium::TagList const& tags) {
    using namespace std::string_view_literals;
    // Single pass over all tags
    for (auto const& t : tags) {
      switch (cista::hash(std::string_view{t.key()})) {
        case cista::hash("access"): access_ = t.value(); break;
        case cista::hash("advertising"): advertising_ = t.value(); break;
        case cista::hash("aerialway"): aerialway_ = t.value(); break;
        case cista::hash("aeroway"): aeroway_ = t.value(); break;
        case cista::hash("amenity"): amenity_ = t.value(); break;
        case cista::hash("artwork_type"): artwork_type_ = t.value(); break;
        case cista::hash("barrier"): barrier_ = t.value(); break;
        case cista::hash("capital"): capital_ = t.value(); break;
        case cista::hash("castle_type"): castle_type_ = t.value(); break;
        case cista::hash("diplomatic"): diplomatic_ = t.value(); break;
        case cista::hash("emergency"): emergency_ = t.value(); break;
        case cista::hash("entrance"): entrance_ = t.value(); break;
        case cista::hash("ford"): ford_ = t.value(); break;
        case cista::hash("generator:method"): generator_method_ = t.value(); break;
        case cista::hash("generator:source"): generator_source_ = t.value(); break;
        case cista::hash("golf"): golf_ = t.value(); break;
        case cista::hash("highway"): highway_ = t.value(); break;
        case cista::hash("historic"): historic_ = t.value(); break;
        case cista::hash("information"): information_ = t.value(); break;
        case cista::hash("leisure"): leisure_ = t.value(); break;
        case cista::hash("man_made"): man_made_ = t.value(); break;
        case cista::hash("memorial"): memorial_ = t.value(); break;
        case cista::hash("military"): military_ = t.value(); break;
        case cista::hash("mountain_pass"): mountain_pass_ = t.value(); break;
        case cista::hash("natural"): natural_ = t.value(); break;
        case cista::hash("office"): office_ = t.value(); break;
        case cista::hash("oneway"): oneway_ = t.value(); break;
        case cista::hash("parking"): parking_ = t.value(); break;
        case cista::hash("place"): place_ = t.value(); break;
        case cista::hash("power"): power_ = t.value(); break;
        case cista::hash("railway"): railway_ = t.value(); break;
        case cista::hash("religion"): religion_ = t.value(); break;
        case cista::hash("shop"): shop_ = t.value(); break;
        case cista::hash("sport"): sport_ = t.value(); break;
        case cista::hash("telescope:type"): telescope_type_ = t.value(); break;
        case cista::hash("tourism"): tourism_ = t.value(); break;
        case cista::hash("tower:construction"): tower_construction_ = t.value(); break;
        case cista::hash("tower:type"): tower_type_ = t.value(); break;
        case cista::hash("vending"): vending_ = t.value(); break;
        case cista::hash("waterway"): waterway_ = t.value(); break;
        default: break;
      }
    }
  }

  amenity_category get_category() const {
    using namespace std::string_view_literals;
    // Restaurant
    if (amenity_ == "restaurant"sv) return amenity_category::kRestaurant;
    if (amenity_ == "food_court"sv) return amenity_category::kRestaurant;
    // Cafe
    if (amenity_ == "cafe"sv) return amenity_category::kCafe;
    // Fast-food
    if (amenity_ == "fast_food"sv) return amenity_category::kFastFood;
    // Bar
    if (amenity_ == "bar"sv) return amenity_category::kBar;
    // Pub
    if (amenity_ == "pub"sv) return amenity_category::kPub;
    // Ice-cream
    if (amenity_ == "ice_cream"sv) return amenity_category::kIceCream;
    // Biergarten
    if (amenity_ == "biergarten"sv) return amenity_category::kBiergarten;
    // Outdoor_seating
    if (leisure_ == "outdoor_seating"sv) return amenity_category::kOutdoorSeating;
    // Artwork
    if (tourism_ == "artwork"sv) return amenity_category::kArtwork;
    // Community_centre
    if (amenity_ == "community_centre"sv) return amenity_category::kCommunityCentre;
    // Library
    if (amenity_ == "library"sv) return amenity_category::kLibrary;
    // Museum
    if (tourism_ == "museum"sv) return amenity_category::kMuseum;
    // Theatre
    if (amenity_ == "theatre"sv) return amenity_category::kTheatre;
    // Cinema
    if (amenity_ == "cinema"sv) return amenity_category::kCinema;
    // Nightclub
    if (amenity_ == "nightclub"sv) return amenity_category::kNightclub;
    // Arts_centre
    if (amenity_ == "arts_centre"sv) return amenity_category::kArtsCentre;
    // Gallery
    if (tourism_ == "gallery"sv) return amenity_category::kGallery;
    // Internet_cafe
    if (amenity_ == "internet_cafe"sv) return amenity_category::kInternetCafe;
    // Casino
    if (amenity_ == "casino"sv) return amenity_category::kCasino;
    // Public_bookcase
    if (amenity_ == "public_bookcase"sv) return amenity_category::kPublicBookcase;
    // Amusement_arcade
    if (leisure_ == "amusement_arcade"sv) return amenity_category::kAmusementArcade;
    // Memorial
    if (historic_ == "memorial"sv) return amenity_category::kMemorial;
    // Archaeological-site
    if (historic_ == "archaeological_site"sv) return amenity_category::kArchaeologicalSite;
    // Carto_shrine
    if (historic_ == "wayside_shrine"sv) return amenity_category::kCartoShrine;
    // Monument
    if (historic_ == "monument"sv) return amenity_category::kMonument;
    // Castle
    if (historic_ == "castle"sv) return amenity_category::kCastle;
    // Plaque
    if (historic_ == "memorial"sv && memorial_ == "plaque"sv) return amenity_category::kPlaque;
    if (historic_ == "memorial"sv && memorial_ == "blue_plaque"sv) return amenity_category::kPlaque;
    // Statue
    if (historic_ == "memorial"sv && memorial_ == "statue"sv) return amenity_category::kStatue;
    if (tourism_ == "artwork"sv && artwork_type_ == "statue"sv) return amenity_category::kStatue;
    // Stone
    if (historic_ == "memorial"sv && memorial_ == "stone"sv) return amenity_category::kStone;
    // Palace
    if (historic_ == "castle"sv && castle_type_ == "palace"sv) return amenity_category::kPalace;
    if (historic_ == "castle"sv && castle_type_ == "stately"sv) return amenity_category::kPalace;
    // Fortress
    if (historic_ == "castle"sv && !castle_type_.empty()) return amenity_category::kFortress;
    // Historic-fort
    if (historic_ == "fort"sv) return amenity_category::kHistoricFort;
    // Bust
    if (historic_ == "memorial"sv && memorial_ == "bust"sv) return amenity_category::kBust;
    if (tourism_ == "artwork"sv && artwork_type_ == "bust"sv) return amenity_category::kBust;
    // City-gate
    if (historic_ == "city_gate"sv) return amenity_category::kCityGate;
    // Manor
    if (historic_ == "manor"sv) return amenity_category::kManor;
    if (historic_ == "castle"sv && castle_type_ == "manor"sv) return amenity_category::kManor;
    // Obelisk
    if (man_made_ == "obelisk"sv) return amenity_category::kObelisk;
    // Playground
    if (leisure_ == "playground"sv) return amenity_category::kPlayground;
    // Fitness
    if (leisure_ == "fitness_centre"sv) return amenity_category::kFitness;
    if (leisure_ == "fitness_station"sv) return amenity_category::kFitness;
    // Golf-icon
    if (leisure_ == "golf_course"sv) return amenity_category::kGolfIcon;
    // Swimming
    if (leisure_ == "water_park"sv) return amenity_category::kSwimming;
    if (leisure_ == "swimming_area"sv) return amenity_category::kSwimming;
    if (leisure_ == "sports_centre"sv && sport_ == "swimming"sv) return amenity_category::kSwimming;
    // Massage
    if (shop_ == "massage"sv) return amenity_category::kMassage;
    // Sauna
    if (leisure_ == "sauna"sv) return amenity_category::kSauna;
    // Public_bath
    if (amenity_ == "public_bath"sv) return amenity_category::kPublicBath;
    // Miniature_golf
    if (leisure_ == "miniature_golf"sv) return amenity_category::kMiniatureGolf;
    // Beach_resort
    if (leisure_ == "beach_resort"sv) return amenity_category::kBeachResort;
    // Fishing
    if (leisure_ == "fishing"sv) return amenity_category::kFishing;
    // Bowling_alley
    if (leisure_ == "bowling_alley"sv) return amenity_category::kBowlingAlley;
    // Dog_park
    if (leisure_ == "dog_park"sv) return amenity_category::kDogPark;
    // Leisure-dance
    if (leisure_ == "dance"sv) return amenity_category::kLeisureDance;
    // Leisure-golf-pin
    if (golf_ == "pin"sv) return amenity_category::kLeisureGolfPin;
    // Toilets
    if (amenity_ == "toilets"sv) return amenity_category::kToilets;
    // Recycling
    if (amenity_ == "recycling"sv) return amenity_category::kRecycling;
    // Waste-basket
    if (amenity_ == "waste_basket"sv) return amenity_category::kWasteBasket;
    // Waste_disposal
    if (amenity_ == "waste_disposal"sv) return amenity_category::kWasteDisposal;
    // Excrement_bags
    if (amenity_ == "vending_machine"sv && vending_ == "excrement_bags"sv) return amenity_category::kExcrementBags;
    // Bench
    if (amenity_ == "bench"sv) return amenity_category::kBench;
    // Shelter
    if (amenity_ == "shelter"sv) return amenity_category::kShelter;
    // Drinking-water
    if (amenity_ == "drinking_water"sv) return amenity_category::kDrinkingWater;
    // Picnic_site
    if (tourism_ == "picnic_site"sv) return amenity_category::kPicnicSite;
    // Fountain
    if (amenity_ == "fountain"sv) return amenity_category::kFountain;
    // Camping
    if (tourism_ == "camp_site"sv) return amenity_category::kCamping;
    // Table
    if (leisure_ == "picnic_table"sv) return amenity_category::kTable;
    // Caravan
    if (tourism_ == "caravan_site"sv) return amenity_category::kCaravan;
    // Bbq
    if (amenity_ == "bbq"sv) return amenity_category::kBbq;
    // Shower
    if (amenity_ == "shower"sv) return amenity_category::kShower;
    // Firepit
    if (leisure_ == "firepit"sv) return amenity_category::kFirepit;
    // Bird_hide
    if (leisure_ == "bird_hide"sv) return amenity_category::kBirdHide;
    // Guidepost
    if (tourism_ == "information"sv && information_ == "guidepost"sv) return amenity_category::kGuidepost;
    // Board
    if (tourism_ == "information"sv && information_ == "board"sv) return amenity_category::kBoard;
    // Map
    if (tourism_ == "information"sv && information_ == "map"sv) return amenity_category::kMap;
    if (tourism_ == "information"sv && information_ == "tactile_map"sv) return amenity_category::kMap;
    // Office
    if (tourism_ == "information"sv && information_ == "office"sv) return amenity_category::kOffice;
    // Terminal
    if (tourism_ == "information"sv && information_ == "terminal"sv) return amenity_category::kTerminal;
    // Audioguide
    if (tourism_ == "information"sv && information_ == "audioguide"sv) return amenity_category::kAudioguide;
    // Viewpoint
    if (tourism_ == "viewpoint"sv) return amenity_category::kViewpoint;
    // Hotel
    if (tourism_ == "hotel"sv) return amenity_category::kHotel;
    // Tourism_guest_house
    if (tourism_ == "guest_house"sv) return amenity_category::kTourismGuestHouse;
    // Hostel
    if (tourism_ == "hostel"sv) return amenity_category::kHostel;
    // Chalet
    if (tourism_ == "chalet"sv) return amenity_category::kChalet;
    // Motel
    if (tourism_ == "motel"sv) return amenity_category::kMotel;
    // Apartment
    if (tourism_ == "apartment"sv) return amenity_category::kApartment;
    // Alpinehut
    if (tourism_ == "alpine_hut"sv) return amenity_category::kAlpinehut;
    // Wilderness_hut
    if (tourism_ == "wilderness_hut"sv) return amenity_category::kWildernessHut;
    // Bank
    if (amenity_ == "bank"sv) return amenity_category::kBank;
    // Atm
    if (amenity_ == "atm"sv) return amenity_category::kAtm;
    // Bureau_de_change
    if (amenity_ == "bureau_de_change"sv) return amenity_category::kBureauDeChange;
    // Pharmacy
    if (amenity_ == "pharmacy"sv) return amenity_category::kPharmacy;
    // Hospital
    if (amenity_ == "hospital"sv) return amenity_category::kHospital;
    // Doctors
    if (amenity_ == "clinic"sv) return amenity_category::kDoctors;
    if (amenity_ == "doctors"sv) return amenity_category::kDoctors;
    // Dentist
    if (amenity_ == "dentist"sv) return amenity_category::kDentist;
    // Veterinary
    if (amenity_ == "veterinary"sv) return amenity_category::kVeterinary;
    // Post_box
    if (amenity_ == "post_box"sv) return amenity_category::kPostBox;
    // Post_office
    if (amenity_ == "post_office"sv) return amenity_category::kPostOffice;
    // Parcel_locker
    if (amenity_ == "parcel_locker"sv) return amenity_category::kParcelLocker;
    // Telephone
    if (amenity_ == "telephone"sv) return amenity_category::kTelephone;
    // Emergency-phone
    if (emergency_ == "phone"sv) return amenity_category::kEmergencyPhone;
    // Parking
    if (amenity_ == "parking"sv) return amenity_category::kParking;
    // Parking-subtle
    if (amenity_ == "parking"sv && parking_ == "lane"sv) return amenity_category::kParkingSubtle;
    if (amenity_ == "parking"sv && parking_ == "street_side"sv) return amenity_category::kParkingSubtle;
    // Bus-stop
    if (highway_ == "bus_stop"sv) return amenity_category::kBusStop;
    // Fuel
    if (amenity_ == "fuel"sv) return amenity_category::kFuel;
    // Parking-bicycle
    if (amenity_ == "bicycle_parking"sv) return amenity_category::kParkingBicycle;
    // Rendering-railway-tram_stop-mapnik
    if (railway_ == "station"sv) return amenity_category::kRenderingRailwayTramStopMapnik;
    if (railway_ == "halt"sv) return amenity_category::kRenderingRailwayTramStopMapnik;
    if (railway_ == "tram_stop"sv) return amenity_category::kRenderingRailwayTramStopMapnik;
    if (aerialway_ == "station"sv) return amenity_category::kRenderingRailwayTramStopMapnik;
    // Amenity_bus_station
    if (amenity_ == "bus_station"sv) return amenity_category::kAmenityBusStation;
    // Helipad
    if (aeroway_ == "helipad"sv) return amenity_category::kHelipad;
    // Aerodrome
    if (aeroway_ == "aerodrome"sv) return amenity_category::kAerodrome;
    // Rental-bicycle
    if (amenity_ == "bicycle_rental"sv) return amenity_category::kRentalBicycle;
    // Transport_slipway
    if (leisure_ == "slipway"sv) return amenity_category::kTransportSlipway;
    // Taxi
    if (amenity_ == "taxi"sv) return amenity_category::kTaxi;
    // Parking_tickets
    if (amenity_ == "vending_machine"sv && vending_ == "parking_tickets"sv) return amenity_category::kParkingTickets;
    // Subway-entrance
    if (railway_ == "subway_entrance"sv) return amenity_category::kSubwayEntrance;
    // Charging_station
    if (amenity_ == "charging_station"sv) return amenity_category::kChargingStation;
    // Elevator
    if (highway_ == "elevator"sv) return amenity_category::kElevator;
    // Rental-car
    if (amenity_ == "car_rental"sv) return amenity_category::kRentalCar;
    // Parking_entrance
    if (amenity_ == "parking_entrance"sv && parking_ == "underground"sv) return amenity_category::kParkingEntrance;
    // Public_transport_tickets
    if (amenity_ == "vending_machine"sv && vending_ == "public_transport_tickets"sv) return amenity_category::kPublicTransportTickets;
    // Ferry-icon
    if (amenity_ == "ferry_terminal"sv) return amenity_category::kFerryIcon;
    // Parking-motorcycle
    if (amenity_ == "motorcycle_parking"sv) return amenity_category::kParkingMotorcycle;
    // Bicycle_repair_station
    if (amenity_ == "bicycle_repair_station"sv) return amenity_category::kBicycleRepairStation;
    // Boat_rental
    if (amenity_ == "boat_rental"sv) return amenity_category::kBoatRental;
    // Parking_entrance_multi-storey
    if (amenity_ == "parking_entrance"sv && parking_ == "multi-storey"sv) return amenity_category::kParkingEntranceMultiStorey;
    // Oneway
    if (oneway_ == "yes"sv) return amenity_category::kOneway;
    // Barrier_gate
    if (barrier_ == "gate"sv) return amenity_category::kBarrierGate;
    // Traffic_light
    if (highway_ == "traffic_signals"sv) return amenity_category::kTrafficLight;
    // Level_crossing2
    if (railway_ == "level_crossing"sv) return amenity_category::kLevelCrossing2;
    if (railway_ == "crossing"sv) return amenity_category::kLevelCrossing2;
    // Level_crossing
    if (railway_ == "level_crossing"sv) return amenity_category::kLevelCrossing;
    if (railway_ == "crossing"sv) return amenity_category::kLevelCrossing;
    // Barrier
    if (barrier_ == "bollard"sv) return amenity_category::kBarrier;
    if (barrier_ == "block"sv) return amenity_category::kBarrier;
    if (barrier_ == "turnstile"sv) return amenity_category::kBarrier;
    if (barrier_ == "log"sv) return amenity_category::kBarrier;
    // Liftgate
    if (barrier_ == "lift_gate"sv) return amenity_category::kLiftgate;
    if (barrier_ == "swing_gate"sv) return amenity_category::kLiftgate;
    // Cycle_barrier
    if (barrier_ == "cycle_barrier"sv) return amenity_category::kCycleBarrier;
    // Barrier_stile
    if (barrier_ == "stile"sv) return amenity_category::kBarrierStile;
    // Highway_mini_roundabout
    if (highway_ == "mini_roundabout"sv) return amenity_category::kHighwayMiniRoundabout;
    // Toll_booth
    if (barrier_ == "toll_booth"sv) return amenity_category::kTollBooth;
    // Barrier_cattle_grid
    if (barrier_ == "cattle_grid"sv) return amenity_category::kBarrierCattleGrid;
    // Kissing_gate
    if (barrier_ == "kissing_gate"sv) return amenity_category::kKissingGate;
    // Full-height_turnstile
    if (barrier_ == "full-height_turnstile"sv) return amenity_category::kFullHeightTurnstile;
    // Motorcycle_barrier
    if (barrier_ == "motorcycle_barrier"sv) return amenity_category::kMotorcycleBarrier;
    // Ford
    if (ford_ == "yes"sv) return amenity_category::kFord;
    if (ford_ == "stepping_stones"sv) return amenity_category::kFord;
    // Mountain_pass
    if (mountain_pass_ == "yes"sv) return amenity_category::kMountainPass;
    // Dam_node
    if (waterway_ == "dam"sv) return amenity_category::kDamNode;
    // Weir_node
    if (waterway_ == "weir"sv) return amenity_category::kWeirNode;
    // Lock_gate_node
    if (waterway_ == "lock_gate"sv) return amenity_category::kLockGateNode;
    // Turning_circle_on_highway_track
    if (highway_ == "turning_circle"sv && highway_ == "track"sv) return amenity_category::kTurningCircleOnHighwayTrack;
    // Tree
    if (natural_ == "tree"sv) return amenity_category::kTree;
    // Peak
    if (natural_ == "peak"sv) return amenity_category::kPeak;
    // Spring
    if (natural_ == "spring"sv) return amenity_category::kSpring;
    // Cave
    if (natural_ == "cave_entrance"sv) return amenity_category::kCave;
    // Waterfall
    if (waterway_ == "waterfall"sv) return amenity_category::kWaterfall;
    // Saddle
    if (natural_ == "saddle"sv) return amenity_category::kSaddle;
    // Volcano
    if (natural_ == "volcano"sv) return amenity_category::kVolcano;
    // Police
    if (amenity_ == "police"sv) return amenity_category::kPolice;
    // Town-hall
    if (amenity_ == "townhall"sv) return amenity_category::kTownHall;
    // Fire-station
    if (amenity_ == "fire_station"sv) return amenity_category::kFireStation;
    // Social_facility
    if (amenity_ == "social_facility"sv) return amenity_category::kSocialFacility;
    // Courthouse
    if (amenity_ == "courthouse"sv) return amenity_category::kCourthouse;
    // Diplomatic
    if (office_ == "diplomatic"sv && diplomatic_ == "embassy"sv) return amenity_category::kDiplomatic;
    // Office-diplomatic-consulate
    if (office_ == "diplomatic"sv && diplomatic_ == "consulate"sv) return amenity_category::kOfficeDiplomaticConsulate;
    // Prison
    if (amenity_ == "prison"sv) return amenity_category::kPrison;
    // Christian
    if (amenity_ == "place_of_worship"sv && religion_ == "christian"sv) return amenity_category::kChristian;
    // Jewish
    if (amenity_ == "place_of_worship"sv && religion_ == "jewish"sv) return amenity_category::kJewish;
    // Muslim
    if (amenity_ == "place_of_worship"sv && religion_ == "muslim"sv) return amenity_category::kMuslim;
    // Taoist
    if (amenity_ == "place_of_worship"sv && religion_ == "taoist"sv) return amenity_category::kTaoist;
    // Hinduist
    if (amenity_ == "place_of_worship"sv && religion_ == "hindu"sv) return amenity_category::kHinduist;
    // Buddhist
    if (amenity_ == "place_of_worship"sv && religion_ == "buddhist"sv) return amenity_category::kBuddhist;
    // Shintoist
    if (amenity_ == "place_of_worship"sv && religion_ == "shinto"sv) return amenity_category::kShintoist;
    // Sikhist
    if (amenity_ == "place_of_worship"sv && religion_ == "sikh"sv) return amenity_category::kSikhist;
    // Place-of-worship
    if (amenity_ == "place_of_worship"sv && !religion_.empty()) return amenity_category::kPlaceOfWorship;
    // Marketplace
    if (amenity_ == "marketplace"sv) return amenity_category::kMarketplace;
    // Convenience
    if (shop_ == "convenience"sv) return amenity_category::kConvenience;
    // Supermarket
    if (shop_ == "supermarket"sv) return amenity_category::kSupermarket;
    // Clothes
    if (shop_ == "clothes"sv) return amenity_category::kClothes;
    if (shop_ == "fashion"sv) return amenity_category::kClothes;
    // Hairdresser
    if (shop_ == "hairdresser"sv) return amenity_category::kHairdresser;
    // Bakery
    if (shop_ == "bakery"sv) return amenity_category::kBakery;
    // Car_repair
    if (shop_ == "car_repair"sv) return amenity_category::kCarRepair;
    // Doityourself
    if (shop_ == "doityourself"sv) return amenity_category::kDoityourself;
    if (shop_ == "hardware"sv) return amenity_category::kDoityourself;
    // Purple-car
    if (shop_ == "car"sv) return amenity_category::kPurpleCar;
    // Newsagent
    if (shop_ == "kiosk"sv) return amenity_category::kNewsagent;
    if (shop_ == "newsagent"sv) return amenity_category::kNewsagent;
    // Beauty
    if (shop_ == "beauty"sv) return amenity_category::kBeauty;
    // Car_wash
    if (amenity_ == "car_wash"sv) return amenity_category::kCarWash;
    // Butcher
    if (shop_ == "butcher"sv) return amenity_category::kButcher;
    // Alcohol
    if (shop_ == "alcohol"sv) return amenity_category::kAlcohol;
    if (shop_ == "wine"sv) return amenity_category::kAlcohol;
    // Furniture
    if (shop_ == "furniture"sv) return amenity_category::kFurniture;
    // Florist
    if (shop_ == "florist"sv) return amenity_category::kFlorist;
    // Mobile-phone
    if (shop_ == "mobile_phone"sv) return amenity_category::kMobilePhone;
    // Electronics
    if (shop_ == "electronics"sv) return amenity_category::kElectronics;
    // Shoes
    if (shop_ == "shoes"sv) return amenity_category::kShoes;
    // Car_parts
    if (shop_ == "car_parts"sv) return amenity_category::kCarParts;
    // Greengrocer
    if (shop_ == "greengrocer"sv) return amenity_category::kGreengrocer;
    if (shop_ == "farm"sv) return amenity_category::kGreengrocer;
    // Laundry
    if (shop_ == "laundry"sv) return amenity_category::kLaundry;
    if (shop_ == "dry_cleaning"sv) return amenity_category::kLaundry;
    // Optician
    if (shop_ == "optician"sv) return amenity_category::kOptician;
    // Jewellery
    if (shop_ == "jewelry"sv) return amenity_category::kJewellery;
    // Books
    if (shop_ == "books"sv) return amenity_category::kBooks;
    // Gift
    if (shop_ == "gift"sv) return amenity_category::kGift;
    // Department_store
    if (shop_ == "department_store"sv) return amenity_category::kDepartmentStore;
    // Bicycle
    if (shop_ == "bicycle"sv) return amenity_category::kBicycle;
    // Confectionery
    if (shop_ == "confectionery"sv) return amenity_category::kConfectionery;
    if (shop_ == "chocolate"sv) return amenity_category::kConfectionery;
    if (shop_ == "pastry"sv) return amenity_category::kConfectionery;
    // Variety_store
    if (shop_ == "variety_store"sv) return amenity_category::kVarietyStore;
    // Travel_agency
    if (shop_ == "travel_agency"sv) return amenity_category::kTravelAgency;
    // Sports
    if (shop_ == "sports"sv) return amenity_category::kSports;
    // Chemist
    if (shop_ == "chemist"sv) return amenity_category::kChemist;
    // Computer
    if (shop_ == "computer"sv) return amenity_category::kComputer;
    // Stationery
    if (shop_ == "stationery"sv) return amenity_category::kStationery;
    // Pet
    if (shop_ == "pet"sv) return amenity_category::kPet;
    // Beverages
    if (shop_ == "beverages"sv) return amenity_category::kBeverages;
    // Perfumery
    if (shop_ == "cosmetics"sv) return amenity_category::kPerfumery;
    if (shop_ == "perfumery"sv) return amenity_category::kPerfumery;
    // Tyres
    if (shop_ == "tyres"sv) return amenity_category::kTyres;
    // Shop_motorcycle
    if (shop_ == "motorcycle"sv) return amenity_category::kShopMotorcycle;
    // Garden_centre
    if (shop_ == "garden_centre"sv) return amenity_category::kGardenCentre;
    // Copyshop
    if (shop_ == "copyshop"sv) return amenity_category::kCopyshop;
    // Toys
    if (shop_ == "toys"sv) return amenity_category::kToys;
    // Deli
    if (shop_ == "deli"sv) return amenity_category::kDeli;
    // Tobacco
    if (shop_ == "tobacco"sv) return amenity_category::kTobacco;
    // Seafood
    if (shop_ == "seafood"sv && shop_ == "fishmonger"sv) return amenity_category::kSeafood;
    // Interior_decoration
    if (shop_ == "interior_decoration"sv) return amenity_category::kInteriorDecoration;
    // Ticket
    if (shop_ == "ticket"sv) return amenity_category::kTicket;
    // Photo
    if (shop_ == "photo"sv) return amenity_category::kPhoto;
    if (shop_ == "photo_studio"sv) return amenity_category::kPhoto;
    if (shop_ == "photography"sv) return amenity_category::kPhoto;
    // Trade
    if (shop_ == "trade"sv) return amenity_category::kTrade;
    if (shop_ == "wholesale"sv) return amenity_category::kTrade;
    // Outdoor
    if (shop_ == "outdoor"sv) return amenity_category::kOutdoor;
    // Houseware
    if (shop_ == "houseware"sv) return amenity_category::kHouseware;
    // Art
    if (shop_ == "art"sv) return amenity_category::kArt;
    // Paint
    if (shop_ == "paint"sv) return amenity_category::kPaint;
    // Fabric
    if (shop_ == "fabric"sv) return amenity_category::kFabric;
    // Bookmaker
    if (shop_ == "bookmaker"sv) return amenity_category::kBookmaker;
    // Second_hand
    if (shop_ == "second_hand"sv) return amenity_category::kSecondHand;
    // Charity
    if (shop_ == "charity"sv) return amenity_category::kCharity;
    // Bed
    if (shop_ == "bed"sv) return amenity_category::kBed;
    // Medical_supply
    if (shop_ == "medical_supply"sv) return amenity_category::kMedicalSupply;
    // Hifi
    if (shop_ == "hifi"sv) return amenity_category::kHifi;
    // Shop_music
    if (shop_ == "music"sv) return amenity_category::kShopMusic;
    // Coffee
    if (shop_ == "coffee"sv) return amenity_category::kCoffee;
    // Hearing-aids
    if (shop_ == "hearing_aids"sv) return amenity_category::kHearingAids;
    // Musical_instrument
    if (shop_ == "musical_instrument"sv) return amenity_category::kMusicalInstrument;
    // Tea
    if (shop_ == "tea"sv) return amenity_category::kTea;
    // Video
    if (shop_ == "video"sv) return amenity_category::kVideo;
    // Bag
    if (shop_ == "bag"sv) return amenity_category::kBag;
    // Carpet
    if (shop_ == "carpet"sv) return amenity_category::kCarpet;
    // Video_games
    if (shop_ == "video_games"sv) return amenity_category::kVideoGames;
    // Vehicle_inspection
    if (amenity_ == "vehicle_inspection"sv) return amenity_category::kVehicleInspection;
    // Dairy
    if (shop_ == "dairy"sv) return amenity_category::kDairy;
    // Shop-other
    if (!shop_.empty()) return amenity_category::kShopOther;
    if (amenity_ == "driving_school"sv) return amenity_category::kShopOther;
    // Office
    if (!office_.empty()) return amenity_category::kOffice;
    // Social_amenity_darken
    if (amenity_ == "nursing_home"sv) return amenity_category::kSocialAmenityDarken;
    if (amenity_ == "childcare"sv) return amenity_category::kSocialAmenityDarken;
    // Storage_tank
    if (man_made_ == "storage_tank"sv) return amenity_category::kStorageTank;
    if (man_made_ == "silo"sv) return amenity_category::kStorageTank;
    // Tower_freestanding
    if (man_made_ == "tower"sv) return amenity_category::kTowerFreestanding;
    // Tower_cantilever_communication
    if (man_made_ == "tower"sv && tower_type_ == "communication"sv) return amenity_category::kTowerCantileverCommunication;
    // Generator_wind
    if (power_ == "generator"sv && generator_source_ == "wind"sv && generator_method_ == "wind_turbine"sv) return amenity_category::kGeneratorWind;
    // Hunting-stand
    if (amenity_ == "hunting_stand"sv) return amenity_category::kHuntingStand;
    // Christian
    if (historic_ == "wayside_cross"sv) return amenity_category::kChristian;
    if (man_made_ == "cross"sv) return amenity_category::kChristian;
    // Water-tower
    if (man_made_ == "water_tower"sv) return amenity_category::kWaterTower;
    // Mast_general
    if (man_made_ == "mast"sv) return amenity_category::kMastGeneral;
    // Bunker-osmcarto
    if (military_ == "bunker"sv) return amenity_category::kBunkerOsmcarto;
    // Chimney
    if (man_made_ == "chimney"sv) return amenity_category::kChimney;
    // Tower_observation
    if (man_made_ == "tower"sv && tower_type_ == "observation"sv) return amenity_category::kTowerObservation;
    if (man_made_ == "tower"sv && tower_type_ == "watchtower"sv) return amenity_category::kTowerObservation;
    // Tower_bell_tower
    if (man_made_ == "tower"sv && tower_type_ == "bell_tower"sv) return amenity_category::kTowerBellTower;
    // Tower_lighting
    if (man_made_ == "tower"sv && tower_type_ == "lighting"sv) return amenity_category::kTowerLighting;
    // Lighthouse
    if (man_made_ == "lighthouse"sv) return amenity_category::kLighthouse;
    // Column
    if (advertising_ == "column"sv) return amenity_category::kColumn;
    // Crane
    if (man_made_ == "crane"sv) return amenity_category::kCrane;
    // Windmill
    if (man_made_ == "windmill"sv) return amenity_category::kWindmill;
    // Tower_lattice_communication
    if (man_made_ == "tower"sv && tower_type_ == "communication"sv && tower_construction_ == "lattice"sv) return amenity_category::kTowerLatticeCommunication;
    // Mast_lighting
    if (man_made_ == "mast"sv && tower_type_ == "lighting"sv) return amenity_category::kMastLighting;
    // Mast_communications
    if (man_made_ == "mast"sv && tower_type_ == "communication"sv) return amenity_category::kMastCommunications;
    // Communication_tower
    if (man_made_ == "communications_tower"sv) return amenity_category::kCommunicationTower;
    // Tower_defensive
    if (man_made_ == "tower"sv && tower_type_ == "defensive"sv) return amenity_category::kTowerDefensive;
    // Tower_cooling
    if (man_made_ == "tower"sv && tower_type_ == "cooling"sv) return amenity_category::kTowerCooling;
    // Tower_lattice
    if (man_made_ == "tower"sv && tower_construction_ == "lattice"sv) return amenity_category::kTowerLattice;
    // Tower_lattice_lighting
    if (man_made_ == "tower"sv && tower_type_ == "lighting"sv && tower_construction_ == "lattice"sv) return amenity_category::kTowerLatticeLighting;
    // Tower_dish
    if (man_made_ == "tower"sv && tower_construction_ == "dish"sv) return amenity_category::kTowerDish;
    // Tower_dome
    if (man_made_ == "tower"sv && tower_construction_ == "dome"sv) return amenity_category::kTowerDome;
    // Telescope_dish
    if (man_made_ == "telescope"sv && telescope_type_ == "radio"sv) return amenity_category::kTelescopeDish;
    // Telescope_dome
    if (man_made_ == "telescope"sv && telescope_type_ == "optical"sv) return amenity_category::kTelescopeDome;
    // Power_tower
    if (power_ == "tower"sv) return amenity_category::kPowerTower;
    // Power_pole
    if (power_ == "pole"sv) return amenity_category::kPowerPole;
    // Place
    if (place_ == "city"sv) return amenity_category::kPlace;
    // Place-capital
    if (!capital_.empty()) return amenity_category::kPlaceCapital;
    // Rect
    if (entrance_ == "yes"sv) return amenity_category::kRect;
    // Entrance_main
    if (entrance_ == "main"sv) return amenity_category::kEntranceMain;
    // Entrance
    if (entrance_ == "service"sv) return amenity_category::kEntrance;
    // Rectdiag
    if (!entrance_.empty() && access_ == "no"sv) return amenity_category::kRectdiag;
    return amenity_category::kNone;
  }

private:
  std::string_view access_;
  std::string_view advertising_;
  std::string_view aerialway_;
  std::string_view aeroway_;
  std::string_view amenity_;
  std::string_view artwork_type_;
  std::string_view barrier_;
  std::string_view capital_;
  std::string_view castle_type_;
  std::string_view diplomatic_;
  std::string_view emergency_;
  std::string_view entrance_;
  std::string_view ford_;
  std::string_view generator_method_;
  std::string_view generator_source_;
  std::string_view golf_;
  std::string_view highway_;
  std::string_view historic_;
  std::string_view information_;
  std::string_view leisure_;
  std::string_view man_made_;
  std::string_view memorial_;
  std::string_view military_;
  std::string_view mountain_pass_;
  std::string_view natural_;
  std::string_view office_;
  std::string_view oneway_;
  std::string_view parking_;
  std::string_view place_;
  std::string_view power_;
  std::string_view railway_;
  std::string_view religion_;
  std::string_view shop_;
  std::string_view sport_;
  std::string_view telescope_type_;
  std::string_view tourism_;
  std::string_view tower_construction_;
  std::string_view tower_type_;
  std::string_view vending_;
  std::string_view waterway_;
};

}  // namespace osr