cmake_minimum_required(VERSION 3.1)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(src/include)
link_directories(${LLVM_LIBRARY_DIRS})

#add_subdirectory(src/lib)  # Use your pass name here.

#add_library(ExtractPass MODULE
#    # List your source files here.
#    src/lib/ct-fuzz-extract-expr.cpp
#)

add_library(CTFuzzInstrument MODULE
  src/lib/ct-fuzz-instrument.cpp
  src/lib/ct-fuzz-naming.cpp
)

find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config-3.9 llvm-config PATHS ${LLVM_CONFIG} DOC "llvm-config")

if (LLVM_CONFIG_EXECUTABLE STREQUAL "LLVM_CONFIG_EXECUTABLE-NOTFOUND")
  message(FATAL_ERROR "llvm-config could not be found!")
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
  OUTPUT_VARIABLE CMAKE_CXX_FLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

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

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Use C++11 to compile our pass (i.e., supply -std=c++11).
#target_compile_features(ExtractPass PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
#set_target_properties(ExtractPass PROPERTIES
#    COMPILE_FLAGS "-fno-rtti"
#)
set_target_properties(CTFuzzInstrument PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(CTFuzzInstrument PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)

#target_link_libraries(CTFuzzInstrument ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS} ${LLVM_LDFLAGS})
