cmake_minimum_required(VERSION 3.10)

project("asm-analyze" VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/etc")
add_executable(${PROJECT_NAME} "main.cpp")

##########
set(ARGS "-fpic -fprofile-generate -funroll-loops -fomit-frame-pointer -finline-functions -pedantic -Wall -Wextra -Wunused -Wdeprecated -Os -march=native -mtune=native")
set(CARGS "-fpeel-loops -fdce -fipa-cp -fipa-icf ${ARGS}")
##########
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "-malign-data -fprefetch-loops ${CARGS}")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "${CARGS}")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${ARGS}")
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)