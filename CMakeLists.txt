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


# Flags

# Sanitizer flags (need for compile and link)
set(SANITIZER_FLAGS
    -fsanitize=address,undefined
    -fsanitize=signed-integer-overflow
)

# Linker flags specifically for sanitizers
set(SANITIZER_LINK_FLAGS
    ${SANITIZER_FLAGS}
)


# Base compile flags
set(BASE_C_FLAGS
    -fdiagnostics-color=always  
    -O0
    -Wall -Wextra -Wpedantic -Werror 

    -g                        
)


add_compile_options(${BASE_C_FLAGS} ${SANITIZER_FLAGS})
add_link_options(
    ${SANITIZER_LINK_FLAGS}
    -rdynamic
)


set(SRC_DIR "${PROJECT_SOURCE_DIR}/source")
set(INC_DIR "${PROJECT_SOURCE_DIR}/include")

# Recursively gather all .c files under source/
file(GLOB_RECURSE SOURCE_FILES
    "${SRC_DIR}/*.c"
)

# Make your headers available to targets
include_directories(${INC_DIR})


add_executable(crapiler
    ${SOURCE_FILES}
)

# If you ever need math, uncomment:
# target_link_libraries(crapiler PRIVATE m)


# install(TARGETS crapiler DESTINATION bin)
# install(DIRECTORY ${INC_DIR}/ DESTINATION include/crapiler)
