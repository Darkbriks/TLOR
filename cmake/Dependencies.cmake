include(FetchContent)

FetchContent_Declare(doctest
        URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.11.tar.gz
        SOURCE_SUBDIR none)
FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        SOURCE_SUBDIR none)
FetchContent_MakeAvailable(doctest nlohmann_json)

add_library(doctest INTERFACE)
target_include_directories(doctest SYSTEM INTERFACE ${doctest_SOURCE_DIR})
target_compile_options(doctest INTERFACE "$<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-#warnings>")
target_compile_definitions(doctest INTERFACE DOCTEST_CONFIG_USE_STD_HEADERS)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json SYSTEM INTERFACE ${nlohmann_json_SOURCE_DIR}/include)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
