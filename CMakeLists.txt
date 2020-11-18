cmake_minimum_required(VERSION 3.15)

set(ENGINE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(ENGINE_MODULE_DIRECTORY "${ENGINE_ROOT_DIR}/Engine/Modules")
set(ENGINE_MODULE_THIRDPARTY_DIRECTORY "${ENGINE_ROOT_DIR}/Engine/ThirdParty")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(ExternalProject)

# Grab the build system includes from Engine/Build/CMake
set(BUILD_INCLUDES_DIR "${ENGINE_ROOT_DIR}/Build/CMake")
file(GLOB_RECURSE BUILD_INCLUDES "${BUILD_INCLUDES_DIR}/*.cmake")
foreach(BUILD_INCLUDE IN ITEMS ${BUILD_INCLUDES})
    # message("Including build function: ${BUILD_INCLUDE}")
    include(${BUILD_INCLUDE})
endforeach()

# For now we are going to manually specify modules to enable
# In the future we can GLOB_RECURSE them to build all when we want
# file(GLOB_RECURSE MODULES_CMAKE_FILES_LIST "${ENGINE_MODULE_DIRECTORY}/*.cmake")
# file(GLOB_RECURSE THIRDPARTYMODULES_CMAKE_FILES_LIST "${ENGINE_MODULE_THIRDPARTY_DIRECTORY}/*.cmake")

macro(subdirectories result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

subdirectories(MODULE_DIRECTORIES "${ENGINE_MODULE_DIRECTORY}")
subdirectories(THIRDPARTYMODULE_DIRECTORIES "${ENGINE_MODULE_THIRDPARTY_DIRECTORY}")

foreach(MODULE_DIR IN ITEMS ${THIRDPARTYMODULE_DIRECTORIES})
    file(GLOB MODULE_DIR_CMAKE_FILES "${ENGINE_MODULE_THIRDPARTY_DIRECTORY}/${MODULE_DIR}/*.cmake")
    list(APPEND THIRDPARTYMODULES_CMAKE_FILES_LIST ${MODULE_DIR_CMAKE_FILES})
    unset(MODULE_DIR_CMAKE_FILES)
endforeach()

foreach(MODULE_DIR IN ITEMS ${MODULE_DIRECTORIES})
    file(GLOB MODULE_DIR_CMAKE_FILES "${ENGINE_MODULE_DIRECTORY}/${MODULE_DIR}/*.cmake")
    list(APPEND MODULES_CMAKE_FILES_LIST ${MODULE_DIR_CMAKE_FILES})
    unset(MODULE_DIR_CMAKE_FILES)
endforeach()

# Include all of the module definitions specified above
foreach(MODULE_CMAKE_FILE IN ITEMS ${THIRDPARTYMODULES_CMAKE_FILES_LIST})
    message("Including module: ${MODULE_CMAKE_FILE}")
    include(${MODULE_CMAKE_FILE})
    message("")
endforeach()

foreach(MODULE_CMAKE_FILE IN ITEMS ${MODULES_CMAKE_FILES_LIST})
    message("Including module: ${MODULE_CMAKE_FILE}")
    include(${MODULE_CMAKE_FILE})
    message("")
endforeach()