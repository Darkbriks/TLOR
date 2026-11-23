file(GLOB_RECURSE TLOR_TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/support/*.cpp)

add_executable(tlor_tests ${TLOR_TEST_SOURCES})
target_include_directories(tlor_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tlor_tests PRIVATE tlor_core doctest nlohmann_json)
doctest_discover_tests(tlor_tests)

add_executable(tlor_check_isolation ${CMAKE_CURRENT_SOURCE_DIR}/check_isolation/main.cpp)
target_link_libraries(tlor_check_isolation PRIVATE tlor_check tlor_model)
add_test(NAME check-isolation COMMAND tlor_check_isolation)

if(GIT_FOUND)
    add_test(NAME build-info-commit
            COMMAND ${CMAKE_COMMAND} -DGIT=${GIT_EXECUTABLE} -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DHEADER=${TLOR_GENERATED_DIR}/tlor/detail/build_info.hpp
            -P ${CMAKE_CURRENT_SOURCE_DIR}/build_info/check_commit.cmake)
endif()

file(GLOB_RECURSE TLOR_CAPI_TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/capi/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/capi/*.c)
add_executable(tlor_capi_tests ${TLOR_CAPI_TEST_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/support/small_arrays.cpp)
target_include_directories(tlor_capi_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tlor_capi_tests PRIVATE tlor doctest)
doctest_discover_tests(tlor_capi_tests)
