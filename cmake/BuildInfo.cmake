find_package(Git QUIET)
set(TLOR_GIT_COMMIT "")
if(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE TLOR_GIT_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --absolute-git-dir
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE tlor_git_dir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} symbolic-ref -q HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE tlor_git_ref
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    foreach(tlor_git_file IN ITEMS HEAD packed-refs ${tlor_git_ref})
        if(tlor_git_dir AND EXISTS ${tlor_git_dir}/${tlor_git_file})
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${tlor_git_dir}/${tlor_git_file})
        endif()
    endforeach()
endif()
if(NOT TLOR_GIT_COMMIT)
    set(TLOR_GIT_COMMIT unknown)
endif()

set(TLOR_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
configure_file(${CMAKE_CURRENT_LIST_DIR}/build_info.hpp.in ${TLOR_GENERATED_DIR}/tlor/detail/build_info.hpp @ONLY)
