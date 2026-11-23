file(GLOB_RECURSE TLOR_TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/support/*.cpp)

add_executable(tlor_tests ${TLOR_TEST_SOURCES})
target_include_directories(tlor_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tlor_tests PRIVATE tlor_core doctest nlohmann_json)
target_compile_definitions(tlor_tests PRIVATE TLOR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
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
target_compile_definitions(tlor_capi_tests PRIVATE TLOR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
doctest_discover_tests(tlor_capi_tests)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    execute_process(COMMAND ${Python3_EXECUTABLE} -c "import numpy, pytest"
            RESULT_VARIABLE tlor_python_ready OUTPUT_QUIET ERROR_QUIET)
    if(tlor_python_ready EQUAL 0)
        add_test(NAME python
                COMMAND ${Python3_EXECUTABLE} -m pytest -q -p no:cacheprovider ${CMAKE_SOURCE_DIR}/python/tests)
        set_tests_properties(python PROPERTIES
                ENVIRONMENT "TLOR_LIBRARY=$<TARGET_FILE:tlor>;PYTHONDONTWRITEBYTECODE=1"
                ENVIRONMENT_MODIFICATION "PYTHONPATH=path_list_prepend:${CMAKE_SOURCE_DIR}/python"
                LABELS python)
    else()
        message(STATUS "Python tests disabled: numpy or pytest is missing for ${Python3_EXECUTABLE}")
    endif()
else()
    message(STATUS "Python tests disabled: no Python 3 interpreter found")
endif()

set(TLOR_APP_FIXTURE ${CMAKE_SOURCE_DIR}/tests/fixtures/tiny)
set(TLOR_APP_OUT ${CMAKE_BINARY_DIR}/app-test)
set(TLOR_APP_REQUIRED --tables ${TLOR_APP_FIXTURE} --maps ${TLOR_APP_FIXTURE}
        --service-time ${TLOR_APP_FIXTURE}/service_time.csv --out ${TLOR_APP_OUT}
        --trucks 1 --truck-capacity-kg 1000 --truck-max-work-s 28800)
add_test(NAME app.clean COMMAND ${CMAKE_COMMAND} -E rm -rf ${TLOR_APP_OUT})
set_tests_properties(app.clean PROPERTIES FIXTURES_SETUP app_clean)
add_test(NAME app.run COMMAND $<TARGET_FILE:tlor_app> ${TLOR_APP_REQUIRED} --horizon 2 --report ${TLOR_APP_OUT}/report.json)
set_tests_properties(app.run PROPERTIES FIXTURES_REQUIRED app_clean FIXTURES_SETUP app
        PASS_REGULAR_EXPRESSION "distance_m [0-9]")
add_test(NAME app.outputs COMMAND ${CMAKE_COMMAND} -DOUT=${TLOR_APP_OUT} -P ${CMAKE_CURRENT_SOURCE_DIR}/app/check_outputs.cmake)
set_tests_properties(app.outputs PROPERTIES FIXTURES_REQUIRED app)

function(tlor_app_exit_test name expect stderr)
    list(JOIN ARGN "," arguments)
    add_test(NAME app.${name} COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:tlor_app> -DEXPECT=${expect}
            "-DSTDERR=${stderr}" "-DARGS=${arguments}" -P ${CMAKE_CURRENT_SOURCE_DIR}/app/expect_exit.cmake)
endfunction()
tlor_app_exit_test(usage 2 "usage: TLOR" --bogus)
tlor_app_exit_test(missing-value 2 "--horizon needs a value" ${TLOR_APP_REQUIRED} --horizon)
tlor_app_exit_test(missing-option 2 "--trucks is required" --tables ${TLOR_APP_FIXTURE} --maps ${TLOR_APP_FIXTURE}
        --out ${TLOR_APP_OUT}-missing --truck-capacity-kg 1000 --truck-max-work-s 28800)
tlor_app_exit_test(bad-number 2 "--step" ${TLOR_APP_REQUIRED} --step 1.5)
tlor_app_exit_test(bad-param 2 "--param" ${TLOR_APP_REQUIRED} --param threshold=1)
tlor_app_exit_test(library-error 1 "active_points.csv" --tables ${TLOR_APP_FIXTURE}/nowhere --maps ${TLOR_APP_FIXTURE}
        --out ${TLOR_APP_OUT}-error --trucks 1 --truck-capacity-kg 1000 --truck-max-work-s 28800)
tlor_app_exit_test(unknown-method 1 "unknown" ${TLOR_APP_REQUIRED} --route nowhere)

add_test(NAME tools.greedy_bound COMMAND $<TARGET_FILE:greedy_bound>)
set_tests_properties(tools.greedy_bound PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        PASS_REGULAR_EXPRESSION "Number of tours: 14\nTotal distance: 934.4 km")
