execute_process(
        COMMAND ${GIT} rev-parse --short HEAD
        WORKING_DIRECTORY ${SOURCE_DIR}
        OUTPUT_VARIABLE head
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE rc
        ERROR_QUIET)
if(NOT rc EQUAL 0)
    message(STATUS "git is not available, nothing to compare")
    return()
endif()
file(READ ${HEADER} content)
string(FIND "${content}" "\"${head}\"" position)
if(position EQUAL -1)
    message(FATAL_ERROR "build information names a stale commit; HEAD is ${head}")
endif()
