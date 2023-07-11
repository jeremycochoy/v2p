cmake_minimum_required(VERSION 3.13)

#Create options
option(CMAKE_BUILD_TYPE "Debug for debug symbols, Release for optimized version" Release)
option(BUILD_STATIC "Tell if the static library should be compiled" ON)
option(BUILD_DYNAMIC "Tell if the dynamic library should be compiled" ON)
option(BUILD_TEST "Compile the test applications and allow to run them with 'make test'" ON)
if(APPLE)
	option(BUILD_COCOATOUCH_FRAMEWORK "Create a cocoatouch V2p framework for iPhone apps" ON)
endif(APPLE)

# C++ 2011 flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(v2p-api VERSION 0.1.0 DESCRIPTION "V2p library" LANGUAGES C)
project(v2p-api_static VERSION 0.1.0 DESCRIPTION "V2p library" LANGUAGES C)
project(tst-suite VERSION 0.1.0 DESCRIPTION "V2p library" LANGUAGES CXX)

file(GLOB v2p-api-src src/*.c)

#Object library used for both dynamic and static library
add_library(v2p-api-obj OBJECT ${v2p-api-src})
set_property(TARGET v2p-api-obj PROPERTY POSITION_INDEPENDENT_CODE 1)

# Allow to include files with relative path starting from source & include directories
target_include_directories(v2p-api-obj PRIVATE include)
target_include_directories(v2p-api-obj PRIVATE lib)
target_include_directories(v2p-api-obj PRIVATE src)

# Static library
if(BUILD_STATIC)
  add_library(v2p-api_static STATIC $<TARGET_OBJECTS:v2p-api-obj>)

  set_target_properties(v2p-api_static PROPERTIES VERSION ${PROJECT_VERSION})
  set_target_properties(v2p-api_static PROPERTIES SOVERSION 1)
  set_target_properties(v2p-api_static PROPERTIES PUBLIC_HEADER include/v2p.h)
endif(BUILD_STATIC)

# Shared library
if(BUILD_DYNAMIC)
  add_library(v2p-api SHARED $<TARGET_OBJECTS:v2p-api-obj>)
  add_compile_definitions(BUILDING_DLL=1)
  set(WINDOWS_EXPORT_ALL_SYMBOLS, TRUE)
  # Copy dll to python/v2p folder
  add_custom_command(TARGET v2p-api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:v2p-api> "../python/v2p/v2p-api.dll"
  )
endif(BUILD_DYNAMIC)

if(BUILD_COCOATOUCH_FRAMEWORK)
  add_library(v2pApi SHARED $<TARGET_OBJECTS:v2p-api-obj>)
  set_target_properties(v2pApi PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION C
    MACOSX_FRAMEWORK_IDENTIFIER fr.v2p.v2pApi
    # MACOSX_FRAMEWORK_INFO_PLIST Info.plist
    PUBLIC_HEADER v2p.h
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
  )
endif(BUILD_COCOATOUCH_FRAMEWORK)

#
# TESTS
#


file(GLOB SRC_TEST_LIB test/lib/*.cpp)
file(GLOB SRC_TEST_FILES test/*.cpp)
add_executable(test-suite
  ${SRC_TEST_FILES}
  ${SRC_TEST_LIB}
)
target_include_directories(test-suite PRIVATE test)
target_include_directories(test-suite PRIVATE include)
target_include_directories(test-suite PRIVATE lib)
target_link_libraries(test-suite v2p-api_static)

#Run tests
add_test (NAME test-suite
  WORKING_DIRECTORY "test/"
COMMAND "test-suite")

#Run Python tests
add_custom_target(accuracy ${CMAKE_COMMAND} -E chdir .. python3 python/v2p-test/accuracy.py \${V2P_SAMPLES_DIRECTORY}) 
