message(FATAL_ERROR "Unfinished yet")

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

if(NOT WIN32)
    message(FATAL_ERROR "This project requires Windows")
elseif(NOT MSVC)
    message(FATAL_ERROR "Only support MSVC for the moment")
elseif(MSVC_VERSION LESS 1910)
    message(FATAL_ERROR "MSVC version too old")
elseif(if(CMAKE_SIZEOF_VOID_P EQUAL 8))
    message(FATAL_ERROR "This project requires 32 bit")
endif()

set(CMAKE_SUPPRESS_REGENERATION true) # Remove ZERO_CHECK project.
set_property(GLOBAL PROPERTY USE_FOLDERS ON) # Organize projects into folders within the VS solution.

# Prevent incremental linking.
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows" AND MSVC)
	set(MSVC_INCREMENTAL_DEFAULT OFF)
endif()

# Set available project configurations.
SET(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "Configs" FORCE)

# Declare the project.
project(Phobos VERSION 3.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(OUTPUT_DIRECTORY_UPPER ${CMAKE_SOURCE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY_UPPER}/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY_UPPER}/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY_UPPER}/${CMAKE_BUILD_TYPE})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY_UPPER}/${CMAKE_BUILD_TYPE}/IntDir)

# Macros
add_compile_definitions(SYR_VER=2)
add_compile_definitions(HAS_EXCEPTIONS=0)
add_compile_definitions(NOMINMAX)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(WIN32_LEAN_AND_MEAN)
add_compile_definitions(_WIN32_WINNT=0x0601)
add_compile_definitions(NTDDI_VERSION=0x06010000)
add_compile_definitions(PHOBOS_DLL = "${PROJECT_NAME}.dll")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
else()
    add_compile_definitions(IS_RELEASE_VER)
endif()

# Warning suppression
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100 /wd4201 /wd4530 /wd4731 /wd4740 /wd4458 /wd4819 /wd5103 /wd5105")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd26495")
endif()

# project source files
include_directories(${CMAKE_SOURCE_DIR}/YRpp)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/lib)
set(EXCLUDED_HEADERS
 "${CMAKE_SOURCE_DIR}/src/Commands/Dummy.h"
 "${CMAKE_SOURCE_DIR}/src/Ext/Bullet/Trajectories/SampleTrajectory.h"
 "${CMAKE_SOURCE_DIR}/src/Utilities/Coroutine.h"
 )
list(REMOVE_ITEM INCLUDE_DIRS ${EXCLUDED_HEADERS})

set(EXCLUDED_FILES
 "${CMAKE_SOURCE_DIR}/src/Commands/Dummy.cpp"
 "${CMAKE_SOURCE_DIR}/src/Ext/Bullet/Trajectories/SampleTrajectory.cpp"
 "${CMAKE_SOURCE_DIR}/src/Misc/PlaceHolders.cpp"
)
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM SRC_FILES "${EXCLUDED_FILES}")

add_library(${PROJECT_NAME}
 ${SRC_FILES}
 ${CMAKE_SOURCE_DIR}/src/version.rc
 ${CMAKE_SOURCE_DIR}/YRpp/StaticInits.cpp
)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()

target_compile_options(${PROJECT_NAME} PUBLIC
#TODO
)

target_link_options(${PROJECT_NAME} PUBLIC
#TODO
)

#TODO
