cmake_minimum_required(VERSION 3.20)
project(adr)

include(cmake/pkg.cmake)

file(GLOB_RECURSE adr-files src/*.cc)
add_library(adr ${adr-files})
target_compile_features(adr PRIVATE cxx_std_23)
target_include_directories(adr PUBLIC include)
target_link_libraries(adr osmium geo cista utl protozero guess boost zlibstatic unordered_dense)

add_executable(adr-extract exe/extract.cc)
target_link_libraries(adr-extract adr boost-program_options)

add_executable(adr-typeahead exe/typeahead.cc)
target_link_libraries(adr-typeahead
    adr
    boost-program_options
    ftxui::screen
    ftxui::dom
    ftxui::component
)

file(GLOB_RECURSE adr-test-files test/*cc)
configure_file (
  ${CMAKE_CURRENT_SOURCE_DIR}/test/test_dir.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/test_dir.h
)
add_executable(adr-test ${adr-test-files})
target_include_directories(adr-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(adr-test gtest adr utl)