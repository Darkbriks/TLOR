find_package(Git QUIET)
set(TLOR_GIT_COMMIT "")
if(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE TLOR_GIT_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
endif()
if(NOT TLOR_GIT_COMMIT)
    set(TLOR_GIT_COMMIT unknown)
endif()

set(TLOR_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
configure_file(${CMAKE_CURRENT_LIST_DIR}/build_info.hpp.in ${TLOR_GENERATED_DIR}/tlor/detail/build_info.hpp @ONLY)
