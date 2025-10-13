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
  kRestaurant14,
  kCafe16,
  kFastFood16,
  kBar16,
  kPub16,
  kIceCream14,
  kBiergarten16,
  kOutdoorSeating14,
  kArtwork14,
  kCommunityCentre14,
  kLibrary16,
  kMuseum16,
  kTheatre16,
  kCinema16,
  kNightclub16,
  kArtsCentre,
  kGallery14,
  kInternetCafe14,
  kCasino14,
  kPublicBookcase14,
  kAmusementArcade14,
  kMemorial16,
  kArchaeologicalSite16,
  kCartoShrine,
  kMonument16,
  kCastle14,
  kPlaque,
  kStatue14,
  kStone14,
  kPalace14,
  kFortress14,
  kHistoricFort,
  kBust14,
  kCityGate14,
  kManor14,
  kObelisk14,
  kPlayground16,
  kFitness,
  kGolfIcon,
  kSwimming16,
  kMassage14,
  kSauna14,
  kPublicBath,
  kMiniatureGolf,
  kBeachResort14,
  kFishing14,
  kBowlingAlley14,
  kDogPark,
  kLeisureDance,
  kLeisureGolfPin,
  kToilets16,
  kRecycling16,
  kWasteBasket12,
  kWasteDisposal14,
  kExcrementBags14,
  kBench16,
  kShelter14,
  kDrinkingWater16,
  kPicnicSite,
  kFountain14,
  kCamping16,
  kTable16,
  kCaravan16,
  kBbq14,
  kShower14,
  kFirepit,
  kBirdHide14,
  kGuidepost14,
  kBoard14,
  kMap14,
  kOffice14,
  kTerminal14,
  kAudioguide14,
  kViewpoint16,
  kHotel16,
  kTourismGuestHouse,
  kHostel16,
  kChalet,
  kMotel16,
  kApartment,
  kAlpinehut,
  kWildernessHut,
  kBank16,
  kAtm14,
  kBureauDeChange14,
  kPharmacy14,
  kHospital14,
  kDoctors14,
  kDentist14,
  kVeterinary14,
  kPostBox12,
  kPostOffice14,
  kParcelLocker,
  kTelephone16,
  kEmergencyPhone16,
  kParking16,
  kParkingSubtle,
  kBusStop12,
  kFuel16,
  kParkingBicycle16,
  kRenderingRailwayTramStopMapnik,
  kAmenityBusStation,
  kHelipad16,
  kAerodrome,
  kRentalBicycle16,
  kTransportSlipway,
  kTaxi16,
  kParkingTickets14,
  kSubwayEntrance12,
  kChargingStation16,
  kElevator12,
  kRentalCar16,
  kParkingEntrance14,
  kPublicTransportTickets14,
  kFerryIcon,
  kParkingMotorcycle16,
  kBicycleRepairStation14,
  kBoatRental14,
  kParkingEntranceMultiStorey14,
  kOneway,
  kBarrierGate,
  kTrafficLight16,
  kLevelCrossing2,
  kLevelCrossing,
  kBarrier,
  kLiftgate7,
  kCycleBarrier14,
  kBarrierStile14,
  kHighwayMiniRoundabout,
  kTollBooth,
  kBarrierCattleGrid14,
  kKissingGate14,
  kFullHeightTurnstile14,
  kMotorcycleBarrier14,
  kFord16,
  kMountainPass8,
  kDamNode,
  kWeirNode,
  kLockGateNode,
  kTurningCircleOnHighwayTrack16,
  kTree16,
  kPeak8,
  kSpring14,
  kCave14,
  kWaterfall14,
  kSaddle8,
  kVolcano8,
  kPolice16,
  kTownHall16,
  kFireStation16,
  kSocialFacility14,
  kCourthouse16,
  kDiplomatic,
  kOfficeDiplomaticConsulate,
  kPrison16,
  kChristian16,
  kJewish16,
  kMuslim16,
  kTaoist16,
  kHinduist16,
  kBuddhist16,
  kShintoist16,
  kSikhist16,
  kPlaceOfWorship16,
  kMarketplace14,
  kConvenience14,
  kSupermarket14,
  kClothes16,
  kHairdresser16,
  kBakery16,
  kCarRepair14,
  kDoityourself16,
  kPurpleCar,
  kNewsagent14,
  kBeauty14,
  kCarWash14,
  kButcher,
  kAlcohol16,
  kFurniture16,
  kFlorist16,
  kMobilePhone16,
  kElectronics16,
  kShoes16,
  kCarParts14,
  kGreengrocer14,
  kLaundry14,
  kOptician16,
  kJewellery16,
  kBooks16,
  kGift16,
  kDepartmentStore16,
  kBicycle16,
  kConfectionery14,
  kVarietyStore14,
  kTravelAgency14,
  kSports14,
  kChemist14,
  kComputer14,
  kStationery14,
  kPet16,
  kBeverages14,
  kPerfumery14,
  kTyres,
  kShopMotorcycle,
  kGardenCentre14,
  kCopyshop14,
  kToys14,
  kDeli14,
  kTobacco14,
  kSeafood14,
  kInteriorDecoration14,
  kTicket14,
  kPhoto14,
  kTrade14,
  kOutdoor14,
  kHouseware14,
  kArt14,
  kPaint14,
  kFabric14,
  kBookmaker14,
  kSecondHand14,
  kCharity14,
  kBed14,
  kMedicalSupply,
  kHifi14,
  kShopMusic,
  kCoffee14,
  kHearingAids,
  kMusicalInstrument14,
  kTea14,
  kVideo14,
  kBag14,
  kCarpet14,
  kVideoGames14,
  kVehicleInspection14,
  kDairy,
  kShopOther16,
  kOffice16,
  kSocialAmenityDarken16,
  kStorageTank14,
  kTowerFreestanding,
  kTowerCantileverCommunication,
  kGeneratorWind14,
  kHuntingStand16,
  kChristian9,
  kWaterTower16,
  kMastGeneral,
  kBunkerOsmcarto,
  kChimney14,
  kTowerObservation,
  kTowerBellTower,
  kTowerLighting,
  kLighthouse16,
  kColumn14,
  kCrane14,
  kWindmill16,
  kTowerLatticeCommunication,
  kMastLighting,
  kMastCommunications,
  kCommunicationTower14,
  kTowerDefensive,
  kTowerCooling,
  kTowerLattice,
  kTowerLatticeLighting,
  kTowerDish,
  kTowerDome,
  kTelescopeDish14,
  kTelescopeDome14,
  kPowerTower,
  kPowerPole,
  kPlace6,
  kPlaceCapital8,
  kRect,
  kEntranceMain,
  kEntrance,
  kRectdiag,
  kExtra
};

constexpr std::array<char const*, 278> amenity_category_names = {
  "none",
  "restaurant_14",
  "cafe_16",
  "fast_food_16",
  "bar_16",
  "pub_16",
  "ice_cream_14",
  "biergarten_16",
  "outdoor_seating_14",
  "artwork_14",
  "community_centre_14",
  "library_16",
  "museum_16",
  "theatre_16",
  "cinema_16",
  "nightclub_16",
  "arts_centre",
  "gallery_14",
  "internet_cafe_14",
  "casino_14",
  "public_bookcase_14",
  "amusement_arcade_14",
  "memorial_16",
  "archaeological_site_16",
  "carto_shrine",
  "monument_16",
  "castle_14",
  "plaque",
  "statue_14",
  "stone_14",
  "palace_14",
  "fortress_14",
  "historic_fort",
  "bust_14",
  "city_gate_14",
  "manor_14",
  "obelisk_14",
  "playground_16",
  "fitness",
  "golf_icon",
  "swimming_16",
  "massage_14",
  "sauna_14",
  "public_bath",
  "miniature_golf",
  "beach_resort_14",
  "fishing_14",
  "bowling_alley_14",
  "dog_park",
  "leisure_dance",
  "leisure_golf_pin",
  "toilets_16",
  "recycling_16",
  "waste_basket_12",
  "waste_disposal_14",
  "excrement_bags_14",
  "bench_16",
  "shelter_14",
  "drinking_water_16",
  "picnic_site",
  "fountain_14",
  "camping_16",
  "table_16",
  "caravan_16",
  "bbq_14",
  "shower_14",
  "firepit",
  "bird_hide_14",
  "guidepost_14",
  "board_14",
  "map_14",
  "office_14",
  "terminal_14",
  "audioguide_14",
  "viewpoint_16",
  "hotel_16",
  "tourism_guest_house",
  "hostel_16",
  "chalet",
  "motel_16",
  "apartment",
  "alpinehut",
  "wilderness_hut",
  "bank_16",
  "atm_14",
  "bureau_de_change_14",
  "pharmacy_14",
  "hospital_14",
  "doctors_14",
  "dentist_14",
  "veterinary_14",
  "post_box_12",
  "post_office_14",
  "parcel_locker",
  "telephone_16",
  "emergency_phone_16",
  "parking_16",
  "parking_subtle",
  "bus_stop_12",
  "fuel_16",
  "parking_bicycle_16",
  "rendering_railway_tram_stop_mapnik",
  "amenity_bus_station",
  "helipad_16",
  "aerodrome",
  "rental_bicycle_16",
  "transport_slipway",
  "taxi_16",
  "parking_tickets_14",
  "subway_entrance_12",
  "charging_station_16",
  "elevator_12",
  "rental_car_16",
  "parking_entrance_14",
  "public_transport_tickets_14",
  "ferry_icon",
  "parking_motorcycle_16",
  "bicycle_repair_station_14",
  "boat_rental_14",
  "parking_entrance_multi_storey_14",
  "oneway",
  "barrier_gate",
  "traffic_light_16",
  "level_crossing2",
  "level_crossing",
  "barrier",
  "liftgate_7",
  "cycle_barrier_14",
  "barrier_stile_14",
  "highway_mini_roundabout",
  "toll_booth",
  "barrier_cattle_grid_14",
  "kissing_gate_14",
  "full_height_turnstile_14",
  "motorcycle_barrier_14",
  "ford_16",
  "mountain_pass_8",
  "dam_node",
  "weir_node",
  "lock_gate_node",
  "turning_circle_on_highway_track_16",
  "tree_16",
  "peak_8",
  "spring_14",
  "cave_14",
  "waterfall_14",
  "saddle_8",
  "volcano_8",
  "police_16",
  "town_hall_16",
  "fire_station_16",
  "social_facility_14",
  "courthouse_16",
  "diplomatic",
  "office_diplomatic_consulate",
  "prison_16",
  "christian_16",
  "jewish_16",
  "muslim_16",
  "taoist_16",
  "hinduist_16",
  "buddhist_16",
  "shintoist_16",
  "sikhist_16",
  "place_of_worship_16",
  "marketplace_14",
  "convenience_14",
  "supermarket_14",
  "clothes_16",
  "hairdresser_16",
  "bakery_16",
  "car_repair_14",
  "doityourself_16",
  "purple_car",
  "newsagent_14",
  "beauty_14",
  "car_wash_14",
  "butcher",
  "alcohol_16",
  "furniture_16",
  "florist_16",
  "mobile_phone_16",
  "electronics_16",
  "shoes_16",
  "car_parts_14",
  "greengrocer_14",
  "laundry_14",
  "optician_16",
  "jewellery_16",
  "books_16",
  "gift_16",
  "department_store_16",
  "bicycle_16",
  "confectionery_14",
  "variety_store_14",
  "travel_agency_14",
  "sports_14",
  "chemist_14",
  "computer_14",
  "stationery_14",
  "pet_16",
  "beverages_14",
  "perfumery_14",
  "tyres",
  "shop_motorcycle",
  "garden_centre_14",
  "copyshop_14",
  "toys_14",
  "deli_14",
  "tobacco_14",
  "seafood_14",
  "interior_decoration_14",
  "ticket_14",
  "photo_14",
  "trade_14",
  "outdoor_14",
  "houseware_14",
  "art_14",
  "paint_14",
  "fabric_14",
  "bookmaker_14",
  "second_hand_14",
  "charity_14",
  "bed_14",
  "medical_supply",
  "hifi_14",
  "shop_music",
  "coffee_14",
  "hearing_aids",
  "musical_instrument_14",
  "tea_14",
  "video_14",
  "bag_14",
  "carpet_14",
  "video_games_14",
  "vehicle_inspection_14",
  "dairy",
  "shop_other_16",
  "office_16",
  "social_amenity_darken_16",
  "storage_tank_14",
  "tower_freestanding",
  "tower_cantilever_communication",
  "generator_wind_14",
  "hunting_stand_16",
  "christian_9",
  "water_tower_16",
  "mast_general",
  "bunker_osmcarto",
  "chimney_14",
  "tower_observation",
  "tower_bell_tower",
  "tower_lighting",
  "lighthouse_16",
  "column_14",
  "crane_14",
  "windmill_16",
  "tower_lattice_communication",
  "mast_lighting",
  "mast_communications",
  "communication_tower_14",
  "tower_defensive",
  "tower_cooling",
  "tower_lattice",
  "tower_lattice_lighting",
  "tower_dish",
  "tower_dome",
  "telescope_dish_14",
  "telescope_dome_14",
  "power_tower",
  "power_pole",
  "place_6",
  "place_capital_8",
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
    // Restaurant-14
    if (amenity_ == "restaurant"sv) return amenity_category::kRestaurant14;
    if (amenity_ == "food_court"sv) return amenity_category::kRestaurant14;
    // Cafe-16
    if (amenity_ == "cafe"sv) return amenity_category::kCafe16;
    // Fast-food-16
    if (amenity_ == "fast_food"sv) return amenity_category::kFastFood16;
    // Bar-16
    if (amenity_ == "bar"sv) return amenity_category::kBar16;
    // Pub-16
    if (amenity_ == "pub"sv) return amenity_category::kPub16;
    // Ice-cream-14
    if (amenity_ == "ice_cream"sv) return amenity_category::kIceCream14;
    // Biergarten-16
    if (amenity_ == "biergarten"sv) return amenity_category::kBiergarten16;
    // Outdoor_seating-14
    if (leisure_ == "outdoor_seating"sv) return amenity_category::kOutdoorSeating14;
    // Artwork-14
    if (tourism_ == "artwork"sv) return amenity_category::kArtwork14;
    // Community_centre-14
    if (amenity_ == "community_centre"sv) return amenity_category::kCommunityCentre14;
    // Library-16
    if (amenity_ == "library"sv) return amenity_category::kLibrary16;
    // Museum-16
    if (tourism_ == "museum"sv) return amenity_category::kMuseum16;
    // Theatre-16
    if (amenity_ == "theatre"sv) return amenity_category::kTheatre16;
    // Cinema-16
    if (amenity_ == "cinema"sv) return amenity_category::kCinema16;
    // Nightclub-16
    if (amenity_ == "nightclub"sv) return amenity_category::kNightclub16;
    // Arts_centre
    if (amenity_ == "arts_centre"sv) return amenity_category::kArtsCentre;
    // Gallery-14
    if (tourism_ == "gallery"sv) return amenity_category::kGallery14;
    // Internet_cafe-14
    if (amenity_ == "internet_cafe"sv) return amenity_category::kInternetCafe14;
    // Casino-14
    if (amenity_ == "casino"sv) return amenity_category::kCasino14;
    // Public_bookcase-14
    if (amenity_ == "public_bookcase"sv) return amenity_category::kPublicBookcase14;
    // Amusement_arcade-14
    if (leisure_ == "amusement_arcade"sv) return amenity_category::kAmusementArcade14;
    // Memorial-16
    if (historic_ == "memorial"sv) return amenity_category::kMemorial16;
    // Archaeological-site-16
    if (historic_ == "archaeological_site"sv) return amenity_category::kArchaeologicalSite16;
    // Carto_shrine
    if (historic_ == "wayside_shrine"sv) return amenity_category::kCartoShrine;
    // Monument-16
    if (historic_ == "monument"sv) return amenity_category::kMonument16;
    // Castle-14
    if (historic_ == "castle"sv) return amenity_category::kCastle14;
    // Plaque
    if (historic_ == "memorial"sv && memorial_ == "plaque"sv) return amenity_category::kPlaque;
    if (historic_ == "memorial"sv && memorial_ == "blue_plaque"sv) return amenity_category::kPlaque;
    // Statue-14
    if (historic_ == "memorial"sv && memorial_ == "statue"sv) return amenity_category::kStatue14;
    if (tourism_ == "artwork"sv && artwork_type_ == "statue"sv) return amenity_category::kStatue14;
    // Stone-14
    if (historic_ == "memorial"sv && memorial_ == "stone"sv) return amenity_category::kStone14;
    // Palace-14
    if (historic_ == "castle"sv && castle_type_ == "palace"sv) return amenity_category::kPalace14;
    if (historic_ == "castle"sv && castle_type_ == "stately"sv) return amenity_category::kPalace14;
    // Fortress-14
    if (historic_ == "castle"sv && !castle_type_.empty()) return amenity_category::kFortress14;
    // Historic-fort
    if (historic_ == "fort"sv) return amenity_category::kHistoricFort;
    // Bust-14
    if (historic_ == "memorial"sv && memorial_ == "bust"sv) return amenity_category::kBust14;
    if (tourism_ == "artwork"sv && artwork_type_ == "bust"sv) return amenity_category::kBust14;
    // City-gate-14
    if (historic_ == "city_gate"sv) return amenity_category::kCityGate14;
    // Manor-14
    if (historic_ == "manor"sv) return amenity_category::kManor14;
    if (historic_ == "castle"sv && castle_type_ == "manor"sv) return amenity_category::kManor14;
    // Obelisk-14
    if (man_made_ == "obelisk"sv) return amenity_category::kObelisk14;
    // Playground-16
    if (leisure_ == "playground"sv) return amenity_category::kPlayground16;
    // Fitness
    if (leisure_ == "fitness_centre"sv) return amenity_category::kFitness;
    if (leisure_ == "fitness_station"sv) return amenity_category::kFitness;
    // Golf-icon
    if (leisure_ == "golf_course"sv) return amenity_category::kGolfIcon;
    // Swimming-16
    if (leisure_ == "water_park"sv) return amenity_category::kSwimming16;
    if (leisure_ == "swimming_area"sv) return amenity_category::kSwimming16;
    if (leisure_ == "sports_centre"sv && sport_ == "swimming"sv) return amenity_category::kSwimming16;
    // Massage-14
    if (shop_ == "massage"sv) return amenity_category::kMassage14;
    // Sauna-14
    if (leisure_ == "sauna"sv) return amenity_category::kSauna14;
    // Public_bath
    if (amenity_ == "public_bath"sv) return amenity_category::kPublicBath;
    // Miniature_golf
    if (leisure_ == "miniature_golf"sv) return amenity_category::kMiniatureGolf;
    // Beach_resort-14
    if (leisure_ == "beach_resort"sv) return amenity_category::kBeachResort14;
    // Fishing-14
    if (leisure_ == "fishing"sv) return amenity_category::kFishing14;
    // Bowling_alley-14
    if (leisure_ == "bowling_alley"sv) return amenity_category::kBowlingAlley14;
    // Dog_park
    if (leisure_ == "dog_park"sv) return amenity_category::kDogPark;
    // Leisure-dance
    if (leisure_ == "dance"sv) return amenity_category::kLeisureDance;
    // Leisure-golf-pin
    if (golf_ == "pin"sv) return amenity_category::kLeisureGolfPin;
    // Toilets-16
    if (amenity_ == "toilets"sv) return amenity_category::kToilets16;
    // Recycling-16
    if (amenity_ == "recycling"sv) return amenity_category::kRecycling16;
    // Waste-basket-12
    if (amenity_ == "waste_basket"sv) return amenity_category::kWasteBasket12;
    // Waste_disposal-14
    if (amenity_ == "waste_disposal"sv) return amenity_category::kWasteDisposal14;
    // Excrement_bags-14
    if (amenity_ == "vending_machine"sv && vending_ == "excrement_bags"sv) return amenity_category::kExcrementBags14;
    // Bench-16
    if (amenity_ == "bench"sv) return amenity_category::kBench16;
    // Shelter-14
    if (amenity_ == "shelter"sv) return amenity_category::kShelter14;
    // Drinking-water-16
    if (amenity_ == "drinking_water"sv) return amenity_category::kDrinkingWater16;
    // Picnic_site
    if (tourism_ == "picnic_site"sv) return amenity_category::kPicnicSite;
    // Fountain-14
    if (amenity_ == "fountain"sv) return amenity_category::kFountain14;
    // Camping-16
    if (tourism_ == "camp_site"sv) return amenity_category::kCamping16;
    // Table-16
    if (leisure_ == "picnic_table"sv) return amenity_category::kTable16;
    // Caravan-16
    if (tourism_ == "caravan_site"sv) return amenity_category::kCaravan16;
    // Bbq-14
    if (amenity_ == "bbq"sv) return amenity_category::kBbq14;
    // Shower-14
    if (amenity_ == "shower"sv) return amenity_category::kShower14;
    // Firepit
    if (leisure_ == "firepit"sv) return amenity_category::kFirepit;
    // Bird_hide-14
    if (leisure_ == "bird_hide"sv) return amenity_category::kBirdHide14;
    // Guidepost-14
    if (tourism_ == "information"sv && information_ == "guidepost"sv) return amenity_category::kGuidepost14;
    // Board-14
    if (tourism_ == "information"sv && information_ == "board"sv) return amenity_category::kBoard14;
    // Map-14
    if (tourism_ == "information"sv && information_ == "map"sv) return amenity_category::kMap14;
    if (tourism_ == "information"sv && information_ == "tactile_map"sv) return amenity_category::kMap14;
    // Office-14
    if (tourism_ == "information"sv && information_ == "office"sv) return amenity_category::kOffice14;
    // Terminal-14
    if (tourism_ == "information"sv && information_ == "terminal"sv) return amenity_category::kTerminal14;
    // Audioguide-14
    if (tourism_ == "information"sv && information_ == "audioguide"sv) return amenity_category::kAudioguide14;
    // Viewpoint-16
    if (tourism_ == "viewpoint"sv) return amenity_category::kViewpoint16;
    // Hotel-16
    if (tourism_ == "hotel"sv) return amenity_category::kHotel16;
    // Tourism_guest_house
    if (tourism_ == "guest_house"sv) return amenity_category::kTourismGuestHouse;
    // Hostel-16
    if (tourism_ == "hostel"sv) return amenity_category::kHostel16;
    // Chalet
    if (tourism_ == "chalet"sv) return amenity_category::kChalet;
    // Motel-16
    if (tourism_ == "motel"sv) return amenity_category::kMotel16;
    // Apartment
    if (tourism_ == "apartment"sv) return amenity_category::kApartment;
    // Alpinehut
    if (tourism_ == "alpine_hut"sv) return amenity_category::kAlpinehut;
    // Wilderness_hut
    if (tourism_ == "wilderness_hut"sv) return amenity_category::kWildernessHut;
    // Bank-16
    if (amenity_ == "bank"sv) return amenity_category::kBank16;
    // Atm-14
    if (amenity_ == "atm"sv) return amenity_category::kAtm14;
    // Bureau_de_change-14
    if (amenity_ == "bureau_de_change"sv) return amenity_category::kBureauDeChange14;
    // Pharmacy-14
    if (amenity_ == "pharmacy"sv) return amenity_category::kPharmacy14;
    // Hospital-14
    if (amenity_ == "hospital"sv) return amenity_category::kHospital14;
    // Doctors-14
    if (amenity_ == "clinic"sv) return amenity_category::kDoctors14;
    if (amenity_ == "doctors"sv) return amenity_category::kDoctors14;
    // Dentist-14
    if (amenity_ == "dentist"sv) return amenity_category::kDentist14;
    // Veterinary-14
    if (amenity_ == "veterinary"sv) return amenity_category::kVeterinary14;
    // Post_box-12
    if (amenity_ == "post_box"sv) return amenity_category::kPostBox12;
    // Post_office-14
    if (amenity_ == "post_office"sv) return amenity_category::kPostOffice14;
    // Parcel_locker
    if (amenity_ == "parcel_locker"sv) return amenity_category::kParcelLocker;
    // Telephone-16
    if (amenity_ == "telephone"sv) return amenity_category::kTelephone16;
    // Emergency-phone-16
    if (emergency_ == "phone"sv) return amenity_category::kEmergencyPhone16;
    // Parking-16
    if (amenity_ == "parking"sv) return amenity_category::kParking16;
    // Parking-subtle
    if (amenity_ == "parking"sv && parking_ == "lane"sv) return amenity_category::kParkingSubtle;
    if (amenity_ == "parking"sv && parking_ == "street_side"sv) return amenity_category::kParkingSubtle;
    // Bus-stop-12
    if (highway_ == "bus_stop"sv) return amenity_category::kBusStop12;
    // Fuel-16
    if (amenity_ == "fuel"sv) return amenity_category::kFuel16;
    // Parking-bicycle-16
    if (amenity_ == "bicycle_parking"sv) return amenity_category::kParkingBicycle16;
    // Rendering-railway-tram_stop-mapnik
    if (railway_ == "station"sv) return amenity_category::kRenderingRailwayTramStopMapnik;
    if (railway_ == "halt"sv) return amenity_category::kRenderingRailwayTramStopMapnik;
    if (railway_ == "tram_stop"sv) return amenity_category::kRenderingRailwayTramStopMapnik;
    if (aerialway_ == "station"sv) return amenity_category::kRenderingRailwayTramStopMapnik;
    // Amenity_bus_station
    if (amenity_ == "bus_station"sv) return amenity_category::kAmenityBusStation;
    // Helipad-16
    if (aeroway_ == "helipad"sv) return amenity_category::kHelipad16;
    // Aerodrome
    if (aeroway_ == "aerodrome"sv) return amenity_category::kAerodrome;
    // Rental-bicycle-16
    if (amenity_ == "bicycle_rental"sv) return amenity_category::kRentalBicycle16;
    // Transport_slipway
    if (leisure_ == "slipway"sv) return amenity_category::kTransportSlipway;
    // Taxi-16
    if (amenity_ == "taxi"sv) return amenity_category::kTaxi16;
    // Parking_tickets-14
    if (amenity_ == "vending_machine"sv && vending_ == "parking_tickets"sv) return amenity_category::kParkingTickets14;
    // Subway-entrance-12
    if (railway_ == "subway_entrance"sv) return amenity_category::kSubwayEntrance12;
    // Charging_station-16
    if (amenity_ == "charging_station"sv) return amenity_category::kChargingStation16;
    // Elevator-12
    if (highway_ == "elevator"sv) return amenity_category::kElevator12;
    // Rental-car-16
    if (amenity_ == "car_rental"sv) return amenity_category::kRentalCar16;
    // Parking_entrance-14
    if (amenity_ == "parking_entrance"sv && parking_ == "underground"sv) return amenity_category::kParkingEntrance14;
    // Public_transport_tickets-14
    if (amenity_ == "vending_machine"sv && vending_ == "public_transport_tickets"sv) return amenity_category::kPublicTransportTickets14;
    // Ferry-icon
    if (amenity_ == "ferry_terminal"sv) return amenity_category::kFerryIcon;
    // Parking-motorcycle-16
    if (amenity_ == "motorcycle_parking"sv) return amenity_category::kParkingMotorcycle16;
    // Bicycle_repair_station-14
    if (amenity_ == "bicycle_repair_station"sv) return amenity_category::kBicycleRepairStation14;
    // Boat_rental-14
    if (amenity_ == "boat_rental"sv) return amenity_category::kBoatRental14;
    // Parking_entrance_multi-storey-14
    if (amenity_ == "parking_entrance"sv && parking_ == "multi-storey"sv) return amenity_category::kParkingEntranceMultiStorey14;
    // Oneway
    if (oneway_ == "yes"sv) return amenity_category::kOneway;
    // Barrier_gate
    if (barrier_ == "gate"sv) return amenity_category::kBarrierGate;
    // Traffic_light-16
    if (highway_ == "traffic_signals"sv) return amenity_category::kTrafficLight16;
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
    // Liftgate-7
    if (barrier_ == "lift_gate"sv) return amenity_category::kLiftgate7;
    if (barrier_ == "swing_gate"sv) return amenity_category::kLiftgate7;
    // Cycle_barrier-14
    if (barrier_ == "cycle_barrier"sv) return amenity_category::kCycleBarrier14;
    // Barrier_stile-14
    if (barrier_ == "stile"sv) return amenity_category::kBarrierStile14;
    // Highway_mini_roundabout
    if (highway_ == "mini_roundabout"sv) return amenity_category::kHighwayMiniRoundabout;
    // Toll_booth
    if (barrier_ == "toll_booth"sv) return amenity_category::kTollBooth;
    // Barrier_cattle_grid-14
    if (barrier_ == "cattle_grid"sv) return amenity_category::kBarrierCattleGrid14;
    // Kissing_gate-14
    if (barrier_ == "kissing_gate"sv) return amenity_category::kKissingGate14;
    // Full-height_turnstile-14
    if (barrier_ == "full-height_turnstile"sv) return amenity_category::kFullHeightTurnstile14;
    // Motorcycle_barrier-14
    if (barrier_ == "motorcycle_barrier"sv) return amenity_category::kMotorcycleBarrier14;
    // Ford-16
    if (ford_ == "yes"sv) return amenity_category::kFord16;
    if (ford_ == "stepping_stones"sv) return amenity_category::kFord16;
    // Mountain_pass-8
    if (mountain_pass_ == "yes"sv) return amenity_category::kMountainPass8;
    // Dam_node
    if (waterway_ == "dam"sv) return amenity_category::kDamNode;
    // Weir_node
    if (waterway_ == "weir"sv) return amenity_category::kWeirNode;
    // Lock_gate_node
    if (waterway_ == "lock_gate"sv) return amenity_category::kLockGateNode;
    // Turning_circle_on_highway_track-16
    if (highway_ == "turning_circle"sv && highway_ == "track"sv) return amenity_category::kTurningCircleOnHighwayTrack16;
    // Tree-16
    if (natural_ == "tree"sv) return amenity_category::kTree16;
    // Peak-8
    if (natural_ == "peak"sv) return amenity_category::kPeak8;
    // Spring-14
    if (natural_ == "spring"sv) return amenity_category::kSpring14;
    // Cave-14
    if (natural_ == "cave_entrance"sv) return amenity_category::kCave14;
    // Waterfall-14
    if (waterway_ == "waterfall"sv) return amenity_category::kWaterfall14;
    // Saddle-8
    if (natural_ == "saddle"sv) return amenity_category::kSaddle8;
    // Volcano-8
    if (natural_ == "volcano"sv) return amenity_category::kVolcano8;
    // Police-16
    if (amenity_ == "police"sv) return amenity_category::kPolice16;
    // Town-hall-16
    if (amenity_ == "townhall"sv) return amenity_category::kTownHall16;
    // Fire-station-16
    if (amenity_ == "fire_station"sv) return amenity_category::kFireStation16;
    // Social_facility-14
    if (amenity_ == "social_facility"sv) return amenity_category::kSocialFacility14;
    // Courthouse-16
    if (amenity_ == "courthouse"sv) return amenity_category::kCourthouse16;
    // Diplomatic
    if (office_ == "diplomatic"sv && diplomatic_ == "embassy"sv) return amenity_category::kDiplomatic;
    // Office-diplomatic-consulate
    if (office_ == "diplomatic"sv && diplomatic_ == "consulate"sv) return amenity_category::kOfficeDiplomaticConsulate;
    // Prison-16
    if (amenity_ == "prison"sv) return amenity_category::kPrison16;
    // Christian-16
    if (amenity_ == "place_of_worship"sv && religion_ == "christian"sv) return amenity_category::kChristian16;
    // Jewish-16
    if (amenity_ == "place_of_worship"sv && religion_ == "jewish"sv) return amenity_category::kJewish16;
    // Muslim-16
    if (amenity_ == "place_of_worship"sv && religion_ == "muslim"sv) return amenity_category::kMuslim16;
    // Taoist-16
    if (amenity_ == "place_of_worship"sv && religion_ == "taoist"sv) return amenity_category::kTaoist16;
    // Hinduist-16
    if (amenity_ == "place_of_worship"sv && religion_ == "hindu"sv) return amenity_category::kHinduist16;
    // Buddhist-16
    if (amenity_ == "place_of_worship"sv && religion_ == "buddhist"sv) return amenity_category::kBuddhist16;
    // Shintoist-16
    if (amenity_ == "place_of_worship"sv && religion_ == "shinto"sv) return amenity_category::kShintoist16;
    // Sikhist-16
    if (amenity_ == "place_of_worship"sv && religion_ == "sikh"sv) return amenity_category::kSikhist16;
    // Place-of-worship-16
    if (amenity_ == "place_of_worship"sv && !religion_.empty()) return amenity_category::kPlaceOfWorship16;
    // Marketplace-14
    if (amenity_ == "marketplace"sv) return amenity_category::kMarketplace14;
    // Convenience-14
    if (shop_ == "convenience"sv) return amenity_category::kConvenience14;
    // Supermarket-14
    if (shop_ == "supermarket"sv) return amenity_category::kSupermarket14;
    // Clothes-16
    if (shop_ == "clothes"sv) return amenity_category::kClothes16;
    if (shop_ == "fashion"sv) return amenity_category::kClothes16;
    // Hairdresser-16
    if (shop_ == "hairdresser"sv) return amenity_category::kHairdresser16;
    // Bakery-16
    if (shop_ == "bakery"sv) return amenity_category::kBakery16;
    // Car_repair-14
    if (shop_ == "car_repair"sv) return amenity_category::kCarRepair14;
    // Doityourself-16
    if (shop_ == "doityourself"sv) return amenity_category::kDoityourself16;
    if (shop_ == "hardware"sv) return amenity_category::kDoityourself16;
    // Purple-car
    if (shop_ == "car"sv) return amenity_category::kPurpleCar;
    // Newsagent-14
    if (shop_ == "kiosk"sv) return amenity_category::kNewsagent14;
    if (shop_ == "newsagent"sv) return amenity_category::kNewsagent14;
    // Beauty-14
    if (shop_ == "beauty"sv) return amenity_category::kBeauty14;
    // Car_wash-14
    if (amenity_ == "car_wash"sv) return amenity_category::kCarWash14;
    // Butcher
    if (shop_ == "butcher"sv) return amenity_category::kButcher;
    // Alcohol-16
    if (shop_ == "alcohol"sv) return amenity_category::kAlcohol16;
    if (shop_ == "wine"sv) return amenity_category::kAlcohol16;
    // Furniture-16
    if (shop_ == "furniture"sv) return amenity_category::kFurniture16;
    // Florist-16
    if (shop_ == "florist"sv) return amenity_category::kFlorist16;
    // Mobile-phone-16
    if (shop_ == "mobile_phone"sv) return amenity_category::kMobilePhone16;
    // Electronics-16
    if (shop_ == "electronics"sv) return amenity_category::kElectronics16;
    // Shoes-16
    if (shop_ == "shoes"sv) return amenity_category::kShoes16;
    // Car_parts-14
    if (shop_ == "car_parts"sv) return amenity_category::kCarParts14;
    // Greengrocer-14
    if (shop_ == "greengrocer"sv) return amenity_category::kGreengrocer14;
    if (shop_ == "farm"sv) return amenity_category::kGreengrocer14;
    // Laundry-14
    if (shop_ == "laundry"sv) return amenity_category::kLaundry14;
    if (shop_ == "dry_cleaning"sv) return amenity_category::kLaundry14;
    // Optician-16
    if (shop_ == "optician"sv) return amenity_category::kOptician16;
    // Jewellery-16
    if (shop_ == "jewelry"sv) return amenity_category::kJewellery16;
    // Books-16
    if (shop_ == "books"sv) return amenity_category::kBooks16;
    // Gift-16
    if (shop_ == "gift"sv) return amenity_category::kGift16;
    // Department_store-16
    if (shop_ == "department_store"sv) return amenity_category::kDepartmentStore16;
    // Bicycle-16
    if (shop_ == "bicycle"sv) return amenity_category::kBicycle16;
    // Confectionery-14
    if (shop_ == "confectionery"sv) return amenity_category::kConfectionery14;
    if (shop_ == "chocolate"sv) return amenity_category::kConfectionery14;
    if (shop_ == "pastry"sv) return amenity_category::kConfectionery14;
    // Variety_store-14
    if (shop_ == "variety_store"sv) return amenity_category::kVarietyStore14;
    // Travel_agency-14
    if (shop_ == "travel_agency"sv) return amenity_category::kTravelAgency14;
    // Sports-14
    if (shop_ == "sports"sv) return amenity_category::kSports14;
    // Chemist-14
    if (shop_ == "chemist"sv) return amenity_category::kChemist14;
    // Computer-14
    if (shop_ == "computer"sv) return amenity_category::kComputer14;
    // Stationery-14
    if (shop_ == "stationery"sv) return amenity_category::kStationery14;
    // Pet-16
    if (shop_ == "pet"sv) return amenity_category::kPet16;
    // Beverages-14
    if (shop_ == "beverages"sv) return amenity_category::kBeverages14;
    // Perfumery-14
    if (shop_ == "cosmetics"sv) return amenity_category::kPerfumery14;
    if (shop_ == "perfumery"sv) return amenity_category::kPerfumery14;
    // Tyres
    if (shop_ == "tyres"sv) return amenity_category::kTyres;
    // Shop_motorcycle
    if (shop_ == "motorcycle"sv) return amenity_category::kShopMotorcycle;
    // Garden_centre-14
    if (shop_ == "garden_centre"sv) return amenity_category::kGardenCentre14;
    // Copyshop-14
    if (shop_ == "copyshop"sv) return amenity_category::kCopyshop14;
    // Toys-14
    if (shop_ == "toys"sv) return amenity_category::kToys14;
    // Deli-14
    if (shop_ == "deli"sv) return amenity_category::kDeli14;
    // Tobacco-14
    if (shop_ == "tobacco"sv) return amenity_category::kTobacco14;
    // Seafood-14
    if (shop_ == "seafood"sv && shop_ == "fishmonger"sv) return amenity_category::kSeafood14;
    // Interior_decoration-14
    if (shop_ == "interior_decoration"sv) return amenity_category::kInteriorDecoration14;
    // Ticket-14
    if (shop_ == "ticket"sv) return amenity_category::kTicket14;
    // Photo-14
    if (shop_ == "photo"sv) return amenity_category::kPhoto14;
    if (shop_ == "photo_studio"sv) return amenity_category::kPhoto14;
    if (shop_ == "photography"sv) return amenity_category::kPhoto14;
    // Trade-14
    if (shop_ == "trade"sv) return amenity_category::kTrade14;
    if (shop_ == "wholesale"sv) return amenity_category::kTrade14;
    // Outdoor-14
    if (shop_ == "outdoor"sv) return amenity_category::kOutdoor14;
    // Houseware-14
    if (shop_ == "houseware"sv) return amenity_category::kHouseware14;
    // Art-14
    if (shop_ == "art"sv) return amenity_category::kArt14;
    // Paint-14
    if (shop_ == "paint"sv) return amenity_category::kPaint14;
    // Fabric-14
    if (shop_ == "fabric"sv) return amenity_category::kFabric14;
    // Bookmaker-14
    if (shop_ == "bookmaker"sv) return amenity_category::kBookmaker14;
    // Second_hand-14
    if (shop_ == "second_hand"sv) return amenity_category::kSecondHand14;
    // Charity-14
    if (shop_ == "charity"sv) return amenity_category::kCharity14;
    // Bed-14
    if (shop_ == "bed"sv) return amenity_category::kBed14;
    // Medical_supply
    if (shop_ == "medical_supply"sv) return amenity_category::kMedicalSupply;
    // Hifi-14
    if (shop_ == "hifi"sv) return amenity_category::kHifi14;
    // Shop_music
    if (shop_ == "music"sv) return amenity_category::kShopMusic;
    // Coffee-14
    if (shop_ == "coffee"sv) return amenity_category::kCoffee14;
    // Hearing-aids
    if (shop_ == "hearing_aids"sv) return amenity_category::kHearingAids;
    // Musical_instrument-14
    if (shop_ == "musical_instrument"sv) return amenity_category::kMusicalInstrument14;
    // Tea-14
    if (shop_ == "tea"sv) return amenity_category::kTea14;
    // Video-14
    if (shop_ == "video"sv) return amenity_category::kVideo14;
    // Bag-14
    if (shop_ == "bag"sv) return amenity_category::kBag14;
    // Carpet-14
    if (shop_ == "carpet"sv) return amenity_category::kCarpet14;
    // Video_games-14
    if (shop_ == "video_games"sv) return amenity_category::kVideoGames14;
    // Vehicle_inspection-14
    if (amenity_ == "vehicle_inspection"sv) return amenity_category::kVehicleInspection14;
    // Dairy
    if (shop_ == "dairy"sv) return amenity_category::kDairy;
    // Shop-other-16
    if (!shop_.empty()) return amenity_category::kShopOther16;
    if (amenity_ == "driving_school"sv) return amenity_category::kShopOther16;
    // Office-16
    if (!office_.empty()) return amenity_category::kOffice16;
    // Social_amenity_darken-16
    if (amenity_ == "nursing_home"sv) return amenity_category::kSocialAmenityDarken16;
    if (amenity_ == "childcare"sv) return amenity_category::kSocialAmenityDarken16;
    // Storage_tank-14
    if (man_made_ == "storage_tank"sv) return amenity_category::kStorageTank14;
    if (man_made_ == "silo"sv) return amenity_category::kStorageTank14;
    // Tower_freestanding
    if (man_made_ == "tower"sv) return amenity_category::kTowerFreestanding;
    // Tower_cantilever_communication
    if (man_made_ == "tower"sv && tower_type_ == "communication"sv) return amenity_category::kTowerCantileverCommunication;
    // Generator_wind-14
    if (power_ == "generator"sv && generator_source_ == "wind"sv && generator_method_ == "wind_turbine"sv) return amenity_category::kGeneratorWind14;
    // Hunting-stand-16
    if (amenity_ == "hunting_stand"sv) return amenity_category::kHuntingStand16;
    // Christian-9
    if (historic_ == "wayside_cross"sv) return amenity_category::kChristian9;
    if (man_made_ == "cross"sv) return amenity_category::kChristian9;
    // Water-tower-16
    if (man_made_ == "water_tower"sv) return amenity_category::kWaterTower16;
    // Mast_general
    if (man_made_ == "mast"sv) return amenity_category::kMastGeneral;
    // Bunker-osmcarto
    if (military_ == "bunker"sv) return amenity_category::kBunkerOsmcarto;
    // Chimney-14
    if (man_made_ == "chimney"sv) return amenity_category::kChimney14;
    // Tower_observation
    if (man_made_ == "tower"sv && tower_type_ == "observation"sv) return amenity_category::kTowerObservation;
    if (man_made_ == "tower"sv && tower_type_ == "watchtower"sv) return amenity_category::kTowerObservation;
    // Tower_bell_tower
    if (man_made_ == "tower"sv && tower_type_ == "bell_tower"sv) return amenity_category::kTowerBellTower;
    // Tower_lighting
    if (man_made_ == "tower"sv && tower_type_ == "lighting"sv) return amenity_category::kTowerLighting;
    // Lighthouse-16
    if (man_made_ == "lighthouse"sv) return amenity_category::kLighthouse16;
    // Column-14
    if (advertising_ == "column"sv) return amenity_category::kColumn14;
    // Crane-14
    if (man_made_ == "crane"sv) return amenity_category::kCrane14;
    // Windmill-16
    if (man_made_ == "windmill"sv) return amenity_category::kWindmill16;
    // Tower_lattice_communication
    if (man_made_ == "tower"sv && tower_type_ == "communication"sv && tower_construction_ == "lattice"sv) return amenity_category::kTowerLatticeCommunication;
    // Mast_lighting
    if (man_made_ == "mast"sv && tower_type_ == "lighting"sv) return amenity_category::kMastLighting;
    // Mast_communications
    if (man_made_ == "mast"sv && tower_type_ == "communication"sv) return amenity_category::kMastCommunications;
    // Communication_tower-14
    if (man_made_ == "communications_tower"sv) return amenity_category::kCommunicationTower14;
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
    // Telescope_dish-14
    if (man_made_ == "telescope"sv && telescope_type_ == "radio"sv) return amenity_category::kTelescopeDish14;
    // Telescope_dome-14
    if (man_made_ == "telescope"sv && telescope_type_ == "optical"sv) return amenity_category::kTelescopeDome14;
    // Power_tower
    if (power_ == "tower"sv) return amenity_category::kPowerTower;
    // Power_pole
    if (power_ == "pole"sv) return amenity_category::kPowerPole;
    // Place-6
    if (place_ == "city"sv) return amenity_category::kPlace6;
    // Place-capital-8
    if (!capital_.empty()) return amenity_category::kPlaceCapital8;
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