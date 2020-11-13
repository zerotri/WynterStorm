macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

function(define_module MODULE_NAME)
    project(${MODULE_NAME})
    message("Defining module: ${MODULE_NAME}")

    # Need to parse DEPENDENCIES
    cmake_parse_arguments(DEFINE_MODULE "INCLUDES_MODULE_BASE" "THIRD_PARTY" "DEPENDENCIES;LINK_LIBS" ${ARGN})

    if(DEFINE_MODULE_DEPENDENCIES)
        message("Depends on:")
        foreach(DEPENDENCY ${DEFINE_MODULE_DEPENDENCIES})
            message("  - ${DEPENDENCY}")
        endforeach()
        set_property(GLOBAL PROPERTY MODULE_${MODULE_NAME}_DEPENDENCIES "${DEFINE_MODULE_DEPENDENCIES}")
    endif()

    if(DEFINE_MODULE_THIRD_PARTY)
        set(MODULE_BASEDIR "${ENGINE_MODULE_THIRDPARTY_DIRECTORY}/${MODULE_NAME}/${DEFINE_MODULE_THIRD_PARTY}")
        set_property(GLOBAL PROPERTY MODULE_${MODULE_NAME}_IS_THIRD_PARTY TRUE)
    else()
        set_property(GLOBAL PROPERTY MODULE_${MODULE_NAME}_IS_THIRD_PARTY FALSE)
        set(MODULE_BASEDIR "${ENGINE_MODULE_DIRECTORY}/${MODULE_NAME}")
        
        # Define module directories and recurse for files
        file(GLOB_RECURSE MODULE_SOURCES "${MODULE_BASEDIR}/Private/*.cpp")
        file(GLOB_RECURSE MODULE_PRIVATE_HEADERS "${MODULE_BASEDIR}/Private/*.h")
        file(GLOB_RECURSE MODULE_PUBLIC_HEADERS "${MODULE_BASEDIR}/Public/*.h")

        if(APPLE)
            file(GLOB_RECURSE MODULE_OBJ_C_SOURCES "${MODULE_BASEDIR}/Private/*.mm")
        endif()
    endif()

    set_property(GLOBAL PROPERTY MODULE_${MODULE_NAME}_BASEDIR "${MODULE_BASEDIR}")

    if(DEFINE_MODULE_LINK_LIBS)
        set_property(GLOBAL PROPERTY MODULE_${MODULE_NAME}_LINK_LIBS "${DEFINE_MODULE_LINK_LIBS}")
    endif()

    
    if(DEFINE_MODULE_INCLUDES_MODULE_BASE)
        message("Including module base: ${MODULE_BASEDIR}")
        set_property(GLOBAL PROPERTY MODULE_${MODULE_NAME}_PUBLIC_INCLUDEDIRS "${MODULE_BASEDIR}/")
    else()
        message("Not including module base")
        set_property(GLOBAL PROPERTY MODULE_${MODULE_NAME}_PRIVATE_INCLUDEDIRS "${MODULE_BASEDIR}/Private")
        set_property(GLOBAL PROPERTY MODULE_${MODULE_NAME}_PUBLIC_INCLUDEDIRS "${MODULE_BASEDIR}/Public")

        # Print module files
        message("Module sources (C++):")
        foreach(SOURCE ${MODULE_SOURCES})
            message("  - ${SOURCE}")
        endforeach()

        if(APPLE)
            message("Module sources (Obj-C):")
            foreach(SOURCE ${MODULE_OBJ_C_SOURCES})
                message("  - ${SOURCE}")
            endforeach()
        endif()
    
        message("Module headers (private):")
        foreach(HEADER ${MODULE_PRIVATE_HEADERS})
            message("  - ${HEADER}")
        endforeach()

        message("Module headers (public):")
        foreach(HEADER ${MODULE_PUBLIC_HEADERS})
            message("  - ${HEADER}")
        endforeach()
    endif()
    
    if(NOT "${MODULE_SOURCES}" STREQUAL "")
        message("${MODULE_NAME} Sources: ${MODULE_SOURCES}")
        
        # Define the module library and add the sources
        add_library(${MODULE_NAME} OBJECT ${MODULE_SOURCES} ${MODULE_OBJ_C_SOURCES})

        # C++11 mode (with extensions)
        # target_compile_features(${MODULE_NAME} PUBLIC cxx_std_11)
        set_target_properties(${MODULE_NAME} PROPERTIES 
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS ON)

        target_include_directories(${MODULE_NAME} PUBLIC
            "${MODULE_BASEDIR}/Public"
        )

        # Add sources
        source_group("${ENGINE_MODULE_DIRECTORY}/${MODULE_NAME}/Private" FILES ${MODULE_SOURCES}) # ${MODULE_PRIVATE_HEADERS}
    endif()
endfunction()

function(add_module MODULE_NAME)
    cmake_parse_arguments(ADD_MODULE "" "PARENT" "" ${ARGN})

    if(ADD_MODULE_PARENT)
        set(MODULE_PARENT "${ADD_MODULE_PARENT}")
    else()
        set(MODULE_PARENT "${ENGINE_TARGET_NAME}")
    endif()
    
    get_property(PRIVATE_INCLUDEDIRS GLOBAL PROPERTY "MODULE_${MODULE_NAME}_PRIVATE_INCLUDEDIRS")
    message("${MODULE_NAME} private includes: ${PRIVATE_INCLUDEDIRS}" )
    if(NOT "${PRIVATE_INCLUDEDIRS}" STREQUAL "")
        target_include_directories(${MODULE_NAME} PUBLIC
            "${PRIVATE_INCLUDEDIRS}/"
        )
    endif()

    
    get_property(PUBLIC_INCLUDEDIRS GLOBAL PROPERTY "MODULE_${MODULE_NAME}_PUBLIC_INCLUDEDIRS")
    message("${MODULE_NAME} public includes: ${PUBLIC_INCLUDEDIRS}" )
    if(NOT "${PUBLIC_INCLUDEDIRS}" STREQUAL "")
        message("Including headers: ${PUBLIC_INCLUDEDIRS}/")
        target_include_directories(${MODULE_PARENT} PUBLIC
            "${PUBLIC_INCLUDEDIRS}/"
        )
    endif()

    get_property(DEPENDENCIES GLOBAL PROPERTY "MODULE_${MODULE_NAME}_DEPENDENCIES")
    message("${MODULE_NAME} depends: ${DEPENDENCIES}" )
    if(NOT "${DEPENDENCIES}" STREQUAL "")
        message("Depends: ${DEPENDENCIES}")
        foreach(DEPENDENCY ${DEPENDENCIES})
            message("  adding depend: ${DEPENDENCY}")
            add_module("${DEPENDENCY}" PARENT "${MODULE_NAME}")
        endforeach()
    endif()

    # Link module into engine build
    message("Linking ${MODULE_PARENT} to module: ${MODULE_NAME}")
    get_property(IS_THIRD_PARTY GLOBAL PROPERTY "MODULE_${MODULE_NAME}_IS_THIRD_PARTY")
    get_property(MODULE_LINK_LIBS GLOBAL PROPERTY "MODULE_${MODULE_NAME}_LINK_LIBS")
    if(NOT IS_THIRD_PARTY)
        target_link_libraries(${MODULE_PARENT} PUBLIC ${MODULE_NAME})
    else()
        target_link_libraries(${MODULE_PARENT} PUBLIC ${MODULE_LINK_LIBS})
    endif()
endfunction()