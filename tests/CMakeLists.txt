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
