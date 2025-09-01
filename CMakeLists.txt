cmake_minimum_required(VERSION 3.28)

project(crapiler
    VERSION 1.0
    LANGUAGES C
)

# Use Clang 19
set(CMAKE_C_COMPILER clang-19)

set(CMAKE_C_SCAN_FOR_MODULES OFF)


set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED off)
set(CMAKE_C_EXTENSIONS OFF)

# Always generate compile commands (e.g. for clang‑tidy, VSCode intellisense)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# First try pkg-config
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(UNWIND libunwind)
  if (UNWIND_FOUND)
    message(STATUS "Found libunwind via pkg-config")
    include_directories(${UNWIND_INCLUDE_DIRS})
    link_directories(${UNWIND_LIBRARY_DIRS})
    # For pkg-config, we need to manually add the arch-specific library
    set(UNWIND_LIBRARIES ${UNWIND_LIBRARIES} unwind-x86_64)
  endif()
endif()

# Fallback to manual find if pkg-config failed or didn't find arch library
if (NOT UNWIND_FOUND)
  message(STATUS "Trying manual libunwind detection...")
  
  find_path(
    UNWIND_INCLUDE_DIR
    NAMES libunwind.h
    PATHS
      /usr/include
      /usr/local/include
      /usr/include/libunwind
      /usr/include/x86_64-linux-gnu
  )
  
  find_library(
    UNWIND_LIBRARY
    NAMES unwind
    PATHS
      /usr/lib
      /usr/local/lib
      /usr/lib/x86_64-linux-gnu
  )
  
  find_library(UNWIND_ARCH_LIBRARY
    NAMES unwind-x86_64
    PATHS
      /usr/lib
      /usr/local/lib
      /usr/lib/x86_64-linux-gnu
  )
  
  # Debug output
  message(STATUS "UNWIND_INCLUDE_DIR: ${UNWIND_INCLUDE_DIR}")
  message(STATUS "UNWIND_LIBRARY: ${UNWIND_LIBRARY}")
  message(STATUS "UNWIND_ARCH_LIBRARY: ${UNWIND_ARCH_LIBRARY}")
  
  if (UNWIND_INCLUDE_DIR AND UNWIND_LIBRARY AND UNWIND_ARCH_LIBRARY)
    set(UNWIND_FOUND TRUE)
    set(UNWIND_INCLUDE_DIRS ${UNWIND_INCLUDE_DIR})
    set(UNWIND_LIBRARIES ${UNWIND_LIBRARY} ${UNWIND_ARCH_LIBRARY})
    message(STATUS "Manual libunwind detection successful")
  else()
    message(FATAL_ERROR "Could not find all required libunwind components")
  endif()
endif()

if (NOT UNWIND_FOUND)
  message(FATAL_ERROR
    "libunwind not found! Please install libunwind-dev "
    "or point CMAKE_PREFIX_PATH to your libunwind installation."
  )
endif()

message(STATUS "Final UNWIND_LIBRARIES: ${UNWIND_LIBRARIES}")

# Flags

# Sanitizer flags (need for compile and link)
set(SANITIZER_FLAGS
    -fsanitize=address,undefined
    -fsanitize=signed-integer-overflow
)
set(SANITIZER_LINK_FLAGS
    ${SANITIZER_FLAGS}
)

# Set maximum stack frame size
set(DEBUG_MAX_FRAME_SIZE 4096)

# Base compile flags
set(BASE_C_FLAGS
    -fdiagnostics-color=always  
    -O0
    -g
    -D_GNU_SOURCE
    -fno-pic

    -Wall -Wextra -Wpedantic -Werror 

    # Everything
    -Weverything -Wno-missing-include-dirs -Wno-pre-c23-compat -Wno-declaration-after-statement -Wno-padded -Wno-switch-enum -Wno-cast-align -Wno-cast-qual
    -Wno-unreachable-code-break
    -Wno-missing-prototypes # Broken
    -Wno-missing-variable-declarations # Broken
    -Wno-unsafe-buffer-usage # Broken
    -Wno-covered-switch-default # Conflicts with -Wswitch-default)
    -Wno-switch-default # Breaks v-table
    -Wno-cast-function-type-strict # Breaks v-table
    -Wno-pre-c11-compat #Useless
    -Wno-float-equal

    # Some flags to try
    # -Wcast-qual // Warns on const incorrect casts, pretty good but breaks with some C lib functions
    # -Wpadded // Tell you about struct padding
    # -Wunreachable-code-break // Try this one
    -Walloca -Wchar-subscripts 
    -Wdouble-promotion -Wenum-conversion 
    -Wextra-semi -Wformat-overflow -Wformat-signedness -Wformat=2 
    -Wframe-larger-than=${DEBUG_MAX_FRAME_SIZE}
    -Wmismatched-tags -Wmissing-braces -Wmultichar 
    -Wpointer-arith -Wrange-loop-construct -Wshadow -Wproperty-attribute-mismatch 
    -Wuninitialized -Wvla -Wwrite-strings 

    -Wno-deprecated-declarations
    -Wconversion
    -Wunused
    -Wold-style-cast
    -Wsign-conversion -Wnull-dereference
    -Wmisleading-indentation -Wimplicit-fallthrough -Wunused-label
    -Wno-dollar-in-identifier-extension
    -pedantic-errors                  
)


add_compile_options(${BASE_C_FLAGS} ${SANITIZER_FLAGS})
add_link_options(
    ${SANITIZER_LINK_FLAGS}
    -rdynamic
    -no-pie
)


set(SRC_DIR "${PROJECT_SOURCE_DIR}/source")

# Recursively gather all .c files under source/
file(GLOB_RECURSE LIB_SOURCE_FILES
    "${SRC_DIR}/*.c"
)

# Remove main.c from the library sources if you have one
list(FILTER LIB_SOURCE_FILES EXCLUDE REGEX ".*main\\.c$")

add_library(crapiler_lib STATIC ${LIB_SOURCE_FILES})

target_include_directories(crapiler_lib PUBLIC
    ${SRC_DIR}
    ${SRC_DIR}/ast
    ${SRC_DIR}/tac
    ${SRC_DIR}/codegen
    ${SRC_DIR}/lib
    ${UNWIND_INCLUDE_DIRS}
)

target_link_libraries(crapiler_lib PUBLIC 
    ${UNWIND_LIBRARIES}
    unwind-x86_64
    dl
)

# Main executable
add_executable(crapiler
    ${SRC_DIR}/main.c  # or whatever your main file is called
)

target_link_libraries(crapiler PRIVATE crapiler_lib)

# Tell the compiler where to look for libunwind.h
target_include_directories(crapiler PRIVATE ${UNWIND_INCLUDE_DIRS})

# Link libunwind libraries
target_link_libraries(crapiler PRIVATE 
    ${UNWIND_LIBRARIES}
    # Fallback: ensure we have the arch-specific library even if detection failed
    unwind-x86_64
    # Add dl for dladdr support
    dl
)



option(COMPILE_TESTS "Compile tests" ON)

# Tests
if(COMPILE_TESTS)
  set(TEST_DIR "${PROJECT_SOURCE_DIR}/tests")

  add_executable(short_vec_test ${TEST_DIR}/lib/short_vec_test.c)
  target_link_libraries(short_vec_test PRIVATE crapiler_lib)
  set_target_properties(short_vec_test PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY tests
  )
endif()