function(define_game GAME_NAME)
    project(${GAME_NAME})
    message("Defining game: ${GAME_NAME}")

    # Need to parse DEPENDENCIES
    cmake_parse_arguments(DEFINE_GAME "" "" "MODULES;LINK_LIBS" ${ARGN})

    add_executable(${GAME_NAME} ${SOURCES})
    set_target_properties(${GAME_NAME} PROPERTIES 
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS ON)

    if(NOT "${DEFINE_GAME_MODULES}" STREQUAL "")
        foreach(MODULE IN ITEMS ${DEFINE_GAME_MODULES})
            add_module("${MODULE}" PARENT ${GAME_NAME})
        endforeach()
    endif()
endfunction()