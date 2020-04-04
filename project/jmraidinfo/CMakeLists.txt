cmake_minimum_required(VERSION 3.0)
project(jmraid C)

set(CMAKE_C_STANDARD 99)

add_library(common STATIC ../../lib/src/disk.c ../../lib/src/jmraid.c)
set_target_properties(common PROPERTIES LINKER_LANGUAGE C)
include_directories(../../lib/inc)

add_executable(jmraid src/main.c)
target_link_libraries(jmraid common)
