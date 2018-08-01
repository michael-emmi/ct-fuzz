cmake_minimum_required(VERSION 3.1)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})

add_library(InstantiateHarness MODULE
  src/lib/instantiate-harness.cpp
  src/lib/utils.cpp
  src/lib/read-inputs.cpp
)

add_library(InstrumentMonitors MODULE
  src/lib/instrument-monitors.cpp
)

add_library(GenerateSeeds MODULE
  src/lib/generate-seeds.cpp
)

add_library(xxHash SHARED
  xxHash/xxhash.c)

target_include_directories(InstrumentMonitors PUBLIC
  ${LLVM_INCLUDE_DIRS}
  src/include
)

target_include_directories(InstantiateHarness PUBLIC
  ${LLVM_INCLUDE_DIRS}
  src/include
)

target_include_directories(GenerateSeeds PUBLIC
  ${LLVM_INCLUDE_DIRS}
  src/include
)

target_include_directories(xxHash PRIVATE
  xxHash
)

link_directories(${LLVM_LIBRARY_DIRS})

find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config-3.9 llvm-config PATHS ${LLVM_CONFIG} DOC "llvm-config")
find_program(MAKE make)

if (LLVM_CONFIG_EXECUTABLE STREQUAL "LLVM_CONFIG_EXECUTABLE-NOTFOUND")
  message(FATAL_ERROR "llvm-config could not be found!")
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
  OUTPUT_VARIABLE CMAKE_CXX_FLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs
  OUTPUT_VARIABLE LLVM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
  OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
  OUTPUT_VARIABLE LLVM_LDFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_dependencies(InstrumentMonitors compile-afl)
add_custom_target(
  compile-afl
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/afl/afl-llvm-pass.so
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/afl/afl-llvm-pass.so
  COMMAND ${MAKE}
  COMMAND ${MAKE} -C llvm_mode
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/afl
)
set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/afl/afl-llvm-pass.so
  PROPERTIES GENERATED TRUE
)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Use C++11 to compile our pass (i.e., supply -std=c++11).
#target_compile_features(ExtractPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
#set_target_properties(ExtractPass PROPERTIES
#    COMPILE_FLAGS "-fno-rtti"
#)
set_target_properties(InstrumentMonitors InstantiateHarness GenerateSeeds
  PROPERTIES COMPILE_FLAGS "-fno-rtti"
)

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
  set_target_properties(InstrumentMonitors InstantiateHarness GenerateSeeds
      PROPERTIES LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)

INSTALL(TARGETS InstrumentMonitors InstantiateHarness xxHash GenerateSeeds
  LIBRARY DESTINATION lib/ct-fuzz
)

INSTALL(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/bin/ct-fuzz
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
  GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
  DESTINATION bin
)

INSTALL(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/afl/afl-clang-fast
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
  GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
  DESTINATION bin
  RENAME ct-fuzz-afl-clang-fast
)

INSTALL(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/afl/afl-clang-fast++
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
  GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
  DESTINATION bin
  RENAME ct-fuzz-afl-clang-fast++
)

INSTALL(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/afl/afl-llvm-pass.so
  ${CMAKE_CURRENT_SOURCE_DIR}/afl/afl-llvm-rt.o
  PERMISSIONS OWNER_WRITE OWNER_READ
  GROUP_READ WORLD_READ
  DESTINATION lib/ct-fuzz
)

INSTALL(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/xxHash/xxhash.h
  PERMISSIONS OWNER_WRITE OWNER_READ
  GROUP_READ WORLD_READ
  DESTINATION share/xxHash/include
)

INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/share/ct-fuzz
  DESTINATION share
  USE_SOURCE_PERMISSIONS
  FILES_MATCHING PATTERN "*.py" PATTERN "*.h" PATTERN "*.c"
)
