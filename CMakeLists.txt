cmake_minimum_required(VERSION 3.1)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})

add_library(CTFuzzInstrumentSelf MODULE
  src/lib/ct-fuzz-instrument-self.cpp
  src/lib/ct-fuzz-naming.cpp
  src/lib/ct-fuzz-options.cpp
  src/lib/ct-fuzz-instrument-utils.cpp
)

add_library(CTFuzzInstrumentSrc MODULE
  src/lib/ct-fuzz-instrument-src.cpp
  src/lib/ct-fuzz-instrument-utils.cpp
  src/lib/ct-fuzz-naming.cpp
)

add_library(xxHash MODULE
  xxHash/xxhash.c)

add_library(CTFuzzReadInputs MODULE
  src/lib/ct-fuzz-read-inputs.cpp
  src/lib/ct-fuzz-instrument-utils.cpp
)

target_include_directories(CTFuzzInstrumentSrc PUBLIC
  ${LLVM_INCLUDE_DIRS}
  src/include
)

target_include_directories(CTFuzzInstrumentSelf PUBLIC
  ${LLVM_INCLUDE_DIRS}
  src/include
)

target_include_directories(CTFuzzReadInputs PUBLIC
  ${LLVM_INCLUDE_DIRS}
  src/include
)

target_include_directories(xxHash PRIVATE
  xxHash
)

#target_link_libraries(CTFuzzInstrument
#  ${LLVM_LIBRARY_DIRS}
#)
link_directories(${LLVM_LIBRARY_DIRS})

find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config-3.9 llvm-config PATHS ${LLVM_CONFIG} DOC "llvm-config")

if (LLVM_CONFIG_EXECUTABLE STREQUAL "LLVM_CONFIG_EXECUTABLE-NOTFOUND")
  message(FATAL_ERROR "llvm-config could not be found!")
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
  OUTPUT_VARIABLE CMAKE_CXX_FLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REPLACE "-fvisibility-inlines-hidden" "" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")

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
set_target_properties(CTFuzzInstrumentSrc PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)
set_target_properties(CTFuzzInstrumentSelf PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)
set_target_properties(CTFuzzReadInputs PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(CTFuzzInstrumentSrc PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
    set_target_properties(CTFuzzInstrumentSelf PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
  set_target_properties(CTFuzzReadInputs PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)

#target_link_libraries(CTFuzzInstrument ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS} ${LLVM_LDFLAGS})
