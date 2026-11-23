cmake_minimum_required(VERSION 3.31)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
            "In-source builds are not supported. Build into build/<preset-name>/ instead, which is "
            "what the presets already do:\n"
            "    cmake --preset linux-debug\n"
            "Remove the CMakeCache.txt and CMakeFiles/ this attempt just created before retrying.")
endif()

project(TLOR VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
add_compile_options("$<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang,GNU>:-ffp-contract=off>"
                    "$<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,GNU>:-ffp-contract=off>"
                    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/utf-8>"
                    "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/utf-8>")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(cmake/Dependencies.cmake)
include(cmake/BuildInfo.cmake)

function(tlor_add_library name type)
    set(sources "")
    foreach(dir IN LISTS ARGN)
        file(GLOB_RECURSE dir_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/${dir}/*.cpp)
        list(APPEND sources ${dir_sources})
    endforeach()
    add_library(${name} ${type} ${sources})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endfunction()

tlor_add_library(tlor_model STATIC model)
target_include_directories(tlor_model PRIVATE ${TLOR_GENERATED_DIR})
target_compile_definitions(tlor_model PRIVATE TLOR_BUILD_TYPE="$<CONFIG>")

tlor_add_library(tlor_check STATIC check)
target_link_libraries(tlor_check PUBLIC tlor_model)

tlor_add_library(tlor_core STATIC sim eval report registry steps methods pipeline io)
target_link_libraries(tlor_core PUBLIC tlor_model tlor_check PRIVATE nlohmann_json)
target_include_directories(tlor_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

tlor_add_library(tlor SHARED capi)
set_target_properties(tlor PROPERTIES PREFIX lib IMPORT_PREFIX lib)
target_link_libraries(tlor PRIVATE tlor_core)
target_include_directories(tlor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(tlor PRIVATE TLOR_BUILDING_LIBRARY)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(tlor PRIVATE "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/cmake/tlor.map")
    set_property(TARGET tlor APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/tlor.map)
elseif(APPLE)
    target_link_options(tlor PRIVATE "LINKER:-exported_symbol,_tlor_*")
endif()

file(GLOB_RECURSE TLOR_APP_SOURCES CONFIGURE_DEPENDS src/app/*.cpp)
add_executable(tlor_app ${TLOR_APP_SOURCES})
set_target_properties(tlor_app PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_link_libraries(tlor_app PRIVATE tlor_core)
target_include_directories(tlor_app PRIVATE src)

add_executable(greedy_bound src/tools/greedy_bound.cpp)
target_link_libraries(greedy_bound PRIVATE tlor_core)

enable_testing()
add_subdirectory(tests)
