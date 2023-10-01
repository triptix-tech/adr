cmake_minimum_required(VERSION 3.20)
project(adr)

include(cmake/pkg.cmake)

option(ADR_MIMALLOC "use mimalloc" ON)
if (ADR_MIMALLOC)
    if(WIN32)
        set(adr-mimalloc-lib mimalloc)
        target_link_libraries(cista INTERFACE mimalloc)
    else()
        set(adr-mimalloc-lib mimalloc-obj)
        target_link_libraries(cista INTERFACE mimalloc-static)
    endif()
    target_compile_definitions(cista INTERFACE CISTA_USE_MIMALLOC=1)
    target_compile_definitions(boost INTERFACE BOOST_ASIO_DISABLE_STD_ALIGNED_ALLOC=1)
endif()


file(GLOB_RECURSE adr-files src/*.cc)
add_library(adr ${adr-files})
target_compile_features(adr PRIVATE cxx_std_23)
target_include_directories(adr PUBLIC include)
target_link_libraries(adr
    osmium
    geo
    cista
    utl
    protozero
    boost
    zlibstatic
    unordered_dense
    tiles-import-library
    utf8proc
    rapidjson
    tg
    rtree
)
target_compile_definitions(adr PUBLIC RAPIDJSON_HAS_STDSTRING)

add_executable(adr-extract exe/extract.cc)
target_link_libraries(adr-extract adr boost-program_options ${adr-mimalloc-lib})

add_executable(adr-contains exe/contains.cc)
target_link_libraries(adr-contains adr ${adr-mimalloc-lib})

add_executable(adr-reverse exe/reverse.cc)
target_link_libraries(adr-reverse adr boost-program_options ${adr-mimalloc-lib})

add_executable(adr-server exe/server.cc)
target_link_libraries(adr-server adr uWebSockets boost-program_options ${adr-mimalloc-lib})

add_executable(adr-typeahead exe/typeahead.cc)
target_link_libraries(adr-typeahead
    adr
    boost-program_options
    ftxui::screen
    ftxui::dom
    ftxui::component
    ${adr-mimalloc-lib}
)

if (MOTIS_MIMALLOC AND WIN32)
    add_custom_command(
            TARGET adr-extract POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
            $<TARGET_FILE:mimalloc>
            $<TARGET_FILE_DIR:adr-extract>
            COMMENT "Copy mimalloc.dll to output directory"
    )
    add_custom_command(
            TARGET adr-extract POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_CURRENT_BINARY_DIR}/deps/mimalloc/mimalloc-redirect.dll"
            $<TARGET_FILE_DIR:adr-extract>
            COMMENT "Copy mimalloc-redirect.dll to output directory"
    )
    add_custom_command(
            TARGET adr-typeahead POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
            $<TARGET_FILE:mimalloc>
            $<TARGET_FILE_DIR:adr-typeahead>
            COMMENT "Copy mimalloc.dll to output directory"
    )
    add_custom_command(
            TARGET adr-typeahead POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_CURRENT_BINARY_DIR}/deps/mimalloc/mimalloc-redirect.dll"
            $<TARGET_FILE_DIR:adr-typeahead>
            COMMENT "Copy mimalloc-redirect.dll to output directory"
    )
endif()

file(GLOB_RECURSE adr-test-files test/*cc)
configure_file (
  ${CMAKE_CURRENT_SOURCE_DIR}/test/test_dir.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/test_dir.h
)
add_executable(adr-test ${adr-test-files})
target_include_directories(adr-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(adr-test gtest adr utl)
